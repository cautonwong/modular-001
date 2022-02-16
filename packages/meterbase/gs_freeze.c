#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"

//extern void set_recordflag(struct pipeline_t* task, uint16_t RecordId);

//计算分钟、小时、日、月冻结 总长度是否超预留空间， 修改的关联对象时调用，  入参关联对象、深度，  
//并且计算保存分钟冻结0-7  8个方案中每个方案的空间大小偏移(扇区)
int cal_freeze_size_sectors(struct instance_t* instance, uint8_t *capture_ptr, uint8_t *depth_ptr, uint32_t oad)
{
    uint8_t scheme = 0,obj_num, scheme_num;
    uint16_t i, k, depth, sector = 0, sectorMax = 0, t_num = 0;
    uint16_t sector_size[9];
    int16_t single_len = 0;
    uint32_t offset = 0, obj , addr_index;
    int ret;
    
    obj_num = capture_ptr[0];


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

            if (scheme == i)
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
                    single_len = 1;
                }
                offset += single_len;
            }
        }
        
        sector_size[i] = sector;

        depth = buf_to_uint16_msb(&depth_ptr[2 * i]);
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


//分冻结 各方案空间分配  恢复出厂参数时，分钟冻结必须调用 
int freez_size_sectors_init(struct instance_t* instance, uint32_t oad)
{
    uint8_t tempbuf[200], depth[1 + 2 * 15];
    uint8_t i, num, maxnum, scheme;
    uint16_t OI, CaptureLen, PeriodLen, DepthLen;
    uint32_t CaptureId, PeriodId, DepthId /*, oad = 0x50020200*/;
    struct record_storage_id_info_t  record_id_info;
    int ret = -1;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -1;
    }

    CaptureId = record_id_info.capture_index;
    PeriodId = record_id_info.period_index;
    DepthId = record_id_info.depth_index;
    CaptureLen = record_id_info.capture_len;// 
    PeriodLen = record_id_info.period_len;
    DepthLen = record_id_info.depth_len;
    maxnum = (CaptureLen - 1) / 4;

    //获取捕获对象
    ret = nd_get_index(instance, CaptureId, &tempbuf[0], sizeof(tempbuf));
    if (ret < 0)
    {
        return -1;
    }
    // 深度
    ret = nd_get_index(instance, DepthId, &depth[0], sizeof(depth));
    if (ret < 0)
    {
        return -1;
    }

    ret = cal_freeze_size_sectors(instance, tempbuf, depth, oad);
    if (ret < 0)
    {
        return -1;
    }
    return 1;
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

void rtc_normal_to_tm(struct tm* time, uint8_t* p)
{
    uint16_t temp=0;

    time->tm_sec = p[6];
    time->tm_min = p[5];
    time->tm_hour = p[4];
    time->tm_mday = p[3];
    time->tm_mon = p[2] - 1;    // 待调整  后续增加时间合法性判断
    mem_swap_cpy((uint8_t*)&temp, (uint8_t*)(p), 2);
    time->tm_year = temp - 1900;
}


//设置或修改时间后调用  冻结回退处理
int record_rollback_after_set_time(struct instance_t* instance)
{
    struct record_storage_id_info_t record_id_info; 
    uint8_t i, num = 0, buffer[10] = { 0xFF };
    struct tm t_before, t_after;
    uint32_t addr,last_timesec, curr_timesec;
    int ret = 0;
    //uint16_t FlagID[] = { MinuteFreeze1, MinuteFreeze2, MinuteFreeze3, MinuteFreeze4, MinuteFreeze5, MinuteFreeze6, MinuteFreeze7, MinuteFreeze8, HourFreeze, DayFreeze, MonthFreeze, };
    uint32_t TabOAD[] = { 0x50020200, 0x50022200, 0x50024200, 0x50026200, 0x50028200, 0x5002A200, 0x5002C200, 0x5002E200, 0x50030200, 0x50040200, 0x50060200, };


    //时间合法性判断 待调整
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
    nd_get_index(instance, addr, buffer, 7);
    //rtc_normal_to_tm(&t_before, buffer);
    //last_timesec = rtc_mktime(&t_before);
    last_timesec = dt_buf_to_second(buffer);

    //时间合法性判断 待调整
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
    nd_get_index(instance, addr, buffer, 7);
    //rtc_normal_to_tm(&t_after, buffer);
    //curr_timesec = rtc_mktime(&t_after);
    curr_timesec = dt_buf_to_second(buffer);

    if (curr_timesec < last_timesec)
    {
        // 保存回退相关数据和标识  待调整

        num = 0;
        for (i = 0; i < (sizeof(TabOAD) / sizeof(TabOAD[0])); i++)
        {
            //掉电退出 待调整
            //喂狗 待调整
            ret = get_record_id_info(instance, TabOAD[i], &record_id_info);
            if (ret <= 0)
            {
                continue;
            }
            
            ret = nd_log_append_rollback(instance, record_id_info.save_index  , public_block_flash, last_timesec, curr_timesec);
            if (ret > 0)
            {
                num++;
                //需要分每个id单独记录
            }
        }

        if (num == (sizeof(TabOAD) / sizeof(TabOAD[0])))
        {
            //清除回退相关数据和标识  待调整
        }
    }

    return 1;
}
//设置或修改时间后 因停电或其他情况导致 回退未完成，需要补处理    待调整 未调用
int record_rollback_finish_check(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)task->ctx;
    uint8_t i, num = 0, buffer[10] = { 0xFF };
    struct tm t_before, t_after;
    uint32_t addr, last_timesec, curr_timesec;
    int ret = 0;
    //uint16_t FlagID[] = { MinuteFreeze1, MinuteFreeze2, MinuteFreeze3, MinuteFreeze4, MinuteFreeze5, MinuteFreeze6, MinuteFreeze7, MinuteFreeze8, HourFreeze, DayFreeze, MonthFreeze, };
    uint32_t TabOAD[] = { 0x50020200, 0x50022200, 0x50024200, 0x50026200, 0x50028200, 0x5002A200, 0x5002C200, 0x5002E200, 0x50030200, 0x50040200, 0x50060200, };

    //读取回退是否完成标志
  

    return 1;
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
    //freeze_rollback(instance, task, mesg);

    minute_freeze_judge(instance, task, mesg);
    hour_freeze_judge(instance, task, mesg);
    day_freeze_judge(instance, task, mesg);
    month_freeze_judge(instance, task, mesg);  
    instant_freeze_judge(instance, task, mesg); //瞬时冻结
    return 1;
}




