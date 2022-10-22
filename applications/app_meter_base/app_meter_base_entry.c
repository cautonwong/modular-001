#include "nidavellir.h"
#include "applications.h"
#include "resource_tool.h"
#include "target.h"
#include "lib_clk.h"
#include "lib_conf.h"
#include "fr_698.h"
#include "gs_record.h"
extern int16_t Check_para_Pro(struct instance_t* instance);
extern void Poffset_adj_oneself(struct instance_t* instance);
//#define     P_BACKLIGHT_ON      GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT |= 0x10; //推挽输出高
//#define     P_BACKLIGHT_OFF     GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT &=~0x10; //推挽输出低

extern const struct dev_info_t rn8209_dev;

#if 1  //  GS

int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return sync_data_record(instance, pipeline, mesg);
}

#endif

int PowerOff_Save_data(struct instance_t* instance,  struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_end) - offsetof(struct mbase_control_block_t, persistent_section_start) + 1;
    uint32_t addr = 0;
    uint32_t res = 0, tmp_u32;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    __disable_irq();
    
    CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
    GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
    GPIOC->IE &= ~(BIT4|BIT5|BIT6);
    GPIOC->DAT |= (BIT6);
    
    //掉电需要保存数据
    //先刷新停电时间ram
    ndv_memcopy((uint8_t *)(&mbase_cb->powerdown_tm),  (uint8_t *)&instance->time,  sizeof(mbase_cb->powerdown_tm));
    //获取对应的RAM地址
    get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
    
    mbase_cb->ec_save_success = 0x5AA5;  //保存前 置 标志  
    
    res = get_res(RES_DEV_EEPROM, mbase_block_persistent_eprom);
    nd_set_block(instance, res, (uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;   

    //保存 冻结停电时间
    if (mbase_cb->frz_replenish_end == 0x5A)
    {
        tmp_u32 = rtc_mktime((&mbase_cb->powerdown_tm));
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PDTime_Frez); // 冻结停电时间
        nd_set_index(instance, addr, (uint8_t *)(&tmp_u32), 4);
    }

    mbase_cb->ec_save_success = 0;       //保存后 清 标志
    
    instance->ac_power_off_sec_cnt = 0;
   
    //掉电清空显示相关内容
    memset((uint8_t *)&mbase_cb->voltage_ac, 0x00, 32);
    memset((uint8_t *)&mbase_cb->AutoPtr, 0x00, 10);
    //清空电池工作时间累加器
    mbase_cb->battery_used_time_cnt = 0;
    mbase_cb->PowerOffS_time_low_power_cnt = 0;  
    mbase_cb->CoverOpenS_time_low_power_cnt = 0;        
    mbase_cb->PowerAbnormalS_time_low_power_cnt = 0; 
    
    __enable_irq();
    
    return true;
}


void SecTick_Task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    struct message_t mesg;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    memset(&mesg, 0, sizeof(struct message_t));
    
    if(mbase_cb->AutoInv > 0)   mbase_cb->AutoInv--;
    
    if(mbase_cb->KeyInv > 0)    mbase_cb->KeyInv--;

    if( instance->ac_power_statue != AC_POWER_ON ) //低功耗
    {
        instance->ac_power_off_sec_cnt++;

        
        event_LowPower_pro(instance, pipeline, &mesg); // 事件 低功耗下处理
  

        if(ANA_GetStatus(ANA_STATUS_COMP1))//判断电池是否存在
        {
            mbase_cb->battery_used_time_cnt++;   //电池使用时间 sec
        }

    }
    else
    {
        if(mbase_cb->commdisp_ir485 > 0) mbase_cb->commdisp_ir485--;
    
        if(mbase_cb->commdisp_plc > 0)   mbase_cb->commdisp_plc--;   
        
        if( mbase_cb->plc_baut_tick > 0)   //载波通道1小时不通讯，复位载波口 
        {
            mbase_cb->plc_baut_tick--;
            if( mbase_cb->plc_baut_tick == 0)
            {
                mbase_cb->Baut_plc_neg =0xfe;
                mbase_cb->Baut_plc = 10;
                mcu_uart_set_baud(115200, 4);
            }
        }

    //  backlight_pro(instance, pipeline);
        Meter_Run_State(instance, pipeline);//运行状态字刷新
    
        mbase_cb->power_on_sec_accum++;
        fr_dl698_sec_task(instance, pipeline);
        
        if(instance->time.tm_sec == 33)     //效验
        {
            Check_para_Pro(instance);
        }
        
        if(((instance->time.tm_sec == 44) || (instance->time.tm_sec == 14)) && (mbase_cb->meter_start_cnt == 0))//每半分钟做计量监测，发现故障立即复位计量芯片
        {
            if(false == rn8209_dev.dev_driver_ops->dev_control(instance, &rn8209_dev , METERCHIP_CHK , 0))
            {
                Electrical_measurement_poweron(instance, pipeline);
            }
        }
        
        if(instance->time.tm_sec == 0)  //定时补拉合闸
        {
            if((instance->time.tm_min == 0) || (instance->time.tm_min == 15) || (instance->time.tm_min == 30) || (instance->time.tm_min == 45))
            {
                mbase_cb->relay_do = 0x5A;
            }
        }
        
        updata_powerdown_time(instance);
        esam_sec_pro(instance, pipeline);   //esam电源关闭 和esam自检
        mac_suspend_sec_pro(instance, pipeline);//mac挂起处理
        
        if(mbase_cb->factor_mode == true) 
        {
            Poffset_adj_oneself(instance);//小信号修正
        }
    }
    
    active_report_handle(instance, NULL, &mesg); // 上电每秒调用  下电前X秒调用
}

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_end) - offsetof(struct mbase_control_block_t, persistent_section_start) + 1;
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_end) - offsetof(struct mbase_control_block_t, static_section_start) + 1;
    size_t persitent_ec_size = offsetof(struct mbase_control_block_t, persistent_energy_EC_label) - offsetof(struct mbase_control_block_t, persistent_section_start);
    uint32_t addr = 0;
    uint32_t res = 0;
    int ret;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if (ANA_GetStatus(ANA_STATUS_VDCINDROP))
    {
        return 0;
    }
    
    //cmd len data cmd 接入校验
    //调用领域层完成业务  要反映用例场景

    uint8_t buf[20] = {0}, tmp[4]={0,};
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

            Key_Display(instance, pipeline, 0);
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
            res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom);
            nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size); //将EPROM中的数据恢复到RAM;  
            ret = Check_para_Pro(instance);     //数据效验
            if (ret < 0) //参数区异常时，恢复485波特率
            {
                para_init( instance );
                rs485_init(instance, pipeline);
            }
            //上电需要恢复数据
            //跳过电能恢复 由电能模块处理        
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                offsetof(struct mbase_control_block_t, persistent_energy_EC_label)  -  offsetof(struct mbase_control_block_t, persistent_section_start));
            nd_get_offset(instance, addr, (uint8_t*)&mbase_cb->persistent_energy_EC_label, (mbase_block_persistent_size - persitent_ec_size));           
            //获取对应的RAM地址
            //get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
            //nd_get_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)(addr + persitent_ec_size),(mbase_block_persistent_size - persitent_ec_size)); //将EPROM中的数据恢复到RAM;        

            rtc_check_poweron(instance);
            Energy_Meter_PowerON(instance, pipeline);
            Electrical_measurement_poweron(instance, pipeline);
            //AllDisplayLCD(instance, pipeline);
            esam_698_power_on(instance);
           
            Magnetic_hold_relay_Init();
            fr_dl698_power_on(instance, pipeline);
            mbase_cb->power_on_sec_accum = 0;
            mbase_cb->Fee |= (F_TOUChange+F_ChangeTmOn+F_DAYTABChange);    //获取当前时区时段费率
            mbase_cb->cmd_dostate = 0;  //上电清除控制命令状态字和错误状态字
            mbase_cb->cmd_errstate = 0;
            mbase_cb->relayerr_record = 0;  //继电器误动作记录起始标志
            mbase_cb->relay_do = 0x5A;
            mbase_cb->alarmmin = 0;     //上电后清零
            mbase_cb->limitmin = 0;     //上电后清零
            mbase_cb->imaxmin = 0;     //上电后清零
            mbase_cb->Baut_plc_neg = 0xfe;
            mbase_cb->Baut_plc = 10;//6;
            mbase_cb->disp_borrow = 0; //借位
            mbase_cb->baud_chg = (BIT0|BIT1);               // 上电 更新波特率标志  
            mbase_cb->plc_baut_tick = 1; 
            //获取防潜开关
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Pstart_En);
            nd_get_index(instance, addr, (uint8_t*)&mbase_cb->no_filt, 1);
            
            //背光配置字
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Blt_Alarm_Byte);
            nd_get_index(instance, addr, (uint8_t*)&mbase_cb->blt_alarm, 4);
            
            //大电流延时时间赋值
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
            nd_get_index(instance, addr, tmp, 4); 
            mem_swap_cpy((uint8_t*)&mbase_cb->I_Prot, tmp, 4);
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
            nd_get_index(instance, addr, tmp, 2); 
            mem_swap_cpy((uint8_t*)&mbase_cb->Prot_time, tmp, 2);
            
            freeze_power_on_pro(instance, pipeline, mesg);
            //事件
            event_PowerOn_pro(instance, pipeline, mesg);
            
            //保存电池使用时间
            break;
      case MSG_POWEROFF:
            mbase_cb->voltage_ac = 0;
            mbase_cb->current_l = 0;
            mbase_cb->active_power = 0;
            mbase_cb->current_n = 0;
            mbase_cb->power_factor = 0;
            mbase_cb->power_abnormal_tick1 = 0;
            mbase_cb->power_abnormal_tick = 0;
            event_poweroff_save(instance,pipeline,mesg);
            
            //开启EE电源和IO口时钟 //
/*            CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
            GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
            GPIOC->IE &= ~(BIT4|BIT5|BIT6);
            GPIOC->DAT |= (BIT6);
          
            //掉电需要保存数据
            //刷新停电时间ram
            ndv_memcopy((uint8_t *)(&mbase_control_block->powerdown_tm),  (uint8_t *)&instance->time,  sizeof(mbase_control_block->powerdown_tm));

            mbase_cb->ec_save_success = 0x5AA5;  //保存前 置 标志   
            get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
            nd_set_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;      
            mbase_cb->ec_save_success = 0;       //保存后 清 标志
            instance->ac_power_off_sec_cnt = 0;

            event_poweroff_save(instance,pipeline,mesg);

            
            //掉电清空显示相关内容
            memset((uint8_t *)&mbase_control_block->voltage_ac, 0x00, 32);
            memset((uint8_t *)&mbase_control_block->AutoPtr, 0x00, 10);
            //读取电池使用时间
*/
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

