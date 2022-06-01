#include "nidavellir.h"
#include "applications.h"
#include "resource_tool.h"
#include "target.h"
#include "lib_clk.h"

//#define     P_BACKLIGHT_ON      GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT |= 0x10; //推挽输出高
//#define     P_BACKLIGHT_OFF     GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT &=~0x10; //推挽输出低

#if 1  //  GS

int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return sync_data_record(instance, pipeline, mesg);
}

#endif


void SecTick_Task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    struct message_t mesg;
    
    if(mbase_control_block->AutoInv > 0)   mbase_control_block->AutoInv--;
    
    if(mbase_control_block->KeyInv > 0)    mbase_control_block->KeyInv--;
       
    if(mbase_control_block->commdisp_ir485 > 0) mbase_control_block->commdisp_ir485--;
    
    if(mbase_control_block->commdisp_plc > 0)   mbase_control_block->commdisp_plc--;

    fr_dl698_sec_task(instance, pipeline);


//    backlight_pro(instance, pipeline);
    Meter_Run_State(instance, pipeline);//运行状态字刷新
    
    
    
    if( instance->ac_power_statue != AC_POWER_ON ) //低功耗
    {
        mbase_control_block->power_down_sec_accum++;
        
        event_LowPower_pro(instance, pipeline, &mesg); // 事件 低功耗下处理
        
/*        if( (mbase_control_block->Key & F_KeyDispay) && (mbase_control_block->KeyInv == 0) )//退出键显,息屏
        {
            mbase_control_block->Key &= ~F_KeyDispay;
            mbase_control_block->AutoPtr = 0xFF;
            mbase_control_block->lcd_keydis_keep_sec = 0;
            //Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
            mbase_control_block->Key &= ~F_KeyAwake;
        }*/
        
        
    }
    else
    {
        mbase_control_block->power_on_sec_accum++;
        
    }
}

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_start) - offsetof(struct mbase_control_block_t, persistent_section_end);
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_end) - offsetof(struct mbase_control_block_t, static_section_start);
    uint32_t addr = 0;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    //cmd len data cmd 接入校验
    //调用领域层完成业务  要反映用例场景
    for (int i = 0; i < max_event_flag; i++)
    {
        if (event_flags_get(instance,i) ==  1)
        {
            //call event_handler(instance,pipeline,mesg);

            event_flags_clr(instance, i);
        }
    }
    uint8_t buf[20] = {0};
    int r = recv_mesg_v2(instance,pipeline,mesg, buf, 20);
    if(r > 0 )
    {
      switch (mesg->ctrl)
      {
      case 0x86://采样的外设数据抵达
          break;
      case 0x87:
          break;
      case 0x88://control_block控制命令 比如清零
          //controlData(instance,pipeline,mesg);
          break;
      case MSG_SECOND:
          SecTick_Task(instance, pipeline);
          
          Rtc_Prossecc(instance, pipeline);
          
          Electrical_measurement(instance, pipeline);
          
          Energy_Pluse_Count(instance, pipeline);
          
          Energy_Meter(instance, pipeline);

          freeze_pro(instance, pipeline, mesg);
          
          event_pro(instance, pipeline, mesg);

          Display(instance, pipeline);
          
          backlight_pro(instance, pipeline, mesg);
          
          do_relay(instance, pipeline);
          
          relay_led_ctl(instance, pipeline);
          
          IOStateCheck(instance, pipeline);

          TouFeePro(instance, pipeline);
          
          break;
      case MSG_PAGE_KEY_UP:
           Key_Display(instance, pipeline, MSG_PAGE_KEY_UP);
           Display(instance, pipeline);
          break;
      case MSG_POWERON:
        //上电恢复参数
        read_mcu_rtc(0x00, instance);
        get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
        nd_get_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, (uint8_t*)addr, mbase_block_static_size); //将EPROM中的数据恢复到RAM;  
        //上电需要恢复数据
        //获取对应的RAM地址
        get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
        nd_get_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将EPROM中的数据恢复到RAM;        
        Electrical_measurement_poweron(instance, pipeline);
        Energy_Meter_PowerON(instance, pipeline);
        AllDisplayLCD(instance, pipeline);
        mbase_control_block->key_state = get_key_status(instance);//获取秘钥状态
        Magnetic_hold_relay_Init();
        fr_dl698_power_on(instance, pipeline);
        mbase_control_block->power_on_sec_accum = 0;
        mbase_cb->Fee |= (F_TOUChange+F_ChangeTmOn+F_DAYTABChange);;    //获取当前时区时段费率
        break;
      case MSG_POWEROFF:
        //掉电需要保存数据
        //获取对应的RAM地址
        get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
        nd_set_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;      
        mbase_control_block->power_down_sec_accum = 0;
        
        break;
      default:
        break;
      }
    }

    return 0;
}

int app_meter_lowpower_wakeup_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    struct mbase_control_block_t* mbase_control_block;
    mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if (mbase_control_block == NULL)
    {
        return -1;
    }
    uint8_t buf[20] = {0};
    int r = recv_mesg_v2(instance,pipeline,mesg, buf, 20);
    if(r > 0 )
    {
        //开启EE电源和IO口时钟
        CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
        GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
        GPIOC->IE &= ~(BIT4|BIT5|BIT6);
        GPIOC->DAT |= (BIT6);

        switch (mesg->ctrl)
        {
            case MSG_SECOND:
                SecTick_Task(instance, pipeline);

                Rtc_Prossecc(instance, pipeline);
                
                Key_Display(instance, pipeline, MSG_SECOND);

                if(mbase_control_block->Key & F_KeyAwake)
                {
                    Display(instance, pipeline);
                }

                IOStateCheck(instance, pipeline);

                break;
            case MSG_PAGE_KEY_UP:
                Key_Display(instance, pipeline, MSG_PAGE_KEY_UP);
                Display(instance, pipeline);
                break;
            default:
                break;
        }
    }
    return 0;
}

