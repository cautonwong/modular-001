#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "domain/events/duration.h"


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

//保存时钟电池工作时间
int save_RtcBat_work_time(struct instance_t* instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t addr = 0, batTime = 0;
    int ret;

    //保存电池工作时间
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Bat_UseTime);
    ret = nd_get_index(instance, addr, (uint8_t *)&batTime, 4);
    batTime += ctrl_block->battery_used_time_cnt;
    ret = nd_set_index(instance, addr, (uint8_t *)&batTime, 4);
    ctrl_block->battery_used_time_cnt = 0;

}


//上电时钟检查
int rtc_check_poweron(struct instance_t* instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t year = 0, mark = 0, buffer[8] = {0}, temp[8] = {0}, settimebuf[8] = {0};
    uint32_t t_now = 0, t_pd = 0, addr = 0;
    struct tm temp_tm; 
    int ret;

    //------------------

    //检查系统时间是否合法
    if (false == rtc_tm_valid(&instance->time))   //可能时间不对，但合法
    {
        mark = 0x5A;       
    }

    //检查上一次停电时间
    ndv_memcopy(&temp_tm, &ctrl_block->powerdown_tm, sizeof(struct tm));
    if (false == rtc_tm_valid(&temp_tm))   
    {
        set_default_rtc(&temp_tm);//优化 可取上一次分钟冻结的时间 待调整
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
        read_mcu_rtc(0x00, instance);
        ctrl_block->RTCErrS_status = 1;


        clr_last_bill_time(instance, LastBillingTime);
        clr_last_bill_time(instance, LastDayFreezTime);
        
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
    uint16_t RecordId = RTCErrS;

    nd_event_duration_t state = {
        .duration = ctrl_block->RTCErrS_duration,
        .active = (uint8_t)(get_activeflag(instance, RecordId) ? 1 : 0),
    };

    nd_event_transition_t result =
        nd_event_duration_step(&state, 0, ctrl_block->RTCErrS_status);

    ctrl_block->RTCErrS_duration = state.duration;
    if (result == ND_EVENT_STARTED) {
        set_activeflag(instance, RecordId);
    } else if (result == ND_EVENT_ENDED) {
        clr_activeflag(instance, RecordId);
    }

    if (result != ND_EVENT_NONE) {
        send_event_mesg_pro(instance, task, RecordId, NULL, (int)result);
    }
    return (int)result;
}
