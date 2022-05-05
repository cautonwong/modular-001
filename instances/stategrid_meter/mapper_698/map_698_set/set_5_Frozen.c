#include "nidavellir.h"
#include "fr_698.h"


int period_check1(uint16_t oi, uint16_t* capture_period)
{
    uint16_t* ptr;

    ptr = capture_period;

    switch (oi)
    {
    case 0x5000:
    case 0x5005:
    case 0x5008:
    case 0x5009:
    case 0x500A:
    case 0x500B:
    case 0x5011:
        if (ptr[0] != 0 && ptr[0] != 1)
        {
            return false;
        }
        break;

    case 0x5001:
    case 0x5002:
        if (ptr[0] > 60)
        {
            return false;
        }

        if (ptr[0] == 0)
        {
            ptr[0] = 1;
        }
        break;

    case 0x5003:
        if (ptr[0] > 24)
        {
            return false;
        }

        if (ptr[0] == 0)
        {
            ptr[0] = 1;
        }
        break;

    case 0x5004:
        if (ptr[0] > 28)
        {
            return false;
        }

        if (ptr[0] == 0)
        {
            ptr[0] = 1;
        }
        break;

    case 0x5006:
        if (ptr[0] > 12)
        {
            return DAR_OTHERS_ERROR;
        }

        if (ptr[0] == 0)
        {
            ptr[0] = 1;
        }
        break;

    default:
        return false;
    }

    return true;
}

int period_check2(uint8_t scheme, uint16_t period, uint8_t* period_buf)
{
    uint8_t i, * tmp;
    uint16_t value;

    tmp = period_buf;

    for (i = 0; i < 8; i++)
    {
        value = buf_to_uint16_msb(tmp + 1);

        if (tmp[0] != 0)
        {
            if (i == scheme)
            {
                if (period != value)
                {
                    return false;
                }
            }
            else
            {
                if (period == value)
                {
                    return false;
                }
            }
        }

        tmp += 3;
    }

    return true;
}

uint8_t capture_check(uint32_t oad, uint8_t* oad_buf)
{
    uint8_t i, count, * tmp;
    uint32_t oad_temp;

    count = oad_buf[0];
    tmp = oad_buf + 1;

    for (i = 0; i < count; i++)
    {
        oad_temp = buf_to_uint32_msb(tmp); 

        if (oad_temp == oad)
        {
            return false;
        }

        tmp += 4;
    }

    return true;
}
int set_frozen_capture(struct instance_t* instance, uint32_t id, uint8_t* buf, struct frozen_obj_t frozen_obj, uint8_t* dar)
{
    uint8_t i,num, maxnum, scheme, offset;
    uint8_t tempbuf[200],period_buf[3 * 8], depth_buf[1 + 2 * 15];
    uint8_t* p = buf;
    uint16_t period, depth, OI, CaptureLen, PeriodLen, DepthLen;
    uint32_t CaptureId, PeriodId, DepthId;
    uint32_t oad;

    struct record_storage_id_info_t  record_id_info;
    int ret;

    ret = get_record_id_info(instance, id, &record_id_info);
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

 
#if 0
    CaptureId = frozen_obj.CaptureId;
    PeriodId = frozen_obj.PeriodId;
    DepthId = frozen_obj.DepthId;
    CaptureLen = frozen_obj.CaptureLen;  // 
    PeriodLen = frozen_obj.PeriodLen;
    DepthLen = frozen_obj.DepthLen;
#endif


    OI = (uint16_t)(id >> 16);
    
    memset(period_buf, 0, sizeof(period_buf));
    memset(depth_buf, 0, sizeof(depth_buf));
    memset(tempbuf, 0, sizeof(tempbuf));

 
    /*
    if (req->oad.elem.index != 0)
    {
        return DAR_RW_REJECT;
    }*/

    if (*p++ != TYPE_array)
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    num = *p++;
    if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    //tempbuf[0] = num;
    for (i = 0; i < num; i++)
    {
        if ((p[0] != TYPE_structure) || (p[1] != 3) || (p[2] != TYPE_long_unsigned) || (p[5] != TYPE_OAD) || (p[10] != TYPE_long_unsigned))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        p += 3;
        period = buf_to_uint16_msb(p);
        p += 3;
        oad = buf_to_uint32_msb(p);
        p += 5;
        depth = buf_to_uint16_msb(p);
        p += 2;

        
        scheme = (uint8_t)((oad >> 13) & 0x07); //方案号
        
        // oad check 

        // period check
        if (true != period_check1(OI, &period))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        if (true != period_check2(scheme, period, period_buf))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        offset = scheme * 3;
        uint16_to_buf_msb(period, period_buf + offset + 1);
        period_buf[offset]++;

        if (false == capture_check(oad, tempbuf))
        {
            *dar = DAR_OTHERS_ERROR;
            return -1;
        }

        tempbuf[0]++;
        offset = i * 4;
        uint32_to_buf_msb(oad, tempbuf + offset + 1);

        // check depth
        if (0x5002 == OI)
        {
            offset = scheme * 2 ;
            if ((i != 0) && ((period_buf[scheme * 3] > 1) && (depth != buf_to_uint16_msb(depth_buf + offset))))
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }
        }
        else
        {
            if ((0 != scheme) || ((i != 0) && (period_buf[0] > 1) && (depth != buf_to_uint16_msb(depth_buf ))))   /*其他冻结 属性特征固定0,每个关联对象周期、深度必须一致*/
            {
                *dar = DAR_OTHERS_ERROR;
                return -1;
            }
            offset = 0;
        }
        uint16_to_buf_msb(depth, depth_buf + offset);
    }

#if 1
    ret = cal_freeze_size_sectors(instance,tempbuf, depth_buf, id);
    if (ret <= 0)
    {
        return -1;
    }
#endif     
    // write  period
    //index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + PeriodId);
    nd_set_index(instance, PeriodId, &period_buf[0], PeriodLen);

    // write  Capture
    //index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + CaptureId);
    nd_set_index(instance, CaptureId, &tempbuf[0], CaptureLen);

    // write  depth
    //index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + DepthId);
    nd_set_index(instance, DepthId, &depth_buf[0], DepthLen);

    //修改关联对象后 清零处理
    ret = modify_capture_clr_pro(instance, oad);
    
    dar = DAR_SUCCESS;
    return (p - buf);// 
}

int SET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    switch (id&0x00001F00) //属性
    {
    //case 0x50020300://关联对象属性表
    //case 0x50030300://关联对象属性表
    //case 0x50040300://关联对象属性表
    //case 0x50050300://关联对象属性表
    //case 0x50050300://关联对象属性表
    case 0x00000300://关联对象属性表
    {
        uint8_t dar = DAR_SUCCESS;
        struct frozen_obj_t frozen_obj;

        frozen_obj.CaptureId = MinuteFreeze_oad;
        frozen_obj.PeriodId = MinuteFreeze_period;
        frozen_obj.DepthId = MinuteFreeze_depth;
        frozen_obj.CaptureLen = 1 + 4*15 ;  // 
        frozen_obj.PeriodLen = 24;
        frozen_obj.DepthLen = 31;

        return set_frozen_capture(instance, id, buf, frozen_obj, &dar);
    }
    
    default:
        break;
    }
    return -DAR_RW_REJECT;
}
