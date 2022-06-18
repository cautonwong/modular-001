#include "nidavellir.h"
#include "applications.h"
#include "gs_record.h"

//extern void set_recordflag(struct pipeline_t* task, uint16_t RecordId);

//计算分钟、小时、日、月冻结 总长度是否超预留空间， 修改的关联对象时调用，  入参关联对象、深度，  
//并且计算保存分钟冻结0-7  8个方案中每个方案的空间大小偏移(扇区)
int cal_freeze_size_sectors(struct instance_t* instance, uint8_t *capture_ptr, uint8_t *depth_ptr, uint32_t oad)
{
    uint8_t scheme = 0,obj_num, scheme_num, tmpbuf[220] = {0};
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
        case 0x50000000: //  瞬时
            sectorMax = 2; //待调整
            break;
        case 0x50020000:// 分钟
            sectorMax = 830; //待调整
            scheme_num = 8;
            break;
        case 0x50030000: // 小时
        case 0x50060000: // 月
            sectorMax = 5;  //待调整
            break;
        case 0x50040000: // 日
            sectorMax = 27; //待调整
            break;
    default:
        return 1;  //其他
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
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
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


int rollback_sequence(struct instance_t* instance, uint32_t oad,  uint16_t rollback_num)
{
    uint32_t sequence = 0;
    struct record_storage_id_info_t record_id_info; 
    int ret;
    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret < 0)
    {
        return -(255);
    }
    
    nd_get_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
    if (sequence <  rollback_num)
    {
        sequence = 0;
    }
    else
    {
        sequence -= rollback_num;
    }
    
    nd_set_index(instance, record_id_info.sequence_index, (uint8_t*)&sequence, sizeof(sequence));
    return 1;
}


//设置或修改时间后调用  冻结回退处理
int record_rollback_after_set_time(struct instance_t* instance)
{
    struct record_storage_id_info_t record_id_info; 
    uint8_t i, num = 0, buffer[10] = { 0xFF };
    struct tm ;
    uint32_t addr,last_timesec, curr_timesec;
    uint16_t rollback_seq_num[11] = { 0 };
    int ret = 0;
    //uint16_t FlagID[] = { MinuteFreeze1, MinuteFreeze2, MinuteFreeze3, MinuteFreeze4, MinuteFreeze5, MinuteFreeze6, MinuteFreeze7, MinuteFreeze8, HourFreeze, DayFreeze, MonthFreeze, };
    uint32_t TabOAD[11] = { 0x50020200, 0x50022200, 0x50024200, 0x50026200, 0x50028200, 0x5002A200, 0x5002C200, 0x5002E200, 0x50030200, 0x50040200, 0x50060200, };


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
            
            rollback_seq_num[i] = nd_log_append_rollback(instance, record_id_info.save_index  , public_block_flash, last_timesec, curr_timesec);
            if (rollback_seq_num[i] > 0)
            {
                rollback_sequence(instance, TabOAD[i], rollback_seq_num[i]);
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
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    int ret = 1;
    //uint32_t TabOAD[] = { 0x50020200, 0x50022200, 0x50024200, 0x50026200, 0x50028200, 0x5002A200, 0x5002C200, 0x5002E200, 0x50030200, 0x50040200, 0x50060200, };

    //读取回退是否完成标志
  

    return ret;
}

int minute_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
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
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t period[24];
    //uint16_t timeinv = 0;
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
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t period[24]= {0}, tmp[10] = {0};
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
            //保存 结算时间
        tmp[0] = instance->time.tm_sec ;
        tmp[1] = instance->time.tm_min ;
        tmp[2] = instance->time.tm_hour;
        tmp[3] = instance->time.tm_mday;
        tmp[4] = instance->time.tm_mon + 1;
        tmp[5] = (uint8_t)((instance->time.tm_year + 1900)-2000);

        index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
        nd_set_index(instance, index, &tmp[0], 5);
        
        send_recordflag_mesg(instance, task,  FlagID, NULL);
    }

    return 1;
}

//月冻结
int month_freeze_judge(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
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

#if 1


//上电日冻结补冻结处理
int day_freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{

    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    uint8_t tmp[10]; 
    uint32_t  now,pd,last,recently;
    struct tm now_tm,pd_tm, last_time;
    uint32_t s_index;
    uint8_t payload[20] = {0};
    //uint8_t *p = payload;
    struct message_t mesg_tmp;
    uint16_t    period;


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
        if (false == rtc_tm_valid(&ctrl_block->powerdown_tm))   // 停电时间无效       退出
        {
            return 0;
        }
        ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->powerdown_tm), sizeof(ctrl_block->powerdown_tm));
    }

    ndv_memcopy((uint8_t *)&now_tm,(uint8_t *)&instance->time, sizeof(now_tm));
    now_tm.tm_sec = 0;
    pd_tm.tm_sec = 0;


    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
    nd_get_index(instance, s_index, &tmp[0], 5);
    last_time.tm_sec  = tmp[0];
    last_time.tm_min  = tmp[1];
    last_time.tm_hour = tmp[2];
    last_time.tm_mday = tmp[3];
    last_time.tm_mon  = tmp[4] - 1;
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


            //日冻结数据补数据 相关处理               待调整


            // 刷新最近一次时间
            tmp[0] = last_time.tm_sec ;
            tmp[1] = last_time.tm_min ;
            tmp[2] = last_time.tm_hour;
            tmp[3] = last_time.tm_mday;
            tmp[4] = last_time.tm_mon + 1;
            tmp[5] = (uint8_t)((last_time.tm_year + 1900)-2000);
            
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastDayFreezTime);
            nd_set_index(instance, s_index, &tmp[0], 5);
            
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
    
    uint8_t cycle, i, tmp[10]; 
    uint32_t  now,pd,last,recently;
    struct tm now_tm,pd_tm, last_time;
    uint32_t s_index;
    uint8_t billdate[3][2];
    uint8_t payload[20] = {0};
    //uint8_t *p = payload;
    struct message_t mesg_tmp;

    ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->v_powerdown_tm), sizeof(ctrl_block->v_powerdown_tm));

    if (false == rtc_tm_valid(&ctrl_block->v_powerdown_tm))    // 模拟停电时间无效          取上次停电时间
    {
        if (false == rtc_tm_valid(&ctrl_block->powerdown_tm))   // 停电时间无效       退出
        {
            return 0;
        }
        ndv_memcopy((uint8_t *)&pd_tm,(uint8_t *)(&ctrl_block->powerdown_tm), sizeof(ctrl_block->powerdown_tm));
    }

    ndv_memcopy((uint8_t *)&now_tm,(uint8_t *)&instance->time, sizeof(now_tm));
    now_tm.tm_sec = 0;
    pd_tm.tm_sec = 0;


    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillingTime);
    nd_get_index(instance, s_index, &tmp[0], 5);
    last_time.tm_sec  = tmp[0];
    last_time.tm_min  = tmp[1];
    last_time.tm_hour = tmp[2];
    last_time.tm_mday = tmp[3];
    last_time.tm_mon  = tmp[4] - 1;
    last_time.tm_year = tmp[5] + 2000 - 1900;


    for(i=0;i<3;i++)
    {
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
        nd_get_index(instance, s_index, tmp, 2); 
        billdate[i][0] = tmp[0];
        billdate[i][1] = tmp[1];
    }


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
                    // 刷新最近一次结算时间
                    tmp[0] = pd_tm.tm_sec ;
                    tmp[1] = pd_tm.tm_min ;
                    tmp[2] = pd_tm.tm_hour;
                    tmp[3] = pd_tm.tm_mday;
                    tmp[4] = pd_tm.tm_mon + 1;
                    tmp[5] = (uint8_t)((pd_tm.tm_year + 1900)-2000);
                    
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillingTime);
                    nd_set_index(instance, s_index, &tmp[0], 5);
                    
                    // 本条记录存储的时间
                    mesg_tmp.id = BillFreeze;
                    uint32_to_buf_msb(0x20210200, &payload[0]);
                    payload[4] = 8; //data len
                    payload[5] = 28;//TYPE_date_time_s;
                    rtc_format_change_from_tm(&pd_tm,tmp);
                    mem_swap_cpy(&payload[6], &tmp[0], 7);
 
                    recv_record_process(instance, task, &mesg_tmp, payload);                    
                    // 记录1次结算       需根据i判断需量结算是否填空，
                    // 第1结算日过后，最大需量清零                是否需要发送消息？             待添加
                }
            }
            else
            {
                return 0;
            }
        }    

        //CLEAR_WATCH_DOG();  清看门狗  待调整
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
        if (++cycle > 60)
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

    
        if ((instance->time.tm_sec != 0) || (instance->time.tm_min != 0))
        {
            return -1;
        }

        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillTimeID);
        nd_get_index(instance, s_index, &tmp[0], 5);
        last_time.tm_sec  = tmp[0];
        last_time.tm_min  = tmp[1];
        last_time.tm_hour = tmp[2];
        last_time.tm_mday = tmp[3];
        last_time.tm_mon  = tmp[4] - 1;
        last_time.tm_year = tmp[5] + 2000 - 1900;
        
        //上次结算时间错，不结算 待调整
        
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
                
                //第二、第三结算日时需量需要清成FF                 待调整
                if (i == 0x00)
                {
                
                }

                //保存 结算时间
                tmp[0] = bill_time.tm_sec ;
                tmp[1] = bill_time.tm_min ;
                tmp[2] = bill_time.tm_hour;
                tmp[3] = bill_time.tm_mday;
                tmp[4] = bill_time.tm_mon + 1;
                tmp[5] = (uint8_t)((bill_time.tm_year + 1900)-2000);

                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBillTimeID);
                nd_set_index(instance, s_index, &tmp[0], 5);
   
                ndv_memcopy((uint8_t *)&record_time, (uint8_t *)&instance->time, sizeof(struct tm));   
                send_recordflag_mesg(instance, task,  FlagID, NULL);
                
                last_hour = bill_hour;
            }
        }
    
        return false;

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
    bill_freeze_judge(instance, task, mesg);
    return 1;
}


// 上电调用
int freeze_power_on_pro(struct instance_t* instance, struct pipeline_t* task, struct message_t* mesg)
{

    day_freeze_power_on_pro(instance, task, mesg);
    bill_freeze_power_on_pro(instance, task, mesg);
    return 1;
}


