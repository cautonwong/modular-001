#include "nidavellir.h"
#include "fr_698.h"



int get_frozen_obj(struct instance_t* instance, uint32_t id, uint8_t* buf, struct frozen_obj_t frozen_obj, uint8_t* dar)
{

    uint8_t i, num, maxnum, scheme;
    uint8_t tempbuf[200], period[1 + 3 * 8], depth[1 + 2 * 15];
    uint8_t* tmp0, * tmp1;
    uint8_t* p = buf;
    uint8_t* oadlist = tempbuf;
    uint16_t OI, CaptureId, PeriodId, DepthId, CaptureLen, PeriodLen, DepthLen;
    uint32_t index;

    CaptureId = frozen_obj.CaptureId;
    PeriodId = frozen_obj.PeriodId;
    DepthId = frozen_obj.DepthId;
    CaptureLen = frozen_obj.CaptureLen;  // 
    PeriodLen = frozen_obj.PeriodLen;
    DepthLen = frozen_obj.DepthLen;
    OI = (uint16_t)(id >> 16);

    maxnum = (CaptureLen - 1) / 4;

    index = ((RES_DEV_EEPROM << 24) + (0x09 << 16) + CaptureId);
    nd_get_index(instance, index, &tempbuf[0], sizeof(tempbuf));
    oadlist = tempbuf;
    num = *oadlist++;

    // 周期
    index = ((RES_DEV_EEPROM << 24) + (0x09 << 16) + PeriodId);
    nd_get_index(instance, index, &period[0], sizeof(period));
    tmp0 = period;

    // 深度
    index = ((RES_DEV_EEPROM << 24) + (0x09 << 16) + DepthId);
    nd_get_index(instance, index, &depth[0], sizeof(depth));
    tmp1 = depth;

    if (OI != 0x5002)
    {
        tmp1++;
    }
    /*
    //三组 个数应一致
    if ((depth[0] != period[0]) || (depth[0] != num))
    {
        return DAR_OTHERS_ERROR;
    }
    */

    *p++ = TYPE_array; //TYPE_array

    if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
    {
        *dar = DAR_OTHERS_ERROR;
        return -1;
    }

    *p++ = num;
    for (i = 0; i < num; i++)
    {
        *p++ = TYPE_structure;
        *p++ = 3;
        *p++ = TYPE_long_unsigned;

        scheme = ((*(oadlist + 1)) >> 13) & 0x07; //方案号
        *p++ = *(tmp0 + scheme * 3 + 2);
        *p++ = *(tmp0 + scheme * 3 + 1);

        *p++ = TYPE_OAD;
        mem_swap_cpy(p, oadlist, 4);
        oadlist += 4;
        p += 4;

        *p++ = TYPE_long_unsigned;
        if (OI == 0x5002)
        {
            *p++ = *(tmp1 + scheme * 2 + 1);
            *p++ = *(tmp1 + scheme * 2);
        }
        else
        {
            *p++ = *(tmp1 + i * 2 + 1);
            *p++ = *(tmp1 + i * 2);
        }
    }

    *dar = DAR_SUCCESS;
    return (p - buf);
}

int GET_5_Frozen(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t dar = DAR_SUCCESS;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (id)
    {
    case 0x50020300://关联对象属性表
    {
        struct frozen_obj_t frozen_obj;

        frozen_obj.CaptureId = MinuteFreeze_oad;
        frozen_obj.PeriodId = MinuteFreeze_period;
        frozen_obj.DepthId = MinuteFreeze_depth;
        frozen_obj.CaptureLen = 1 + 4 * 15;  // 
        frozen_obj.PeriodLen = 24;
        frozen_obj.DepthLen = 31;

        return get_frozen_obj(instance, id, buf, frozen_obj, &dar);
    }
    break;
    default:
        break;
    }
    return 0x01;
}
