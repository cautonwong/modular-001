#include "nidavellir.h"
#include "applications.h"
#include "resource_tool.h"
#include "target.h"
#include "lib_clk.h"
#include "lib_conf.h"
#include "fr_698.h"
#include "gs_record.h"

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

    memset(&mesg, 0, sizeof(struct message_t));
    
    if(mbase_control_block->AutoInv > 0)   mbase_control_block->AutoInv--;
    
    if(mbase_control_block->KeyInv > 0)    mbase_control_block->KeyInv--;
       
    if(mbase_control_block->commdisp_ir485 > 0) mbase_control_block->commdisp_ir485--;
    
    if(mbase_control_block->commdisp_plc > 0)   mbase_control_block->commdisp_plc--;

    
    
    if( mbase_control_block->plc_baut_tick > 0)   //载波通道1小时不通讯，复位载波口 
    {
        mbase_control_block->plc_baut_tick--;
        if( mbase_control_block->plc_baut_tick == 0)
        {
            mbase_control_block->Baut_plc_neg =0xfe;
            mbase_control_block->Baut_plc = 6;
            mcu_uart_set_baud(9600, 4);
        }
    }


//  backlight_pro(instance, pipeline);
    Meter_Run_State(instance, pipeline);//运行状态字刷新
    

    
    
    if( instance->ac_power_statue != AC_POWER_ON ) //低功耗
    {
        instance->ac_power_off_sec_cnt++;

        
        //CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);//待调整
        //GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
        //GPIOC->IE &= ~(BIT4|BIT5|BIT6);
        //GPIOC->DAT |= (BIT6);
        
        
        event_LowPower_pro(instance, pipeline, &mesg); // 事件 低功耗下处理
  
        
/*        if( (mbase_control_block->Key & F_KeyDispay) && (mbase_control_block->KeyInv == 0) )//退出键显,息屏
        {
            mbase_control_block->Key &= ~F_KeyDispay;
            mbase_control_block->AutoPtr = 0xFF;
            mbase_control_block->lcd_keydis_keep_sec = 0;
            //Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
            mbase_control_block->Key &= ~F_KeyAwake;
        }*/
        if(instance->time.tm_sec == 0)
        {
            if(ANA_GetStatus(ANA_STATUS_COMP1))//判断电池是否存在
            {
                mbase_control_block->battery_used_time++;   //电池使用时间
            }
        }
    }
    else
    {
        mbase_control_block->power_on_sec_accum++;
        fr_dl698_sec_task(instance, pipeline);
    }
    
    active_report_handle(instance, NULL, &mesg); // 上电每秒调用  下电前X秒调用
}

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_end) - offsetof(struct mbase_control_block_t, persistent_section_start) + 1;
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_end) - offsetof(struct mbase_control_block_t, static_section_start) + 1;
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
            mbase_cb->cmd_dostate = 0;  //上电清除控制命令状态字和错误状态字
            mbase_cb->cmd_errstate = 0;
            mbase_control_block->alarmmin = 0;  //上电后清零
            mbase_control_block->Baut_plc_neg =0xfe;
            mbase_cb->Baut_plc = 6;
            
            freeze_power_on_pro(instance, pipeline, mesg);
            //事件
            event_PowerOn_pro(instance, pipeline, mesg);
            
            //保存电池使用时间
            break;
      case MSG_POWEROFF:
            //开启EE电源和IO口时钟 //
            CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
            GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
            GPIOC->IE &= ~(BIT4|BIT5|BIT6);
            GPIOC->DAT |= (BIT6);
          
            //掉电需要保存数据
            //先刷新停电时间ram
            ndv_memcopy((uint8_t *)(&mbase_control_block->powerdown_tm),  (uint8_t *)&instance->time,  sizeof(mbase_control_block->powerdown_tm));
            //获取对应的RAM地址
            get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
            nd_set_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;      
            
            instance->ac_power_off_sec_cnt = 0;

            event_poweroff_save(instance,pipeline,mesg);

            
            //掉电清空显示相关内容
            memset((uint8_t *)&mbase_control_block->voltage_ac, 0x00, 32);
            memset((uint8_t *)&mbase_control_block->AutoPtr, 0x00, 10);
            //读取电池使用时间
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

