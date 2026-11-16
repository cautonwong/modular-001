#include "nidavellir.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "application/ports/resource_access.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "port.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "domain/events/duration.h"
#include "domain/events/rtc.h"
#include "targets/stategrid_meter/app/events/events_applications.h"
#include "targets/stategrid_meter/app/freeze/freeze_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"


static void tm_to_datetime(const struct tm *t, nd_datetime_t *d)
{
    d->second = (uint8_t)t->tm_sec;
    d->minute = (uint8_t)t->tm_min;
    d->hour = (uint8_t)t->tm_hour;
    d->day = (uint8_t)t->tm_mday;
    d->month = (uint8_t)(t->tm_mon + 1);
    d->year = (uint16_t)(t->tm_year + 1900);
}

static void datetime_to_tm(const nd_datetime_t *d, struct tm *t)
{
    t->tm_sec = d->second;
    t->tm_min = d->minute;
    t->tm_hour = d->hour;
    t->tm_mday = d->day;
    t->tm_mon = (int)(d->month - 1);
    t->tm_year = (int)(d->year - 1900);
}

//22年8月8日，8时8分8秒
void set_default_rtc(struct tm *t)
{
    nd_datetime_t value;

    nd_rtc_default_datetime(&value);
    datetime_to_tm(&value, t);
}



void tm_to_mcu_rtc_bcd(struct tm t, uint8_t *des)
{
    nd_datetime_t value;

    tm_to_datetime(&t, &value);
    nd_calendar_to_rtc_bcd(&value, des);
    des[6] = nd_calendar_weekday(value.day, value.month, value.year);
}

//保存时钟电池工作时间
int save_RtcBat_work_time(void *instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t addr = 0, batTime = 0;
    int ret;

    //保存电池工作时间
    addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Bat_UseTime);
    ret = nd_get_index(instance, addr, (uint8_t *)&batTime, 4);
    batTime += ctrl_block->battery_used_time_cnt;
    ret = nd_set_index(instance, addr, (uint8_t *)&batTime, 4);
    ctrl_block->battery_used_time_cnt = 0;

}


//上电时钟检查
int rtc_check_poweron(void *instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t year = 0, mark = 0, buffer[8] = {0}, temp[8] = {0}, settimebuf[8] = {0};
    uint32_t t_now = 0, t_pd = 0, addr = 0;
    struct tm temp_tm; 
    int ret;

    //------------------

    //检查上一次停电时间
    ndv_memcopy(&temp_tm, &ctrl_block->powerdown_tm, sizeof(struct tm));
    if (false == rtc_tm_valid(&temp_tm))
    {
        set_default_rtc(&temp_tm);//优化 可取上一次分钟冻结的时间 待调整
    }

    t_now = rtc_mktime(nd_fr698_time(instance));
    t_pd  = rtc_mktime(&temp_tm);

    if (nd_rtc_poweron_fault((uint8_t)(rtc_tm_valid(nd_fr698_time(instance)) ? 1 : 0),
                             t_now, t_pd))
    {
        mark = 0x5A;
    }


    if (mark == 0x5A)
    {

        record_rollback_after_set_time(instance, 0x5A); // 检查可能有未完成的覆盖
    
        //保存校时前时间 
        rtc_format_change_from_tm(nd_fr698_time(instance),temp); 
        mem_swap_cpy( &buffer[0], &temp[0], 7);
        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
        nd_set_index(instance, addr, buffer, 7);
        
        //保存校时后时间 
        rtc_format_change_from_tm(&temp_tm,temp); 
        mem_swap_cpy( &buffer[0], &temp[0], 7);
        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
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
void write_powerdown_time(void *instance)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t addr;
    
    ndv_memcopy((uint8_t *)(&ctrl_block->powerdown_tm),  (uint8_t *)nd_fr698_time(instance),  sizeof(ctrl_block->powerdown_tm));

    addr = ((uint32_t)(DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
        nd_mbase_offset_powerdown_tm()) -  nd_mbase_offset_persistent_section_start();
    nd_set_offset(instance, addr, (uint8_t*)&ctrl_block->powerdown_tm, sizeof(ctrl_block->powerdown_tm));
}


//上电运行 期间 定时保存停电时间  每日12点1分35秒 保存停电时间  --每秒调用，
void updata_powerdown_time(void *instance)
{
    nd_datetime_t now;

    tm_to_datetime(nd_fr698_time(instance), &now);
    if (nd_rtc_snapshot_due(&now))
    {
        write_powerdown_time(instance);
    }
}


// 上电运行期间 每秒调用 
// 事件 判断:  返回1:记录发生                0:不记录        -1:记录结束
int RTCErr_event_judge(void *instance, void *task, void *mesg)
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
