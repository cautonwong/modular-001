#include "nidavellir.h"
#include "fr_698.h"
#include "gs_record.h"


int freeze_add_one_capture(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint8_t num, maxnum, scheme, offset;
    uint8_t tempbuf[200], period_buf[3 * 8], depth_buf[1 + 2 * 15];
//    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t period, depth, OI, CaptureLen, PeriodLen, DepthLen;
    uint32_t CaptureId, PeriodId, DepthId;
    uint32_t capture_oad;

    struct record_storage_id_info_t  record_id_info;
    int ret;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    CaptureId = record_id_info.capture_index;
    PeriodId = record_id_info.period_index;
    DepthId = record_id_info.depth_index;
    CaptureLen = record_id_info.capture_len;// 
    PeriodLen = record_id_info.period_len;
    DepthLen = record_id_info.depth_len;
    maxnum = (CaptureLen - 1) / 4;

    OI = (uint16_t)(oad >> 16);
    
    memset(period_buf, 0, sizeof(period_buf));
    memset(depth_buf, 0, sizeof(depth_buf));
    memset(tempbuf, 0, sizeof(tempbuf));

    if ((p[0] != TYPE_structure) || (p[1] != 3) || (p[2] != TYPE_long_unsigned) || (p[5] != TYPE_OAD) || (p[10] != TYPE_long_unsigned))
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    p += 3;
    period = buf_to_uint16_msb(p);
    p += 3;
    capture_oad = buf_to_uint32_msb(p);
    ret = check_oad(capture_oad);
    if(ret < 0){
        return -DAR_RW_REJECT;
    }
    p += 5;
    depth = buf_to_uint16_msb(p);
    p += 2;

    scheme = (uint8_t)((capture_oad >> 13) & 0x07); //方案号

    nd_get_index(instance, CaptureId, &tempbuf[0], CaptureLen);
    oadlist = tempbuf;
    num = *oadlist++;

    // 周期
    nd_get_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // 深度
    nd_get_index(instance, DepthId, &depth_buf[0], DepthLen);

    if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    // oad check 

    // period check
    if (true != period_check1(OI, &period))
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    if (true != period_check2(scheme, period, period_buf))
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }
#if 0
    if (false == capture_check(capture_oad, tempbuf))
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }
#endif
    //周期
    offset = scheme * 3;
    uint16_to_buf_msb(period, period_buf + offset + 1);
    period_buf[offset]++;

    //obj
    offset = num * 4;
    uint32_to_buf_msb(capture_oad, tempbuf + offset + 1);
    tempbuf[0]++;

    // check depth
    if (0x5002 == OI)
    {
        offset = scheme * 2;
        if (((period_buf[scheme * 3] > 1) && (depth != buf_to_uint16_msb(depth_buf + offset))))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
    }
    else
    {
        if ((0 != scheme) || ((period_buf[0] > 1) && (depth != buf_to_uint16_msb(depth_buf))))   /*其他冻结 属性特征固定0,每个关联对象周期、深度必须一致*/
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
        offset = 0;
    }
    uint16_to_buf_msb(depth, depth_buf + offset);

#if 1
    ret = cal_record_size_sectors(instance,tempbuf, depth_buf, oad);
    if (ret <= 0)
    {
        return -DAR_BUFFER_OVERFLOW;
    }
#endif   
    // write  period
    nd_set_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // write  Capture
    nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);

    // write  depth
    nd_set_index(instance, DepthId, &depth_buf[0], DepthLen);

    //修改关联对象后 清零处理
    ret = modify_capture_clr_pro(instance, oad);
    
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }


    return (int)(p - buf);
}

int freeze_delete_one_capture(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint8_t i, num, maxnum, scheme, offset;
    uint8_t tempbuf[200], period_buf[3 * 8],depth_buf[1 + 2 * 15];
//    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t  OI, CaptureLen, PeriodLen,DepthLen;//period, depth,
    uint32_t CaptureId, PeriodId, DepthId;
    uint32_t capture_oad, temp_oad;

    struct record_storage_id_info_t  record_id_info;
    int ret;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    CaptureId = record_id_info.capture_index;
    PeriodId = record_id_info.period_index;
    DepthId = record_id_info.depth_index;
    CaptureLen = record_id_info.capture_len;
    PeriodLen = record_id_info.period_len;
    DepthLen = record_id_info.depth_len;
    maxnum = (CaptureLen - 1) / 4;

    OI = (uint16_t)(oad >> 16);

    memset(period_buf, 0, sizeof(period_buf));
    memset(depth_buf, 0, sizeof(depth_buf));
    memset(tempbuf, 0, sizeof(tempbuf));

    if ((*p++ != TYPE_OAD))
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }


    // 周期
    nd_get_index(instance, PeriodId, &period_buf[0], PeriodLen);

    ret = nd_get_index(instance, CaptureId, &tempbuf[0], CaptureLen);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    oadlist = tempbuf;
    num = *oadlist++;

    if ((num >= maxnum) || (num == 0))
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    capture_oad = buf_to_uint32_msb(p);
    scheme = (uint8_t)((capture_oad >> 13) & 0x07); //方案号

    if ((0x5002 != OI) && (0 != scheme)) //除分钟冻结外 不支持多方案配置
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    for (i = 0; i < num; i++)
    {
        temp_oad = buf_to_uint32_msb(oadlist);
        if (capture_oad == temp_oad)
        {
            break;
        }
        oadlist += 4;
    }
#if 0
    if (i >= num)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }
#endif
    offset = scheme * 3;//对应周期
    period_buf[offset]--;


    memcpy(oadlist, oadlist + 4, CaptureLen + tempbuf - oadlist);//关联对象
    tempbuf[0] -= 1;

#if 1
    ret = cal_record_size_sectors(instance,tempbuf, depth_buf, oad);
    if (ret <= 0)
    {
        return -DAR_BUFFER_OVERFLOW;
    }
#endif     
    // write  period
    nd_set_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // write  Capture
    nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);

    //修改关联对象后 清零处理
    ret = modify_capture_clr_pro(instance, oad);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    return (int)(p - buf);
}

int freeze_add_multi_capture(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint8_t add_num, num, maxnum, scheme, offset;
    uint8_t tempbuf[200], period_buf[3 * 8], depth_buf[1 + 2 * 15];
//    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t period, depth, OI, CaptureLen, PeriodLen, DepthLen;
    uint32_t CaptureId, PeriodId, DepthId;
    uint32_t capture_oad;

    struct record_storage_id_info_t  record_id_info;
    int ret;

    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -DAR_OBJECT_NOT_EXIST;;
    }

    CaptureId = record_id_info.capture_index;
    PeriodId = record_id_info.period_index;
    DepthId = record_id_info.depth_index;
    CaptureLen = record_id_info.capture_len;
    PeriodLen = record_id_info.period_len;
    DepthLen = record_id_info.depth_len;
    maxnum = (CaptureLen - 1) / 4;

    OI = (uint16_t)(oad >> 16);

    memset(period_buf, 0, sizeof(period_buf));
    memset(depth_buf, 0, sizeof(depth_buf));
    memset(tempbuf, 0, sizeof(tempbuf));

    if (*p++ != TYPE_array)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    add_num = *p++;

    // 获取关联对象
    nd_get_index(instance, CaptureId, &tempbuf[0], CaptureLen);
    oadlist = tempbuf;
    oadlist++;
    num = tempbuf[0] + add_num;
    if (num > maxnum)  // 关联对象个数超
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_BUFFER_OVERFLOW;
    }

    // 获取周期
    nd_get_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // 获取深度
    nd_get_index(instance, DepthId, &depth_buf[0], DepthLen);

    while (add_num--)
    {
        // 喂狗
        if ((p[0] != TYPE_structure) || (p[1] != 3) || (p[2] != TYPE_long_unsigned) || (p[5] != TYPE_OAD) || (p[10] != TYPE_long_unsigned))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }

        p += 3;
        period = buf_to_uint16_msb(p);
        p += 3;
        capture_oad = buf_to_uint32_msb(p);
        p += 5;
        depth = buf_to_uint16_msb(p);
        p += 2;

        scheme = (uint8_t)((capture_oad >> 13) & 0x07); //方案号

        // oad check 

        // period check
        if (true != period_check1(OI, &period))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }

        if (true != period_check2(scheme, period, period_buf))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
#if 0
        if (false == capture_check(capture_oad, tempbuf))
        {
            *dar = DAR_OTHERS_ERROR;
            return -DAR_OTHERS_ERROR;
        }
#endif
        //周期
        offset = scheme * 3;
        uint16_to_buf_msb(period, period_buf + offset + 1);
        period_buf[offset]++;

        //obj
        offset = tempbuf[0] * 4;
        uint32_to_buf_msb(capture_oad, tempbuf + offset + 1);
        tempbuf[0]++;

        // check depth
        if (0x5002 == OI)
        {
            offset = scheme * 2;
            if (((period_buf[scheme * 3] > 1) && (depth != buf_to_uint16_msb(depth_buf + offset))))
            {
                *dar = DAR_OTHERS_ERROR;
                return -DAR_OTHERS_ERROR;
            }
        }
        else
        {
            if ((0 != scheme) || ((period_buf[0] > 1) && (depth != buf_to_uint16_msb(depth_buf))))   /*其他冻结 属性特征固定0,每个关联对象周期、深度必须一致*/
            {
                *dar = DAR_OTHERS_ERROR;
                return -DAR_OTHERS_ERROR;
            }
            offset = 0;
        }
        uint16_to_buf_msb(depth, depth_buf + offset);
    }

#if 1
    ret = cal_record_size_sectors(instance,tempbuf, depth_buf, oad);
    if (ret <= 0)
    {
        return -DAR_BUFFER_OVERFLOW;
    }
#endif    
    // write  period
    nd_set_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // write  Capture
    nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);

    // write  depth
    nd_set_index(instance, DepthId, &depth_buf[0], DepthLen);

    //修改关联对象后 清零处理
    ret = modify_capture_clr_pro(instance, oad);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }

    return (int)(p - buf);
}

int freeze_delete_all_capture(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t* dar)
{
    uint8_t maxnum;//, scheme;//, offset;num, 
    uint8_t tempbuf[200], period_buf[3 * 8], depth_buf[1 + 2 * 15];
    uint8_t* p = buf;
//    uint8_t* oadlist = tempbuf;
    uint16_t OI, CaptureLen, PeriodLen, DepthLen;//period, depth,
    uint32_t CaptureId, PeriodId, DepthId;
//    uint32_t capture_oad;

    struct record_storage_id_info_t  record_id_info;
    int ret;

    if (*p++ != TYPE_null)
    {
        *dar = DAR_TYPE_NOT_MATCH;
        return -DAR_TYPE_NOT_MATCH;
    } 
    
    ret = get_record_id_info(instance, oad, &record_id_info);
    if (ret <= 0)
    {
        return -DAR_OBJECT_NOT_EXIST;
    }

    CaptureId = record_id_info.capture_index;
    PeriodId = record_id_info.period_index;
    DepthId = record_id_info.depth_index;
    CaptureLen = record_id_info.capture_len;
    PeriodLen = record_id_info.period_len;
    DepthLen = record_id_info.depth_len;
    maxnum = (CaptureLen - 1) / 4;

    OI = (uint16_t)(oad >> 16);

    memset(period_buf, 0, sizeof(period_buf));
    memset(depth_buf, 0, sizeof(depth_buf));
    memset(tempbuf, 0, sizeof(tempbuf));

    // write  period
    nd_set_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // write  Capture
    nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);

    // write  depth
    nd_set_index(instance, DepthId, &depth_buf[0], DepthLen);


    //修改关联对象后 清零处理
    ret = modify_capture_clr_pro(instance, oad);
    if (ret < 0)
    {
        *dar = DAR_OTHERS_ERROR;
        return -DAR_OTHERS_ERROR;
    }
    return (int)(p - buf);
}


int ACTION_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{  

    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
//    uint16_t i;
//    uint8_t tempbuf[200];
//    uint8_t* des = tempbuf;
    uint8_t  dar = 0;
//    int ret;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if (mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }

    uint16_t oi;
    uint8_t methond, index;

    oi = (uint16_t)((id & 0xFFFF0000) >> 16);
    methond = (uint8_t)((id & 0x0000FF00) >> 8);
    index = (uint8_t)(id & 0x000000FF);

    switch (methond)
    {
    case 3: // 触发一次冻结 
        if ((0x5000 != oi) || (p[0] != TYPE_long_unsigned))
        {
            dar = DAR_TYPE_NOT_MATCH;
            return -DAR_TYPE_NOT_MATCH;
        }
        p++;
        mbase_cb->trigger_insfreeze_delay_sec = buf_to_uint16_msb(p);
        return (int)(p - buf);

    case 4: // 添加一个冻结关联对象属性 
        return freeze_add_one_capture(instance, id, p, &dar);

    case 5: //  删除一个冻结关联对象属性 
        return freeze_delete_one_capture(instance, id, p, &dar);

    case 7: // 批量添加冻结对象属性 
        return freeze_add_multi_capture(instance, id, p, &dar);

    case 8: //  清除关联对象属性表 
        return freeze_delete_all_capture(instance, id, p, &dar);


    default:
        break;
    }

    return -DAR_OBJECT_UNDEF;
}
