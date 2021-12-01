#include "nidavellir.h"
#include "applications.h"

extern void set_recordflag(struct pipeline_t* task, uint16_t RecordId);


void send_recordflag_mesg(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg, uint16_t FlagId, uint8_t* payload)
{

    mesg->from = kernel;
    mesg->to = pid_resource_manager;
    mesg->ctrl = 0;
    mesg->qos = 0;
    mesg->len = 4;
    mesg->id = FlagId;
    mesg->oad_n = 1;

    set_recordflag(task, mesg->id);

    send_mesg(instance, NULL, mesg, payload);
}



int freeze_judge(struct instance_t* instance, uint8_t* pperiod, uint16_t unit, struct tm *record_time)
{
    uint8_t valid;
    uint16_t timeinv = 0;
    uint32_t min, value;
    

    if (instance->time.tm_sec != 0)
    {
        return -1;
    }

    min = rtc_mktime(&instance->time);
    min = min / 60;
    valid = pperiod[0];
    memcpy((uint8_t*)&timeinv, (pperiod+1), 0x02);

    value = (uint32_t)timeinv * unit;

    if (valid && value)
    {
        if (0x00 == (min % value))
        {
            if (record_time != NULL) // 拷贝当前时间
            {
                memcpy((uint8_t*)&record_time->tm_sec, (uint8_t*)&instance->time.tm_sec, sizeof(struct tm));
            }
            return 1;
        }
    }
    return -1;

}


int minute_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t i, period[24];
    uint16_t timeinv = 0;
    uint32_t index;
    struct tm record_time;
    uint16_t PeriodID = MinuteFreeze_period;
    uint16_t FlagID = MinuteFreeze1;

    if (instance->time.tm_sec != 0)
    {
        return -1;
    }
    // 周期
    index = ((RES_DEV_EEPROM << 24) + (0x09 << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    for (i = 0; i < 8; i++)
    {
        if (1 == freeze_judge(instance, &period[3 * i], 1, &record_time))
        {
            send_recordflag_mesg(instance, task, mesg, (FlagID + i), NULL);
        }
    }
    return 1;
}
int hour_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t i, period[24];
    uint16_t timeinv = 0;
    uint32_t index;
    struct tm record_time;
    uint16_t PeriodID = HourFreeze_period;
    uint16_t FlagID = HourFreeze;

    if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0))
    {
        return -1;
    }
    // 周期
    index = ((RES_DEV_EEPROM << 24) + (0x09 << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    if (1 == freeze_judge(instance, &period[0], 60, &record_time))
    {
        send_recordflag_mesg(instance, task, mesg, FlagID, NULL);
    }

    return 1;
}



// 每秒调用
int freeze_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    minute_freeze_judge(instance, task, mesg);
    hour_freeze_judge(instance, task, mesg);
}




