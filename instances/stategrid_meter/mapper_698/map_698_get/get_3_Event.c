#include "nidavellir.h"
#include "fr_698.h"


extern const uint8_t init_30050200[];
int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;
    uint8_t tempbuf[200];
    uint8_t* src = tempbuf;
    uint16_t i;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (id)
    {
    case 0x30050200://关联对象属性表
    {
        uint8_t num,maxnum;
        uint32_t CaptureId, index;
        
        CaptureId = OverCurrentL1_oad;
        maxnum = ((13*init_30050200[1]+9)/10);

        index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + CaptureId);
        nd_get_index(instance, index, &tempbuf[0], sizeof(tempbuf));
        src = tempbuf;
        num = *src++;

        *p++ = TYPE_array; //TYPE_array
        
        if (num > maxnum)  // 关联对象按个数的1.3倍限制 or 按默认存储空间的1.3倍限制
        {
            return DAR_OTHERS_ERROR;
        }
        
        *p++ = num;
        for (i = 0; i < num; i++)
        {
            *p++ = TYPE_OAD;
            mem_swap_cpy(p, src, 4);
            src += 4;
            p += 4;            
        }

        return (p - buf);
    }
    case 0x30110300://当前记录数
    case 0x30050300://当前记录数
    {
        return 0;
    }
    case 0x30050400://最大记录数
    {
        return 0;
    }
    case 0x30050500://配置参数 过流阈值
    {
        *p++ = TYPE_structure;
        *p++ = 2;
        *p++ = TYPE_double_long;
        memcpy(p, (uint8_t*)&mbase_cb->i_over_event_threshold, 0x04);
        p += 4;
        *p++ = TYPE_unsigned;;
        memcpy(p, (uint8_t*)&mbase_cb->i_over_event_duration_para, 0x01);
        p++;
        return (p - buf);
    }
    case 0x30050600://事件记录表1
    {
        return 0;
    }
    case 0x30050700://事件记录表2
    {
        return 0;
    }
    case 0x30050800://事件记录表3
    {
        return 0;
    }
    case 0x30050900://事件记录表4
    {
        return 0;
    }
    case 0x30050A00://当前值记录表
    {
        return 0;
    }
    case 0x30050B00://上报标识
    {
        *p++ = TYPE_enum;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_type, p, 0x01);
        p++;
        return (p - buf);
    }
    case 0x30050C00://有效标识   固定1
    {
        *p++ = TYPE_bool;
        *p++ = 1;
        return (p - buf);
    }
    case 0x30050E00://时间状态记录表
    {
        return 0;
    }
    case 0x30050F00://上报方式
    {
        *p++ = TYPE_enum;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_mode, p, 0x01);
        p++;
        return (p - buf);
    }


    default:
        break;
    }
    return 0x01;
}
