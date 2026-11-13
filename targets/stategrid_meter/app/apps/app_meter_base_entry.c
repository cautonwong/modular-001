#include "nidavellir.h"
#include "resource_tool.h"
#include "target.h"
#include "lib_clk.h"
#include "lib_conf.h"
#include "fr_698.h"
#include "gs_record.h"
#include "utils.h"
#include "cpu_surport.h"
#include "application/apps/app_config.h"
#include "application/apps/app_entry.h"
#include "application/apps/display_applications.h"
#include "domain/events/legacy/events_applications.h"
#include "domain/events/legacy/gs_record.h"
#include "domain/freeze/legacy/freeze_applications.h"
#include "domain/load_control/legacy/load_control_applications.h"
#include "domain/metering/legacy/metering_applications.h"
#include "domain/tariff/legacy/tariff_applications.h"
#include "infra/boards/stategrid_single_phase/relay_config.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/driver_entry.h"
#include "targets/stategrid_meter/mapper_698/resource_map.h"
extern int16_t Check_para_Pro(struct instance_t* instance);
extern void Poffset_adj_oneself(struct instance_t* instance);
#ifdef ND_FIRMWARE_SIM
void nd_sim_stage(const char *name);
#define ND_SIM_STAGE(name) nd_sim_stage(name)
#else
#define ND_SIM_STAGE(name)
#endif
//#define     P_BACKLIGHT_ON      GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT |= 0x10; //推挽输出高
//#define     P_BACKLIGHT_OFF     GPIOB->OEN &=~0x10; GPIOB->IE &=~0x10; GPIOB->ATT &=~0x10; GPIOB->DAT &=~0x10; //推挽输出低

extern const struct dev_info_t rn8209_dev;
extern void rs485_init(struct instance_t* instance, struct pipeline_t* pipeline);
extern const uint32_t baud_table[];

#if 1 

int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return sync_data_record(instance, pipeline, mesg);
}

#endif

int PowerOff_Save_data(struct instance_t* instance,  struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    size_t mbase_block_persistent_size = nd_mbase_offset_persistent_section_end() - nd_mbase_offset_persistent_section_start() + 1;
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
    ndv_memcopy((uint8_t *)(&nd_mbase_powerdown_tm(mbase_cb)),  (uint8_t *)&instance->time,  sizeof(nd_mbase_powerdown_tm(mbase_cb)));

    Energy_Meter_PowerOFF(instance, pipeline); 

    nd_mbase_ec_save_success(mbase_cb) = 0x5AA5;  //保存前 置 标志  

    //获取对应的RAM地址
    get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
    res = get_res(RES_DEV_EEPROM, mbase_block_persistent_eprom);
    nd_set_block(instance, res, (uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;   

    //保存 冻结停电时间
    if (nd_mbase_frz_replenish_end(mbase_cb) == 0x5A)
    {
        tmp_u32 = rtc_mktime((&nd_mbase_powerdown_tm(mbase_cb)));
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PDTime_Frez); // 冻结停电时间
        nd_set_index(instance, addr, (uint8_t *)(&tmp_u32), 4);
        nd_mbase_frz_replenish_end(mbase_cb) = 0;
    }

    nd_mbase_ec_save_success(mbase_cb) = 0;       //保存后 清 标志
    
    instance->ac_power_off_sec_cnt = 0;
   
    //掉电清空显示相关内容
    memset((uint8_t *)&nd_mbase_voltage_ac(mbase_cb), 0x00, 32);
    memset((uint8_t *)&nd_mbase_AutoPtr(mbase_cb), 0x00, 10);
    //清空电池工作时间累加器
    nd_mbase_battery_used_time_cnt(mbase_cb) = 0;
    nd_mbase_PowerOffS_time_low_power_cnt(mbase_cb) = 0;  
    nd_mbase_CoverOpenS_time_low_power_cnt(mbase_cb) = 0;        
    nd_mbase_PowerAbnormalS_time_low_power_cnt(mbase_cb) = 0; 


    nd_mbase_voltage_ac(mbase_cb) = 0;
    nd_mbase_current_l(mbase_cb) = 0;
    nd_mbase_active_power(mbase_cb) = 0;
    nd_mbase_current_n(mbase_cb) = 0;
    nd_mbase_power_factor(mbase_cb) = 0;
    nd_mbase_power_abnormal_tick1(mbase_cb) = 0;
    nd_mbase_power_abnormal_tick(mbase_cb) = 0;
    event_poweroff_save(instance,pipeline,mesg);



    
    __enable_irq();
    
    return true;
}


void SecTick_Task(struct instance_t* instance,  struct pipeline_t* pipeline)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    struct message_t mesg;
    struct mbase_control_block_t* mbase_cb;
    uint8_t baud = DEF_BAUT_PLC;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
   
    memset(&mesg, 0, sizeof(struct message_t));
    
    if(nd_mbase_AutoInv(mbase_cb) > 0)   nd_mbase_AutoInv(mbase_cb)--;
    
    if(nd_mbase_KeyInv(mbase_cb) > 0)    nd_mbase_KeyInv(mbase_cb)--;

    if( instance->ac_power_statue != AC_POWER_ON ) //低功耗
    {
        instance->ac_power_off_sec_cnt++;

        
        event_LowPower_pro(instance, pipeline, &mesg); // 事件 低功耗下处理
  

        if(ANA_GetStatus(ANA_STATUS_COMP1))//判断电池是否存在
        {
            nd_mbase_battery_used_time_cnt(mbase_cb)++;   //电池使用时间 sec
        }

    }
    else
    {
        if(nd_mbase_commdisp_ir485(mbase_cb) > 0) nd_mbase_commdisp_ir485(mbase_cb)--;
    
        if(nd_mbase_commdisp_plc(mbase_cb) > 0)   nd_mbase_commdisp_plc(mbase_cb)--;   
        
        if( nd_mbase_plc_baut_tick(mbase_cb) > 0)   //载波通道1小时不通讯，复位载波口 
        {
            nd_mbase_plc_baut_tick(mbase_cb)--;
            if( nd_mbase_plc_baut_tick(mbase_cb) == 0)
            {
                nd_mbase_Baut_plc_neg(mbase_cb) =0xfe;
                baud = nd_mbase_Baut_plc(mbase_cb);
                if(baud > 10)
                {
                    baud = DEF_BAUT_PLC; 
                }
                mcu_uart_set_baud(baud_table[baud], 4);
            }
        }

        Meter_Run_State(instance, pipeline);//运行状态字刷新        
        check_RtcBat_volt(instance);

        if((nd_mbase_voltage_ac(mbase_cb)) >= (nd_mbase_volt_60(mbase_cb)))  //电压＞60%Un   才累加上电时间//三相
        {
            nd_mbase_power_on_sec_accum(mbase_cb)++; 
        }
        
        fr_dl698_sec_task(instance, pipeline);
        fr_dl645_sec_task(instance, pipeline);
        
        if(instance->time.tm_sec == 33)     //效验
        {
            Check_para_Pro(instance);
        }
        
        if(((instance->time.tm_sec == 44) || (instance->time.tm_sec == 14)) && (nd_mbase_meter_start_cnt(mbase_cb) == 0))//每半分钟做计量监测，发现故障立即复位计量芯片
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
                if(nd_mbase_relay_do(mbase_cb) == 0)
                {
                    nd_mbase_relay_do(mbase_cb) = 0x5A; //远程命令有动作时应先执行，再后补
                }
            }
        }
        
        updata_powerdown_time(instance);
        esam_sec_pro(instance, pipeline);   //esam电源关闭 和esam自检
        mac_suspend_sec_pro(instance, pipeline);//mac挂起处理
        
        if(nd_mbase_factor_mode(mbase_cb) == true) 
        {
            Poffset_adj_oneself(instance);//小信号修正
        }

        upgrade_second_pro(instance);
        
    }
    
    active_report_handle(instance); // 上电每秒调用  下电前X秒调用
}

// !!!在此app中不做IO操作!!!
int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
//    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
    size_t mbase_block_persistent_size = nd_mbase_offset_persistent_section_end() - nd_mbase_offset_persistent_section_start() + 1;
    size_t mbase_block_static_size = nd_mbase_offset_static_section_end() - nd_mbase_offset_static_section_start() + 1;
    size_t persitent_ec_size = nd_mbase_offset_persistent_energy_EC_label() - nd_mbase_offset_persistent_section_start();
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

            ND_SIM_STAGE("SecTick_Task");
            SecTick_Task(instance, pipeline);

            ND_SIM_STAGE("Rtc_Prossecc");
            Rtc_Prossecc(instance, pipeline);
            ND_SIM_STAGE("TouFeePro");
            TouFeePro(instance, pipeline);

            ND_SIM_STAGE("Electrical_measurement");
            Electrical_measurement(instance, pipeline);

            ND_SIM_STAGE("Energy_Pluse_Count");
            Energy_Pluse_Count(instance, pipeline);

            ND_SIM_STAGE("Energy_Meter");
            Energy_Meter(instance, pipeline);

            ND_SIM_STAGE("freeze_pro");
            freeze_pro(instance, pipeline, mesg);

            ND_SIM_STAGE("event_pro");
            event_pro(instance, pipeline, mesg);

            ND_SIM_STAGE("Key_Display");
            Key_Display(instance, pipeline, 0);
            ND_SIM_STAGE("Display");
            Display(instance, pipeline);

            ND_SIM_STAGE("backlight_pro");
            backlight_pro(instance, pipeline, mesg);

            ND_SIM_STAGE("do_relay");
            do_relay(instance, pipeline);

            ND_SIM_STAGE("relay_led_ctl");
            relay_led_ctl(instance, pipeline);

            ND_SIM_STAGE("IOStateCheck");
            IOStateCheck(instance, pipeline);

            instance->reset_flag = 0x00; //走完一圈秒任务后 复位标志清零
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

            //上电需要恢复数据   //跳过电能恢复 由电能模块处理  
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                nd_mbase_offset_persistent_energy_EC_label()  -  nd_mbase_offset_persistent_section_start());
            nd_get_offset(instance, addr, (uint8_t*)&nd_mbase_persistent_energy_EC_label(mbase_cb), (mbase_block_persistent_size - persitent_ec_size));           
            //获取对应的RAM地址
            //get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
            //nd_get_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)(addr + persitent_ec_size),(mbase_block_persistent_size - persitent_ec_size)); //将EPROM中的数据恢复到RAM;        

            ret = Check_para_Pro(instance);     //参数效验
            if (ret < 0) //参数区异常时，恢复485波特率
            {
                auto_parameter_inital(instance);  //首次上电初始化
                rs485_init(instance, pipeline);   //配置485波特率
            }

            save_RtcBat_work_time(instance);
            rtc_check_poweron(instance);
            Energy_Meter_PowerON(instance, pipeline);
            
            Electrical_measurement_poweron(instance, pipeline);
            //AllDisplayLCD(instance, pipeline);
            esam_698_power_on(instance);
           
            Magnetic_hold_relay_Init();
            fr_dl698_power_on(instance, pipeline);
            fr_dl645_power_on(instance, pipeline);
            nd_mbase_power_on_sec_accum(mbase_cb) = 0;
            nd_mbase_Fee(mbase_cb) |= (F_TOUChange+F_ChangeTmOn+F_DAYTABChange);    //获取当前时区时段费率
            nd_mbase_cmd_dostate(mbase_cb) = 0;  //上电清除控制命令状态字和错误状态字
            nd_mbase_cmd_errstate(mbase_cb) = 0;
            nd_mbase_relayerr_record(mbase_cb) = 0;  //继电器误动作记录起始标志
            if(nd_mbase_relay_do(mbase_cb) == 0)
            {
                nd_mbase_relay_do(mbase_cb) = 0x5A; //远程命令有动作时应先执行，再后补
            }
            nd_mbase_alarmmin(mbase_cb) = 0;     //上电后清零
            nd_mbase_limitmin(mbase_cb) = 0;     //上电后清零
            nd_mbase_imaxmin(mbase_cb) = 0;     //上电后清零
            nd_mbase_Baut_plc_neg(mbase_cb) = 0xfe;
            //nd_mbase_Baut_plc(mbase_cb) = DEF_BAUT_PLC;//6;  上电从e2p恢复
            nd_mbase_disp_borrow(mbase_cb) = 0; //借位
            nd_mbase_baud_chg(mbase_cb) = (BIT0|BIT1);               // 上电 更新波特率标志  
            nd_mbase_plc_baut_tick(mbase_cb) = 1; 
            //获取防潜开关
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Pstart_En);
            nd_get_index(instance, addr, (uint8_t*)&nd_mbase_no_filt(mbase_cb), 1);
            
            //背光配置字
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Blt_Alarm_Byte);
            nd_get_index(instance, addr, (uint8_t*)&nd_mbase_blt_alarm(mbase_cb), 4);
            
            //大电流延时时间赋值
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectValue);
            nd_get_index(instance, addr, tmp, 4); 
            mem_swap_cpy((uint8_t*)&nd_mbase_I_Prot(mbase_cb), tmp, 4);
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ImaxProtectTime);
            nd_get_index(instance, addr, tmp, 2); 
            mem_swap_cpy((uint8_t*)&nd_mbase_Prot_time(mbase_cb), tmp, 2);
            
            freeze_power_on_pro(instance, pipeline, mesg);
            //事件
            event_PowerOn_pro(instance, pipeline, mesg);

            instance->need_data_recovery = 0;//数据恢复完成 
            break;
      case MSG_POWEROFF: //掉电相关的操作 不在此操作
            //开启EE电源和IO口时钟 //
/*            CLK_AHBPeriphralCmd((CLK_AHBPERIPHRAL_GPIO), ENABLE);
            GPIOC->OEN &= ~(BIT4|BIT5|BIT6); 
            GPIOC->IE &= ~(BIT4|BIT5|BIT6);
            GPIOC->DAT |= (BIT6);
          
            //掉电需要保存数据
            //刷新停电时间ram
            ndv_memcopy((uint8_t *)(&nd_mbase_powerdown_tm(mbase_control_block)),  (uint8_t *)&instance->time,  sizeof(nd_mbase_powerdown_tm(mbase_control_block)));

            nd_mbase_ec_save_success(mbase_cb) = 0x5AA5;  //保存前 置 标志   
            get_block_frist_addr(instance,RES_DEV_RAM, mbase_block_persistent,&addr);//RAM对应的地址
            nd_set_block(instance,RES_DEV_EEPROM, mbase_block_persistent_eprom,(uint8_t *)addr,mbase_block_persistent_size); //将RAM中的数据保存到EPROM;      
            nd_mbase_ec_save_success(mbase_cb) = 0;       //保存后 清 标志
            instance->ac_power_off_sec_cnt = 0;

            event_poweroff_save(instance,pipeline,mesg);

            
            //掉电清空显示相关内容
            memset((uint8_t *)&nd_mbase_voltage_ac(mbase_control_block), 0x00, 32);
            memset((uint8_t *)&nd_mbase_AutoPtr(mbase_control_block), 0x00, 10);
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
    uint32_t addr = 0;
    size_t mbase_block_persistent_size = nd_mbase_offset_persistent_section_end() - nd_mbase_offset_persistent_section_start() + 1;
    size_t mbase_block_static_size = nd_mbase_offset_static_section_end() - nd_mbase_offset_static_section_start() + 1;
    size_t persitent_ec_size = nd_mbase_offset_persistent_energy_EC_label() - nd_mbase_offset_persistent_section_start();

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

        if(instance->need_data_recovery == 0x5A) //数据恢复
        {
            //上电需要恢复数据   //跳过电能恢复 由电能模块处理  
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
                nd_mbase_offset_persistent_energy_EC_label()  -  nd_mbase_offset_persistent_section_start());
            nd_get_offset(instance, addr, (uint8_t*)&nd_mbase_persistent_energy_EC_label(mbase_control_block), (mbase_block_persistent_size - persitent_ec_size));           
        
            rtc_check_poweron(instance);
            Energy_Meter_PowerON(instance, pipeline);
            instance->need_data_recovery = 0x00;
        }


        switch (mesg->ctrl)
        {
            case MSG_SECOND:

                SecTick_Task(instance, pipeline);

                Rtc_Prossecc(instance, pipeline);
                
                Key_Display(instance, pipeline, MSG_SECOND);

                if(nd_mbase_Key(mbase_control_block) & F_KeyAwake)
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

