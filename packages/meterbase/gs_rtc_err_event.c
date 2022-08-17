#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"


//22年8月8日，8时8分8秒
void set_default_rtc(struct tm *t)
{

    t->tm_sec = 8;
    t->tm_min = 8;
    t->tm_hour = 8;
    t->tm_mday  = 8;
    t->tm_mon = 8;
    t->tm_year = 2022 - 1900;  
}



void tm_to_mcu_rtc_bcd(struct tm t, uint8_t *des)
{
    
    uint8_t year = 0 ,  timer[8] = {0};

    year = (uint8_t)(t.tm_year + 1900 - 2000);

    timer[0] = t.tm_mday;
    timer[1] = (t.tm_mon + 1);
    timer[2] = (year);
    
    des[6] = GetWeek(&timer[0]);
    des[5] = BYTE_BCD(year);//年
    des[4] = BYTE_BCD((t.tm_mon + 1));//月
    des[3] = BYTE_BCD(t.tm_mday);//日
    des[2] = BYTE_BCD(t.tm_hour);//时
    des[1] = BYTE_BCD(t.tm_min);//分
    des[0] = BYTE_BCD(t.tm_sec);//秒
}




//上电时钟检查
int rtc_check_poweron(struct instance_t* instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t year = 0, mark = 0, buffer[8] = {0}, temp[8] = {0}, settimebuf[8] = {0};
    uint32_t t_now = 0, t_pd = 0, addr = 0,  batTime = 0;
    struct tm temp_tm; 
    int ret;


    //保存电池工作时间
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Bat_UseTime);
    ret = nd_get_index(instance, addr, (uint8_t *)&batTime, 4);
    batTime += ctrl_block->battery_used_time_cnt;
    ret = nd_set_index(instance, addr, (uint8_t *)&batTime, 4);
    ctrl_block->battery_used_time_cnt = 0;
    //------------------

    //检查系统时间是否合法
    if (false == rtc_tm_valid(&instance->time))   
    {
        mark = 0x5A;       
    }

    //检查上一次停电时间
    ndv_memcopy(&temp_tm, &ctrl_block->powerdown_tm, sizeof(struct tm));
    if (false == rtc_tm_valid(&temp_tm))   
    {
        set_default_rtc(&temp_tm);
    }

    t_now = rtc_mktime(&instance->time);
    t_pd  = rtc_mktime(&temp_tm); 

    if (t_now < t_pd)
    {
        mark = 0x5A;    
    }    
    else if((t_now  - t_pd) > 86400000) // 1000 day
    {
        mark = 0x5A;
    }


    if (mark == 0x5A)
    {

        record_rollback_after_set_time(instance, 0x5A); // 检查可能有未完成的覆盖
    
        //保存校时前时间 
        rtc_format_change_from_tm(&instance->time,temp); 
        mem_swap_cpy( &buffer[0], &temp[0], 7);
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
        nd_set_index(instance, addr, buffer, 7);
        
        //保存校时后时间 
        rtc_format_change_from_tm(&temp_tm,temp); 
        mem_swap_cpy( &buffer[0], &temp[0], 7);
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
        nd_set_index(instance, addr, buffer, 7);

        record_rollback_after_set_time(instance, 0); // 翻转覆盖

        // 上一次停电时间 写 rtc
        tm_to_mcu_rtc_bcd(temp_tm, settimebuf);
        write_mcu_rtc(0, settimebuf, 7);
 
        ctrl_block->RTCErrS_status = 1;
        
        return -1;
    }
    
    return 1;
}

//写当前时间到停电时间
void write_powerdown_time(struct instance_t* instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t addr;
    
    ndv_memcopy((uint8_t *)(&ctrl_block->powerdown_tm),  (uint8_t *)&instance->time,  sizeof(ctrl_block->powerdown_tm));

    addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
        offsetof(struct mbase_control_block_t, powerdown_tm)) -  offsetof(struct mbase_control_block_t, persistent_section_start);
    nd_set_offset(instance, addr, (uint8_t*)&ctrl_block->powerdown_tm, sizeof(ctrl_block->powerdown_tm));
}


//上电运行 期间 定时保存停电时间  每日12点1分35秒 保存停电时间  --每秒调用，
void updata_powerdown_time(struct instance_t* instance)
{
    if ((instance->time.tm_sec == 35) && (instance->time.tm_min == 1) && (instance->time.tm_hour == 12)) 
    {
        write_powerdown_time(instance);
    }
}


// 上电运行期间 每秒调用 
// 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int RTCErr_event_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
//    uint8_t payload[100];
    int8_t result = 0;

    uint8_t duration_para       = 0; //ctrl_block->RTCErrS_duration_para;
    uint8_t *duration           = &ctrl_block->RTCErrS_duration;
    //uint32_t* accumulate_time   = &ctrl_block->RTCErrS_acc_time;
    uint8_t state               =  ctrl_block->RTCErrS_status;

    uint16_t RecordId           = RTCErrS;

    result = event_duration_pro(duration_para, duration, state, instance, RecordId);

    if (result != 0)
    {
        send_event_mesg_pro(instance, task,  RecordId, NULL, result);
    }

#if 0
    if (true == get_activeflag(instance, RecordId))
    {
        (*accumulate_time)++;// 累计时间累加 
    }
#endif

    return result;
}
