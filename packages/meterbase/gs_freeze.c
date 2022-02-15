#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"

//extern void set_recordflag(struct pipeline_t* task, uint16_t RecordId);

//计算分钟、小时、日、月冻结 总长度是否超预留空间， 修改的关联对象时调用，  入参关联对象、深度，  
//并且计算保存分钟冻结0-7  8个方案中每个方案的空间大小偏移(扇区)
int cal_freeze_size_sectors(struct instance_t* instance, uint8_t *capture_ptr, uint8_t *depth_ptr, uint32_t oad)
{
    uint8_t maxnum,scheme = 0,obj_num, scheme_num;
    uint16_t i, k, depth, sector = 0, sectorMax = 0, t_num = 0;
    uint16_t sector_size[9];
    int16_t single_len = 0;
    uint32_t offset = 0, obj , addr_index;
    int ret;
    
    obj_num = capture_ptr[0];
    if (obj_num > maxnum)
    {
        return -1;
    }

    scheme_num = 1;
    sectorMax = 0;
    switch (oad & 0xFFFF0000)
    {
        case 0x50020000:// 分钟
            sectorMax = 830;
            scheme_num = 8;
            break;
        case 0x50030000: // 小时
        case 0x50060000: // 月
            sectorMax = 5;
            break;
        case 0x50040000: // 日
            sectorMax = 27;
            break;
    default:
        break;
    }

    for (i = 0; i < scheme_num; i++)
    {
        offset = 5 + 5;  // 序号 + 时间

        for (k = 0; k < obj_num; k++)
        {
            obj = buf_to_uint32_msb(&capture_ptr[4*k+1]);//待调整 

            scheme = (uint8_t)((obj >> 13) & 0x07); //方案号

            if (scheme == k)
            {
                single_len = nd_size(instance, obj);
                if (single_len > 0)
                {
                    ret = record_change_format(NULL, NULL, obj, RFC_GET_STORE_LEN);
                    if (ret >= 0)
                    {
                        single_len = ret;
                    }
                }
                else
                {
                    return -1;
                }
                offset += single_len;
            }
        }
        
        sector_size[i] = sector;

        depth = buf_to_uint16_msb(&depth_ptr[2 * k]);
        if (depth != 0)
        {
            t_num = 0x1000 / offset; //一个扇区多少条
            t_num = (depth % t_num) ? (depth / t_num + 1) : (depth / t_num);
            sector += 0x01 + t_num; //预留1个扇区
        }

    }
    
    sector_size[i] = sector;

    if (sector <= sectorMax)
    {
        if ((oad & 0xFFFF0000) == 0x50020000)
        {
            addr_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + MinFrez_Sectors);
            nd_set_index(instance, addr_index, (uint8_t*)&sector_size[0], 18);
        }

        return 1;
    }

    return -1;
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
    mem_swap_cpy((uint8_t*)&timeinv, (pperiod+1), 0x02);

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

int instant_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    struct tm record_time;

    uint16_t FlagID = InstantFreeze;


    if (ctrl_block->trigger_insfreeze_delay_sec == 0xFFFF)
    {
        return -1;
    }

    if (ctrl_block->trigger_insfreeze_delay_sec > 0)
    {
        ctrl_block->trigger_insfreeze_delay_sec--;
    }

    if (ctrl_block->trigger_insfreeze_delay_sec == 0)
    {
        ctrl_block->trigger_insfreeze_delay_sec = 0xFFFF;
        memcpy((uint8_t*)&record_time.tm_sec, (uint8_t*)&instance->time.tm_sec, sizeof(struct tm));
        send_recordflag_mesg(instance, task,  (FlagID), NULL);
        return 1;
    }

    return -1;
}



int minute_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t i, period[24];
    uint32_t index;
    struct tm record_time;
    uint16_t PeriodID = MinuteFreeze_period;
    uint16_t FlagID = MinuteFreeze1;

    if (instance->time.tm_sec != 0)
    {
        return -1;
    }
    // 周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    for (i = 0; i < 8; i++)
    {
        if (1 == freeze_judge(instance, &period[3 * i], 1, &record_time))
        {
            send_recordflag_mesg(instance, task,  (FlagID + i), NULL);
        }
    }
    return 1;
}
int hour_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t period[24];
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
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    if (1 == freeze_judge(instance, &period[0], 60, &record_time))
    {
        send_recordflag_mesg(instance, task,  FlagID, NULL);
    }

    return 1;
}

int day_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t period[24];
    uint32_t index;
    struct tm record_time;
    uint16_t PeriodID = DayFreeze_period;
    uint16_t FlagID = DayFreeze;

    if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0) || (instance->time.tm_hour != 0))
    {
        return -1;
    }

    //读最近一次冻结时间 比较当前时间记录是否已存在   待添加
    //待添加

    // 周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    if (1 == freeze_judge(instance, &period[0], 60*24, &record_time))
    {
        send_recordflag_mesg(instance, task,  FlagID, NULL);
    }

    return 1;
}

int month_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t period[24];
    uint32_t index;
    struct tm record_time;
    uint16_t PeriodID = MonthFreeze_period;
    uint16_t FlagID = MonthFreeze;

    if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0) || (instance->time.tm_hour != 0))
    {
        return -1;
    }

    //读最近一次冻结时间 比较当前时间记录是否已存在   待添加
    //待添加

    // 周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    if (1 == freeze_judge(instance, &period[0], 60 * 24, &record_time))
    {
        send_recordflag_mesg(instance, task,  FlagID, NULL);
    }

    return 1;
}

// 每秒调用
int freeze_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    minute_freeze_judge(instance, task, mesg);
    hour_freeze_judge(instance, task, mesg);
    day_freeze_judge(instance, task, mesg);
    month_freeze_judge(instance, task, mesg);  
    instant_freeze_judge(instance, task, mesg); //瞬时冻结
}




