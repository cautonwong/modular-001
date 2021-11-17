#include "nidavellir.h"
#include "fr_698.h"



int GET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    uint8_t* p = buf;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (id)
    {
    case 0x30050200://关联对象属性表
    {
        return 0;
    }
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
        obj->block_name = 0x0A;//只是举例
        obj->device_name = 0x0D;//只是举例
        //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
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
        *p++ = DTC_ENUM;
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
        *p++ = DTC_ENUM;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_mode, p, 0x01);
        p++;
        return (p - buf);
    }


    default:
        break;
    }
    return 0x01;
}
