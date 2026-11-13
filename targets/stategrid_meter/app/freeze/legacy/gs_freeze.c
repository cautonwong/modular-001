#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "application/ports/resource_access.h"
#include "gs_record.h"
#include "fr_698.h"
#include "lib_ana.h"
#include "domain/freeze/schedule.h"
#include "domain/freeze/backfill.h"
#include "domain/freeze/sequence.h"
#include "domain/common/runtime.h"
#include "application/apps/app_config.h"
#include "domain/events/legacy/events_applications.h"
#include "domain/events/legacy/gs_record.h"
#include "domain/freeze/legacy/freeze_applications.h"
#include "domain/freeze/storage.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"
#define CONVERT_TM_MONTH(x)  ((x<1)?0:(x-1))

static void freeze_stored_datetime(const uint8_t* p, nd_datetime_t* out)
{
    out->second = p[0];
    out->minute = p[1];
    out->hour = p[2];
    out->day = p[3];
    out->month = (p[4] < 1) ? 1 : p[4];
    out->year = (uint16_t)(p[5] + 2000);
}

static void freeze_to_legacy_tm(const nd_datetime_t* in, struct tm* out)
{
    out->tm_sec = in->second;
    out->tm_min = in->minute;
    out->tm_hour = in->hour;
    out->tm_mday = in->day;
    out->tm_mon = in->month - 1;
    out->tm_year = in->year - 1900;
}

static void freeze_now_datetime(const struct tm* time, nd_datetime_t* out)
{
    out->second = time->tm_sec;
    out->minute = time->tm_min;
    out->hour = time->tm_hour;
    out->day = time->tm_mday;
    out->month = time->tm_mon + 1;
    out->year = (uint16_t)(time->tm_year + 1900);
}
//计算记录型配置关联对象时  总长度是否超预留空间，  入参关联对象、深度，  事件无深度，需传Null.
//并且计算保存分钟冻结8个方案中每个方案的空间大小偏移(扇区)
int cal_record_size_sectors(struct instance_t* instance, uint8_t *capture_ptr, uint8_t *depth_ptr, uint32_t oad)
{
    uint8_t scheme = 0,obj_num, scheme_num, tmpbuf[220] = {0};
    uint16_t i, k, recordID = 0;
    uint16_t sector_size[9];
    int16_t single_len = 0;
    uint32_t offset = 0, obj , addr_index = 0, sectorMax = 0, sector = 0;
    uint32_t depth = 0;
    struct record_storage_id_info_t record_id_info; 
    uint8_t device_type=0; // 0: e2p,    1: flash 
    int ret;
    
    obj_num = capture_ptr[0];


    scheme_num = 1;
    sectorMax = 0;
    device_type=0;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret < 0)
    {
        return -1;
    }

    recordID = (uint16_t)record_id_info.save_index;


    
    //获取冻结或事件的记录空间sectorMax  
    if (get_OAD_Class(oad) == 3) // event   
    {
        ret = supply_event_info(instance, recordID, &addr_index, &sectorMax, &depth);
        if (ret < 0)
        {
            return -1;
        }
        
    }
    else if (get_OAD_Class(oad) == 5)//freeze
    {
        switch (oad & 0xFFFF0000)
        {
            case 0x50020000:// 分钟  
                //supply_event_flash_1_info(instance, recordID, &addr_index, &sectorMax, &depth);
                //if (ret < 0)
                //{
                //    return -1;
                //}
                sectorMax = MinuteFreeze_SECTION_SIZE * 4096; 
                scheme_num = 8;
                device_type=1;
                break;
            case 0x50030000: // 小时
            case 0x50060000: // 月
            case 0x50040000: // 日
                ret = supply_event_flash_2_info(instance, recordID, &addr_index, &sectorMax, &depth);
                if (ret < 0)
                {
                    return -1;
                }
                device_type=1;
                break;
        default:
            ret = supply_event_info(instance, recordID, &addr_index, &sectorMax, &depth);
            if (ret < 0)
            {
                return -1;
            }
            break;
        }
    }
    else
    {
        return -1;
    }

    if (device_type == 1)
    {
        sectorMax /= 4096; //扇区数
    }
    

    for (i = 0; i < scheme_num; i++)
    {
        offset = 5 + 5;  // 序号 + 时间
        
        for (k = 0; k < obj_num; k++)
        {
            obj = buf_to_uint32_msb(&capture_ptr[4*k+1]);

            scheme = (uint8_t)((obj >> 13) & 0x07); //方案号

            if (scheme == i)
            {
                single_len = nd_size(instance,tmpbuf, obj);
                if (single_len > 0)
                {
                    ret = record_change_format(instance, NULL, NULL, obj, 0, RFC_GET_STORE_LEN);
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


        if (depth_ptr) //深度  有入参  用入参，无入参用默认
        {
            depth = buf_to_uint16_msb(&depth_ptr[2 * i]);
        }

        if (device_type == 1) // 存flash
        {
            sector_size[i] = sector;
        }

        if (depth != 0)
        {
            sector += nd_freeze_scheme_units(device_type == 1, offset, depth);
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
    uint32_t CaptureId, DepthId /*, oad = 0x50020200*/;
    struct record_storage_id_info_t  record_id_info;
    int ret = -1;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -1; 
    }

    CaptureId = record_id_info.capture_index;
    //PeriodId = record_id_info.period_index;
    DepthId = record_id_info.depth_index;
    //CaptureLen = record_id_info.capture_len;// 
    //PeriodLen = record_id_info.period_len;
    //DepthLen = record_id_info.depth_len;
    //maxnum = (CaptureLen - 1) / 4;

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

    ret = cal_record_size_sectors(instance, tempbuf, depth, oad);
    if (ret < 0)
    {
        return -1;
    }
    return 1;
}


int instant_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    nd_freeze_instant_t instant;

    uint16_t FlagID = InstantFreeze;
    uint8_t tmp[10] = {0};
    uint8_t payload[20] = {0};
    struct message_t mesg_tmp;


    if (ctrl_block->trigger_insfreeze_delay_sec == 0xFFFF)
    {
        return -1;
    }

    instant.remaining = ctrl_block->trigger_insfreeze_delay_sec;
    instant.armed = 1;

    if (!nd_freeze_instant_step(&instant))
    {
        ctrl_block->trigger_insfreeze_delay_sec = instant.remaining;
        return -1;
    }

    ctrl_block->trigger_insfreeze_delay_sec = 0xFFFF;

    mesg_tmp.id = FlagID;
    uint32_to_buf_msb(0x20210200, &payload[0]);
    payload[4] = 8; //data len
    payload[5] = 0x1C;//TYPE_date_time_s;
    rtc_format_change_from_tm(&instance->time,tmp);
    tmp[0] = 0; //sec = 0;
    mem_swap_cpy(&payload[6], &tmp[0], 7);
    recv_record_process(instance, NULL, &mesg_tmp, payload);


    //memcpy((uint8_t*)&record_time.tm_sec, (uint8_t*)&instance->time.tm_sec, sizeof(struct tm));
    //send_recordflag_mesg(instance, task,  (FlagID), NULL);
    return 1;
}



int rollback_sequence(struct instance_t* instance, uint32_t oad,  uint16_t rollback_num)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint32_t sequence = 0, s_index = 0;
    struct record_storage_id_info_t record_id_info; 
    struct circular_info_t circular_info = {0};
    uint16_t SeqId = 0;
    uint8_t tmpbuf[300];
    int ret;
    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret < 0)
    {
        return -(255);
    }

    SeqId = (uint16_t)record_id_info.sequence_index;

    if ((SeqId >= MinuteFreeze1_seq) && (SeqId <= MinuteFreeze8_seq))
    {
        sequence = nd_mbase_minute_freeze_seq(mbase_cb)[SeqId - MinuteFreeze1_seq];
    }
    else
    {
        nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
    }

    circular_info.start_mark = 0;
    circular_info.end_mark = 0xffffffffU;
    circular_info.mode =   0;
    ret = nd_log_output(instance, record_id_info.save_index,  tmpbuf, 1, &circular_info);

    sequence = nd_freeze_sequence_rollback(sequence, rollback_num,
                                           (uint8_t)(circular_info.valid_num != 0));

    if ((SeqId >= MinuteFreeze1_seq) && (SeqId <= MinuteFreeze8_seq))
    {
        nd_mbase_minute_freeze_seq(mbase_cb)[SeqId - MinuteFreeze1_seq] = sequence;

        s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
        nd_mbase_offset_minute_freeze_seq()) + ((SeqId - MinuteFreeze1_seq) * 4) -  nd_mbase_offset_persistent_section_start();
        nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_minute_freeze_seq(mbase_cb)[SeqId - MinuteFreeze1_seq], 4);
    }
    else
    {
        nd_set_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
    }

    return 1;
}

#if 1
//设置或修改时间后调用  冻结回退处理
//finish_check: 5A - 检查是否有未完成的回滚操作。 
//              其他- 回滚操作
int record_rollback_after_set_time(struct instance_t* instance, uint8_t finish_check)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    struct record_storage_id_info_t record_id_info; 
    uint8_t i, num = 0, buffer[10] = { 0xFF };

    uint32_t addr,last_timesec, curr_timesec;
    uint16_t rollback_seq_num[11] = { 0 };
    int ret = 0;
    //uint16_t FlagID[] = { MinuteFreeze1, MinuteFreeze2, MinuteFreeze3, MinuteFreeze4, MinuteFreeze5, MinuteFreeze6, MinuteFreeze7, MinuteFreeze8, HourFreeze, DayFreeze, MonthFreeze, };
    uint32_t TabOAD[11] = { 0x50020200, 0x50022200, 0x50024200, 0x50026200, 0x50028200, 0x5002A200, 0x5002C200, 0x5002E200, 0x50030200, 0x50040200, 0x50060200, };


    if (finish_check == 0x5A)
    {
        if (nd_mbase_record_rollback_flag(mbase_cb) != 0x5A)
        {
            return 1;
        }
    }

    //时间合法性判断 待调整
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
    ret = nd_get_index(instance, addr, buffer, 7);
    if (ret < 0)
    {
        return -1;
    }

    last_timesec = dt_buf_to_second(buffer);

    //时间合法性判断 待调整
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
    ret = nd_get_index(instance, addr, buffer, 7);
    if (ret < 0)
    {
        return -1;
    }

    curr_timesec = dt_buf_to_second(buffer);

    if (nd_freeze_rollback_required(last_timesec, curr_timesec))
    {
        // 非检查 情况下 保存回退标志 
        if (finish_check != 0x5A)
        {
            nd_mbase_record_rollback_flag(mbase_cb) = 0x5A;
            
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
            nd_mbase_offset_record_rollback_flag()) -  nd_mbase_offset_persistent_section_start();
            nd_set_offset(instance, addr, (uint8_t*)&nd_mbase_record_rollback_flag(mbase_cb), sizeof(nd_mbase_record_rollback_flag(mbase_cb)));
        }
        
        num = 0;
        for (i = 0; i < (sizeof(TabOAD) / sizeof(TabOAD[0])); i++)
        {
            //掉电退出
            if (ANA_GetStatus(ANA_STATUS_VDCINDROP))    
            {
                return 1;
            }
            nd_runtime_watchdog_clear();
            ret = get_record_id_info(instance, TabOAD[i], &record_id_info);
            if (ret <= 0)
            {
                continue;
            }
            
            rollback_seq_num[i] = nd_log_append_rollback(instance, record_id_info.save_index  , public_block_flash, last_timesec, curr_timesec);
            if (rollback_seq_num[i] > 0)
            {
                rollback_sequence(instance, TabOAD[i], rollback_seq_num[i]);
                num++;
            }
        }

        if (i >= (sizeof(TabOAD) / sizeof(TabOAD[0])))
        {
            //清除回退标志
            nd_mbase_record_rollback_flag(mbase_cb) = 0;
            
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
            nd_mbase_offset_record_rollback_flag()) -  nd_mbase_offset_persistent_section_start();
            nd_set_offset(instance, addr, (uint8_t*)&nd_mbase_record_rollback_flag(mbase_cb), sizeof(nd_mbase_record_rollback_flag(mbase_cb)));
        }
    }

    return 1;
}
#endif
int minute_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    uint8_t i, period[24];
    uint32_t index;
    nd_datetime_t now;
    uint16_t PeriodID = MinuteFreeze_period;
    uint16_t FlagID = MinuteFreeze1;
    struct message_t mesg_tmp;

    if (instance->time.tm_sec != 0)
    {
        return -1;
    }
    memset((uint8_t *)&mesg_tmp, 0, sizeof(struct message_t));

    // 周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    freeze_now_datetime(&instance->time, &now);

    for (i = 0; i < 8; i++)
    {
        nd_freeze_period_t freeze_period;

        freeze_period.valid = period[3 * i];
        freeze_period.interval = buf_to_uint16_msb(&period[3 * i + 1]);

        if (nd_freeze_period_due(&now, &freeze_period, 1))
        {
            mesg_tmp.id = FlagID + i;
            recv_record_process(instance, NULL, &mesg_tmp, NULL);
        }
    }
    return 1;
}
int hour_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    uint8_t period[24];
    uint32_t index;
    nd_datetime_t now;
    nd_freeze_period_t freeze_period;
    uint16_t PeriodID = HourFreeze_period;
    uint16_t FlagID = HourFreeze;
    struct message_t mesg_tmp;



    if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0))
    {
        return -1;
    }
    memset((uint8_t *)&mesg_tmp, 0, sizeof(struct message_t));
    // 周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    freeze_now_datetime(&instance->time, &now);
    freeze_period.valid = period[0];
    freeze_period.interval = buf_to_uint16_msb(&period[1]);

    if (nd_freeze_period_due(&now, &freeze_period, 60))
    {
        mesg_tmp.id = FlagID;
        recv_record_process(instance, NULL, &mesg_tmp, NULL);
    }

    return 1;
}

int day_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    uint8_t period[24]= {0}, tmp[10] = {0};
    uint32_t index;
    nd_datetime_t now;
    nd_freeze_period_t freeze_period;
    uint16_t PeriodID = DayFreeze_period;
    uint16_t FlagID = DayFreeze;
    struct message_t mesg_tmp;

    if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0) || (instance->time.tm_hour != 0))
    {
        return -1;
    }
    memset((uint8_t *)&mesg_tmp, 0, sizeof(struct message_t));

    //读最近一次冻结时间 比较当前时间记录是否已存在   待添加
    //待添加

    // 周期
    index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    nd_get_index(instance, index, &period[0], sizeof(period));

    freeze_now_datetime(&instance->time, &now);
    freeze_period.valid = period[0];
    freeze_period.interval = buf_to_uint16_msb(&period[1]);

    if (nd_freeze_period_due(&now, &freeze_period, 60*24))
    {
            //保存 结算时间
        tmp[0] = instance->time.tm_sec ;
        tmp[1] = instance->time.tm_min ;
        tmp[2] = instance->time.tm_hour;
        tmp[3] = instance->time.tm_mday;
        tmp[4] = instance->time.tm_mon + 1;
        tmp[5] = (uint8_t)((instance->time.tm_year + 1900)-2000);

        index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
        nd_set_index(instance, index, &tmp[0], 6);

        //send_recordflag_mesg(instance, task,  FlagID, NULL);
        mesg_tmp.id = FlagID;
        recv_record_process(instance, NULL, &mesg_tmp, NULL);
    }

    return 1;
}

//月冻结
int month_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    nd_datetime_t now;
    uint16_t FlagID = MonthFreeze;
    struct message_t mesg_tmp;

    freeze_now_datetime(&instance->time, &now);
    if (!nd_freeze_month_due(&now))
    {
        return -1;
    }
    memset((uint8_t *)&mesg_tmp, 0, sizeof(struct message_t));

    //读最近一次冻结时间 比较当前时间记录是否已存在   待添加
    //待添加

    // 周期
    //index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodID);
    //nd_get_index(instance, index, &period[0], sizeof(period));

    //if (1 == freeze_judge(instance, &period[0], 60 * 24, &record_time))
    {
        //send_recordflag_mesg(instance, task,  FlagID, NULL);
        mesg_tmp.id = FlagID;
        recv_record_process(instance, NULL, &mesg_tmp, NULL);
    }

    return 1;
}

#if 1


//上电日冻结补冻结处理
int day_freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{

    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    uint8_t tmp[10];
    struct tm pd_tm;
    nd_datetime_t now_dt, pd_dt, last_dt;
    nd_datetime_t dates[ND_FREEZE_DAY_CATCH_UP_MAX];
    uint32_t s_index, tmp_u32;
    uint8_t payload[20] = {0};
    //uint8_t *p = payload;
    struct message_t mesg_tmp;
    uint16_t    period;
    uint8_t count, i;
    int ret;

    //获取周期 天
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + DayFreeze_period);
    nd_get_index(instance, s_index, &tmp[0], 3);
    
    period =  buf_to_uint16_msb(&tmp[1]);
    if (period == 0)
    {
        period = 1;
    }

    ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->v_powerdown_tm), sizeof(ctrl_block->v_powerdown_tm));

    if (false == rtc_tm_valid(&ctrl_block->v_powerdown_tm))    // 模拟停电时间无效          取上次停电时间
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PDTime_Frez); // 冻结停电时间
        ret = nd_get_index(instance, s_index, (uint8_t *)&tmp_u32, 4);
        rtc_localtime(tmp_u32, &pd_tm);

        if ((ret < 0) || (tmp_u32 == 0) || (false == rtc_tm_valid(&pd_tm)))   // 冻结停电时间无效  则使用电表停电时间
        {
            if (false == rtc_tm_valid(&ctrl_block->powerdown_tm))   // 电表停电时间无效       退出
            {
                return 0;
            }
            ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->powerdown_tm), sizeof(ctrl_block->powerdown_tm));
        }
    }

    freeze_now_datetime(&instance->time, &now_dt);
    freeze_now_datetime(&pd_tm, &pd_dt);

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
    nd_get_index(instance, s_index, &tmp[0], 6);
    freeze_stored_datetime(tmp, &last_dt);

    count = nd_freeze_day_backfill(&now_dt, &pd_dt, &last_dt, period,
                                   dates, ND_FREEZE_DAY_CATCH_UP_MAX);

    for (i = 0; i < count; i++)
    {
        struct tm record_tm;

        freeze_to_legacy_tm(&dates[i], &record_tm);

        // 刷新最近一次时间
        tmp[0] = dates[i].second;
        tmp[1] = dates[i].minute;
        tmp[2] = dates[i].hour;
        tmp[3] = dates[i].day;
        tmp[4] = dates[i].month;
        tmp[5] = (uint8_t)(dates[i].year - 2000);

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
        nd_set_index(instance, s_index, &tmp[0], 6);

        // 本条记录存储的时间
        mesg_tmp.id = DayFreeze;
        uint32_to_buf_msb(0x20210200, &payload[0]);
        payload[4] = 8; //data len
        payload[5] = 28;//TYPE_date_time_s;
        rtc_format_change_from_tm(&record_tm,tmp);
        mem_swap_cpy(&payload[6], &tmp[0], 7);
        recv_record_process(instance, task, &mesg_tmp, payload);
        nd_runtime_watchdog_clear();//清看门狗
    }


    return 1;
}



//  上电补结算冻结处理           上电调用 
int bill_freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    uint8_t tmp[10];
    struct tm pd_tm;
    nd_datetime_t now_dt, pd_dt, last_dt;
    nd_freeze_bill_date_t bill_dates[ND_FREEZE_BILL_DATES];
    nd_freeze_bill_event_t events[ND_FREEZE_BILL_DATES * ND_FREEZE_BILL_CATCH_UP_MONTHS];
    uint32_t s_index, tmp_u32;
    uint8_t payload[20] = {0};
    //uint8_t *p = payload;
    struct message_t mesg_tmp;
    uint8_t count, i;
    int ret = 0;

    ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->v_powerdown_tm), sizeof(ctrl_block->v_powerdown_tm));

    if (false == rtc_tm_valid(&ctrl_block->v_powerdown_tm))    // 模拟停电时间无效          取上次停电时间
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PDTime_Frez); // 冻结停电时间
        ret = nd_get_index(instance, s_index, (uint8_t *)&tmp_u32, 4);
        rtc_localtime(tmp_u32, &pd_tm);
        if ((ret < 0) || (tmp_u32 == 0)  || (false == rtc_tm_valid(&pd_tm)))   // 冻结停电时间无效  则使用电表停电时间
        {
            if (false == rtc_tm_valid(&ctrl_block->powerdown_tm))   // 电表停电时间无效       退出
            {
                return 0;
            }
            ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->powerdown_tm), sizeof(ctrl_block->powerdown_tm));
        }
    }

    freeze_now_datetime(&instance->time, &now_dt);
    freeze_now_datetime(&pd_tm, &pd_dt);

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillingTime);
    nd_get_index(instance, s_index, &tmp[0], 6);
    freeze_stored_datetime(tmp, &last_dt);

    for(i=0;i<ND_FREEZE_BILL_DATES;i++)
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
        nd_get_index(instance, s_index, tmp, 2);
        bill_dates[i].day = tmp[0];
        bill_dates[i].hour = tmp[1];
    }

    count = nd_freeze_bill_backfill(&now_dt, &pd_dt, &last_dt, bill_dates, events,
                                    ND_FREEZE_BILL_DATES * ND_FREEZE_BILL_CATCH_UP_MONTHS);

    for (i = 0; i < count; i++)
    {
        struct tm record_tm;

        freeze_to_legacy_tm(&events[i].when, &record_tm);

        if (events[i].slot == 0)
        {
            ctrl_block->first_MbillDay_flag = 0x5A;
        }

        // 刷新最近一次结算时间
        tmp[0] = events[i].when.second;
        tmp[1] = events[i].when.minute;
        tmp[2] = events[i].when.hour;
        tmp[3] = events[i].when.day;
        tmp[4] = events[i].when.month;
        tmp[5] = (uint8_t)(events[i].when.year - 2000);

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillingTime);
        nd_set_index(instance, s_index, &tmp[0], 6);

        // 本条记录存储的时间
        mesg_tmp.id = BillFreeze;
        uint32_to_buf_msb(0x20210200, &payload[0]);
        payload[4] = 8; //data len
        payload[5] = 28;//TYPE_date_time_s;
        rtc_format_change_from_tm(&record_tm,tmp);
        mem_swap_cpy(&payload[6], &tmp[0], 7);
         // 记录1次结算       需根据i判断需量结算是否填空，
        // 第1结算日过后，最大需量清零                是否需要发送消息？             待添加
        recv_record_process(instance, task, &mesg_tmp, payload);
        nd_runtime_watchdog_clear();
    }

    return 0;
}

#endif

//结算冻结
int bill_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
        struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
        uint32_t s_index;
        nd_datetime_t now, last_bill;
        uint8_t tmp[20];
        uint8_t billdate[3][2];
        uint16_t LastBillTimeID = LastBillingTime;
        uint16_t FlagID = BillFreeze;
        uint8_t  i;
        struct message_t mesg_tmp;

        if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0))
        {
            return -1;
        }
        memset((uint8_t *)&mesg_tmp, 0, sizeof(struct message_t));

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillTimeID);
        nd_get_index(instance, s_index, &tmp[0], 6);

        last_bill.second = tmp[0];
        last_bill.minute = tmp[1];
        last_bill.hour = tmp[2];
        last_bill.day = tmp[3];
        last_bill.month = (tmp[4] < 1) ? 1 : tmp[4];
        last_bill.year = (uint16_t)(tmp[5] + 2000);

        for(i=0;i<3;i++)
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
            nd_get_index(instance, s_index, tmp, 2);
            billdate[i][0] = tmp[0];
            billdate[i][1] = tmp[1];
        }

        freeze_now_datetime(&instance->time, &now);

        ctrl_block->first_MbillDay_flag = 0;
        for (i = 0; i < 3; i++)
        {
            nd_freeze_bill_date_t bill;

            bill.day = billdate[i][0];
            bill.hour = billdate[i][1];

            if (!nd_freeze_bill_due(&now, &last_bill, &bill))
            {
                continue;
            }

            //第二、第三结算日时需量需要清成FF
            if (i == 0x00)
            {
                ctrl_block->first_MbillDay_flag = 0x5A;
            }

            //保存 结算时间
            tmp[0] = now.second;
            tmp[1] = now.minute;
            tmp[2] = now.hour;
            tmp[3] = now.day;
            tmp[4] = now.month;
            tmp[5] = (uint8_t)(now.year - 2000);

            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillTimeID);
            nd_set_index(instance, s_index, &tmp[0], 6);

            mesg_tmp.id = FlagID;
            recv_record_process(instance, NULL, &mesg_tmp, NULL);
            last_bill = now;
        }

        return false;

}



#if (FREEZ_TEST_OVERTURN)
uint8_t test_freeze = 0;
#endif
// 每秒调用
int freeze_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
#if RESET_INFO    
        //复位记录测试代码
        if (instance->reset_flag == 0x5A)
        {
            
            send_event_mesg_pro(instance, NULL, StepCutFreeze, NULL, 1);
        }
#endif   



#if (FREEZ_TEST_OVERTURN)   
    if (test_freeze)
    {
    
        send_recordflag_mesg(instance, task,  MinuteFreeze1, NULL);
        send_recordflag_mesg(instance, task,  HourFreeze, NULL);
        send_recordflag_mesg(instance, task,  DayFreeze, NULL);          
        //send_recordflag_mesg(instance, task,  MonthFreeze, NULL);           
        //send_recordflag_mesg(instance, task,  BillFreeze, NULL);   
        
        //send_recordflag_mesg(instance, task,  SeasonTabCutFreeze, NULL);   
        //send_recordflag_mesg(instance, task,  DayTabCutFreeze, NULL);   
        //send_recordflag_mesg(instance, task,  InstantFreeze, NULL);   
    }
    else
#endif    
    {
    
        record_rollback_after_set_time(instance, 0x5A);//冻结回滚检查


#if(UPGRADE_MODE == UPGRADE_MODE_LOCAL)
        if (ctrl_block->frz_min_start_dlysec == 0)
#endif
        {
            minute_freeze_judge(instance, task, mesg); //分钟冻结
        }   
        
        hour_freeze_judge(instance, task, mesg);   //小时冻结
        day_freeze_judge(instance, task, mesg);    //日冻结
        month_freeze_judge(instance, task, mesg);  //月冻结
        instant_freeze_judge(instance, task, mesg); //瞬时冻结
        bill_freeze_judge(instance, task, mesg);   //结算冻结

    }

    return 1;
}

int recover_minFreeze_sequence(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t sequence = 0;
    uint32_t oad = 0x50020200;
    uint8_t buf[20] = {0};
    int ret = 0;
    uint8_t i;


    for(i = 0; i < 8; i++)
    {    
        sequence = 0;
        ret = read_record_data_form_698_protocol(instance, buf, (oad), (0x20230200 + (i<<13)),  1, sizeof(buf));
        if(ret > 0)
        {
            sequence = buf_to_uint32_msb(buf + 1 + CIRCULAR_OFFSET_SEQUENCE);
            nd_mbase_minute_freeze_seq(mbase_cb)[i] =
                nd_freeze_sequence_recover(nd_mbase_minute_freeze_seq(mbase_cb)[i], sequence);
        }
    }
    return 0;
}
// 上电调用
int freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t tmp_u32 = 0, s_index;
    
    ctrl_block->trigger_insfreeze_delay_sec = 0xFFFF; 
    
    record_rollback_after_set_time(instance, 0x5A);

    if (instance->reset_flag == 0x5A)
    {
        recover_minFreeze_sequence(instance); //分钟冻结序号检查
    }

    day_freeze_power_on_pro(instance, task, mesg);
    bill_freeze_power_on_pro(instance, task, mesg);
    ctrl_block->frz_replenish_end = 0x5A; //完成补冻结
    
    tmp_u32 = 0;
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PDTime_Frez); // 清冻结停电时间
    nd_set_index(instance, s_index, (uint8_t *)&tmp_u32, 4);
    
    memset((uint8_t *)(&ctrl_block->v_powerdown_tm), 0 , sizeof(ctrl_block->v_powerdown_tm));
    return 1;
}


