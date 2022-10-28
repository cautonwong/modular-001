#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"
#include "fr_698.h"
#include "lib_ana.h"
#define CONVERT_TM_MONTH(x)  ((x<1)?0:(x-1))
//计算记录型配置关联对象时  总长度是否超预留空间，  入参关联对象、深度，  事件无深度，需传Null.
//并且计算保存分钟冻结8个方案中每个方案的空间大小偏移(扇区)
int cal_record_size_sectors(struct instance_t* instance, uint8_t *capture_ptr, uint8_t *depth_ptr, uint32_t oad)
{
    uint8_t scheme = 0,obj_num, scheme_num, tmpbuf[220] = {0};
    uint16_t i, k,  t_num = 0 , recordID = 0;
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
                supply_event_flash_2_info(instance, recordID, &addr_index, &sectorMax, &depth);
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
            if (depth != 0)
            {
                t_num = 0x1000 / offset; //一个扇区多少条
                t_num = (depth % t_num) ? (depth / t_num + 1) : (depth / t_num);
                sector += 0x01 + t_num; //预留1个扇区
                
            }
        }
        else // 存e2p   byte num
        {
            if (depth != 0)
            {
                sector = offset * depth + 1; //预留1个字节
            }
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


int freeze_judge(struct instance_t* instance, uint8_t* pperiod, uint16_t unit, struct tm *record_time)
{
    uint8_t valid;
    uint16_t timeinv = 0;
    uint32_t min, value, ref_min;
    struct tm ref_time;//

    if (instance->time.tm_sec != 0)
    {
        return -1;
    }

    //2000年1月1日0时0分
    ref_time.tm_year = 0 + 2000 - 1900;;
    ref_time.tm_mon = 1 - 1;
    ref_time.tm_mday = 1;
    ref_time.tm_hour = 0;
    ref_time.tm_min = 0;
    ref_time.tm_sec = 0;

    ref_min = rtc_mktime(&ref_time);
    min = rtc_mktime(&instance->time);
    if (min >= ref_min)
    {
        min -= ref_min;
    }
    
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
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct tm record_time;

    uint16_t FlagID = InstantFreeze;
    uint8_t tmp[10] = {0}; 
    uint8_t payload[20] = {0};
    struct message_t mesg_tmp;


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

    return -1;
}

void rtc_normal_to_tm(struct tm* time, uint8_t* p)
{
    uint16_t temp=0;

    time->tm_sec = p[6];
    time->tm_min = p[5];
    time->tm_hour = p[4];
    time->tm_mday = p[3];
    if(p[2])
    {
        time->tm_mon = p[2] - 1;   
    }
    else
    {
        time->tm_mon = 0;    
    }
    
    mem_swap_cpy((uint8_t*)&temp, (uint8_t*)(p), 2);
    time->tm_year = temp - 1900;
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
        sequence = mbase_cb->minute_freeze_seq[SeqId - MinuteFreeze1_seq];
    }
    else
    {
        nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
    }

    circular_info.start_mark = 0;
    circular_info.end_mark = 0xffffffffU;
    circular_info.mode =   0;
    ret = nd_log_output(instance, record_id_info.save_index,  tmpbuf, 1, &circular_info);

    if ((circular_info.valid_num == 0))
    {
        sequence = 0;
    }

    if (sequence <  rollback_num)
    {
        sequence = 0;
    }
    else
    {
        sequence -= rollback_num;
    }

    if ((SeqId >= MinuteFreeze1_seq) && (SeqId <= MinuteFreeze8_seq))
    {
        mbase_cb->minute_freeze_seq[SeqId - MinuteFreeze1_seq] = sequence;

        s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
        offsetof(struct mbase_control_block_t, minute_freeze_seq[0])) + ((SeqId - MinuteFreeze1_seq) * 4) -  offsetof(struct mbase_control_block_t, persistent_section_start);
        nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->minute_freeze_seq[SeqId - MinuteFreeze1_seq], 4);
    }
    else
    {
        nd_set_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
    }

    return 1;
}


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
        if (mbase_cb->record_rollback_flag != 0x5A)
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

    if (curr_timesec < last_timesec)
    {
        // 非检查 情况下 保存回退标志 
        if (finish_check != 0x5A)
        {
            mbase_cb->record_rollback_flag = 0x5A;
            
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
            offsetof(struct mbase_control_block_t, record_rollback_flag)) -  offsetof(struct mbase_control_block_t, persistent_section_start);
            nd_set_offset(instance, addr, (uint8_t*)&mbase_cb->record_rollback_flag, sizeof(mbase_cb->record_rollback_flag));
        }
        
        num = 0;
        for (i = 0; i < (sizeof(TabOAD) / sizeof(TabOAD[0])); i++)
        {
            //掉电退出
            if (ANA_GetStatus(ANA_STATUS_VDCINDROP))    
            {
                return 1;
            }
            mcu_clear_watchdog();
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
            mbase_cb->record_rollback_flag = 0;
            
            addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
            offsetof(struct mbase_control_block_t, record_rollback_flag)) -  offsetof(struct mbase_control_block_t, persistent_section_start);
            nd_set_offset(instance, addr, (uint8_t*)&mbase_cb->record_rollback_flag, sizeof(mbase_cb->record_rollback_flag));
        }
    }

    return 1;
}


int minute_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i, period[24];
    uint32_t index;
    struct tm record_time;
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

    for (i = 0; i < 8; i++)
    {
        if (1 == freeze_judge(instance, &period[3 * i], 1, &record_time))
        {
            //send_recordflag_mesg(instance, task,  (FlagID + i), NULL);
            mesg_tmp.id = FlagID + i;
            recv_record_process(instance, NULL, &mesg_tmp, NULL);
        }
    }
    return 1;
}
int hour_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t period[24];
    //uint16_t timeinv = 0;
    uint32_t index;
    struct tm record_time;
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

    if (1 == freeze_judge(instance, &period[0], 60, &record_time))
    {
        //send_recordflag_mesg(instance, task,  FlagID, NULL);
        mesg_tmp.id = FlagID;
        recv_record_process(instance, NULL, &mesg_tmp, NULL);
    }

    return 1;
}

int day_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t period[24]= {0}, tmp[10] = {0};
    uint32_t index;
    struct tm record_time;
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

    if (1 == freeze_judge(instance, &period[0], 60*24, &record_time))
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
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint16_t FlagID = MonthFreeze;
    struct message_t mesg_tmp;

    if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0) || (instance->time.tm_hour != 0) || (instance->time.tm_mday != 1))
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
    uint32_t  now,pd,last,recently;
    struct tm now_tm,pd_tm, last_time;
    uint32_t s_index, tmp_u32;
    uint8_t payload[20] = {0};
    //uint8_t *p = payload;
    struct message_t mesg_tmp;
    uint16_t    period;
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

    ndv_memcopy((uint8_t *)&now_tm,(uint8_t *)&instance->time, sizeof(now_tm));
    now_tm.tm_sec = 0;
    pd_tm.tm_sec = 0;


    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
    nd_get_index(instance, s_index, &tmp[0], 6);
    last_time.tm_sec  = tmp[0];
    last_time.tm_min  = tmp[1];
    last_time.tm_hour = tmp[2];
    last_time.tm_mday = tmp[3];
    last_time.tm_mon  = CONVERT_TM_MONTH(tmp[4]);
    last_time.tm_year = tmp[5] + 2000 - 1900;



    now = rtc_mktime(&now_tm)/86400;
    pd  = rtc_mktime(&pd_tm)/86400;          //earliest
    last = rtc_mktime(&last_time)/86400;    //lasted

    recently = pd;

    // 以上电时刻为准，补最近7个日冻结数据
    if ((now - recently) / period > 7)
    {
        pd  =  now - 7 * period + 1;
        
        recently  =  pd;
    }
    else
    {
        recently += period;
    }
        
    while(recently <= now)
    {
        if(now == 0)
        {
            return 0;
        }
        if (recently > last)
        {
            last = recently;
            
            rtc_localtime(recently * 86400, &last_time);
            last_time.tm_min = 0;
            last_time.tm_sec = 0;


            //日冻结数据补数据 相关处理 


            // 刷新最近一次时间
            tmp[0] = last_time.tm_sec ;
            tmp[1] = last_time.tm_min ;
            tmp[2] = last_time.tm_hour;
            tmp[3] = last_time.tm_mday;
            tmp[4] = last_time.tm_mon + 1;
            tmp[5] = (uint8_t)((last_time.tm_year + 1900)-2000);
            
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
            nd_set_index(instance, s_index, &tmp[0], 6);
            
            // 本条记录存储的时间
            mesg_tmp.id = DayFreeze;
            uint32_to_buf_msb(0x20210200, &payload[0]);
            payload[4] = 8; //data len
            payload[5] = 28;//TYPE_date_time_s;
            rtc_format_change_from_tm(&last_time,tmp);
            mem_swap_cpy(&payload[6], &tmp[0], 7);
            recv_record_process(instance, task, &mesg_tmp, payload);  
            v85xxp_clear_watchdog();//清看门狗
        }
        recently += period;
    }


    return 1;
}



//  上电补结算冻结处理           上电调用 
int bill_freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    uint8_t cnt, i, tmp[10]; 
    uint32_t  now,pd,last,recently;
    struct tm now_tm,pd_tm, last_time;
    uint32_t s_index, tmp_u32;
    uint8_t billdate[3][2];
    uint8_t payload[20] = {0};
    //uint8_t *p = payload;
    struct message_t mesg_tmp;
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

    ndv_memcopy((uint8_t *)&now_tm,(uint8_t *)&instance->time, sizeof(now_tm));
    now_tm.tm_sec = 0;
    pd_tm.tm_sec = 0;


    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillingTime);
    nd_get_index(instance, s_index, &tmp[0], 6);
    last_time.tm_sec  = tmp[0];
    last_time.tm_min  = tmp[1];
    last_time.tm_hour = tmp[2];
    last_time.tm_mday = tmp[3];
    last_time.tm_mon  = CONVERT_TM_MONTH(tmp[4]);
    last_time.tm_year = tmp[5] + 2000 - 1900;


    for(i=0;i<3;i++)
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
        nd_get_index(instance, s_index, tmp, 2); 
        billdate[i][0] = tmp[0];
        billdate[i][1] = tmp[1];
    }

    cnt     =   0;

    now = rtc_mktime(&now_tm);
    pd  = rtc_mktime(&pd_tm);
    last = rtc_mktime(&last_time);    

    recently  = pd;

    while(recently <= now)
    {
        for (i = 0; i < 3; i++)
        {
            pd_tm.tm_min = 0;
            pd_tm.tm_mday = billdate[i][0];
            pd_tm.tm_hour = billdate[i][1];
            
            if ((pd_tm.tm_mday == 0x63) && (pd_tm.tm_hour == 0x63)
                || (pd_tm.tm_mday == 0x99) && (pd_tm.tm_hour == 0x99)
                || (pd_tm.tm_mday == 0xFF) && (pd_tm.tm_hour == 0xFF))
            {
                continue;
            }
            
            last   =  rtc_mktime(&last_time);  
            recently =  rtc_mktime(&pd_tm); 
            if (recently <= now)
            {
                if ((recently > last)&&(recently >= pd))
                {
                    if(i == 0)
                    {
                        ctrl_block->first_MbillDay_flag = 0x5A;
                    }
                
                    // 刷新最近一次结算时间
                    tmp[0] = pd_tm.tm_sec ;
                    tmp[1] = pd_tm.tm_min ;
                    tmp[2] = pd_tm.tm_hour;
                    tmp[3] = pd_tm.tm_mday;
                    tmp[4] = pd_tm.tm_mon + 1;
                    tmp[5] = (uint8_t)((pd_tm.tm_year + 1900)-2000);
    
                    ndv_memcopy((uint8_t *)&last_time,(uint8_t *)(&pd_tm), sizeof(pd_tm));
                    
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillingTime);
                    nd_set_index(instance, s_index, &tmp[0], 6);
                    
                    // 本条记录存储的时间
                    mesg_tmp.id = BillFreeze;
                    uint32_to_buf_msb(0x20210200, &payload[0]);
                    payload[4] = 8; //data len
                    payload[5] = 28;//TYPE_date_time_s;
                    rtc_format_change_from_tm(&pd_tm,tmp);
                    mem_swap_cpy(&payload[6], &tmp[0], 7);
                     // 记录1次结算       需根据i判断需量结算是否填空，
                    // 第1结算日过后，最大需量清零                是否需要发送消息？             待添加
                    recv_record_process(instance, task, &mesg_tmp, payload);                    



                }
            }
            else
            {
                return 0;
            }
        }    

        //CLEAR_WATCH_DOG();  清看门狗  
        v85xxp_clear_watchdog();
        if (pd_tm.tm_mon < 11)
        {
            pd_tm.tm_mon++;
        }
        else
        {
            pd_tm.tm_mon  = 0x00;
            pd_tm.tm_year  += 0x01;
        }

        /*最多补5年的数据*/
        if (++cnt > 12)
        {
            return 0;
        }  
    }

    return 0; 
}

#endif

//结算冻结
int bill_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
        struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
        uint32_t s_index;
        struct tm last_time, bill_time, record_time;//
        uint8_t tmp[20];
        uint8_t billdate[3][2];
        uint16_t LastBillTimeID = LastBillingTime;
        uint16_t FlagID = BillFreeze;   
        uint8_t  i;
        uint32_t cur_hour, last_hour, bill_hour;
        int ret;
        struct message_t mesg_tmp;
    
        if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0))
        {
            return -1;
        }
        memset((uint8_t *)&mesg_tmp, 0, sizeof(struct message_t));

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillTimeID);
        ret = nd_get_index(instance, s_index, &tmp[0], 6);

        last_time.tm_sec  = tmp[0];
        last_time.tm_min  = tmp[1];
        last_time.tm_hour = tmp[2];
        last_time.tm_mday = tmp[3];
        last_time.tm_mon  = CONVERT_TM_MONTH(tmp[4]);
        last_time.tm_year = tmp[5] + 2000 - 1900;
        
        cur_hour = rtc_mktime(&instance->time);
        cur_hour = cur_hour / 60 /60;

        last_hour = rtc_mktime(&last_time);
        last_hour = cur_hour / 60 /60;


        for(i=0;i<3;i++)
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
            nd_get_index(instance, s_index, tmp, 2); 
            billdate[i][0] = tmp[0];
            billdate[i][1] = tmp[1];
        }
        
        ndv_memcopy((uint8_t *)&bill_time, (uint8_t *)&instance->time, sizeof(struct tm));

        ctrl_block->first_MbillDay_flag = 0;
        for (i = 0; i < 3; i++)
        {
            bill_time.tm_mday = billdate[i][0];
            bill_time.tm_hour = billdate[i][1];
            
            
            if ((bill_time.tm_mday == 0x63) && (bill_time.tm_hour == 0x63)
                || (bill_time.tm_mday == 0x99) && (bill_time.tm_hour == 0x99)
                || (bill_time.tm_mday == 0xFF) && (bill_time.tm_hour == 0xFF))
            {
                continue;
            }


            bill_hour = rtc_mktime(&bill_time);
            bill_hour = bill_hour / 60 /60;
    
            
            if ((cur_hour == bill_hour)&&(last_hour != bill_hour))
            {
                
                //第二、第三结算日时需量需要清成FF       
                if (i == 0x00)
                {
                    ctrl_block->first_MbillDay_flag = 0x5A;
                }

                //保存 结算时间
                tmp[0] = bill_time.tm_sec ;
                tmp[1] = bill_time.tm_min ;
                tmp[2] = bill_time.tm_hour;
                tmp[3] = bill_time.tm_mday;
                tmp[4] = bill_time.tm_mon + 1;
                tmp[5] = (uint8_t)((bill_time.tm_year + 1900)-2000);

                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillTimeID);
                nd_set_index(instance, s_index, &tmp[0], 6);
   
                ndv_memcopy((uint8_t *)&record_time, (uint8_t *)&instance->time, sizeof(struct tm));   
                //send_recordflag_mesg(instance, task,  FlagID, NULL);
                mesg_tmp.id = FlagID;
                recv_record_process(instance, NULL, &mesg_tmp, NULL);
                last_hour = bill_hour;
            }
        }
    
        return false;

}



#if (FREEZ_TEST_OVERTURN)
uint8_t test_freeze = 0;
#endif
// 每秒调用
int freeze_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{

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
        send_recordflag_mesg(instance, task,  MonthFreeze, NULL);           
        send_recordflag_mesg(instance, task,  BillFreeze, NULL);   
        
        //send_recordflag_mesg(instance, task,  SeasonTabCutFreeze, NULL);   
        //send_recordflag_mesg(instance, task,  DayTabCutFreeze, NULL);   
        //send_recordflag_mesg(instance, task,  InstantFreeze, NULL);   
    }
    else
#endif    
    {
    
        record_rollback_after_set_time(instance, 0x5A);//冻结回滚检查
   
        minute_freeze_judge(instance, task, mesg); //分钟冻结
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
            if((mbase_cb->minute_freeze_seq[i] < (sequence + 1)) &&  (sequence != 0xFFFFFFFF))
            {
                mbase_cb->minute_freeze_seq[i] = sequence + 1;  //待调整  待测试
            }
        }
    }

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


