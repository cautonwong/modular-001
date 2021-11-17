#include "nidavellir.h"
#include "fr_698.h"



int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
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
    case 0x30050500://过流阈值
        {
            //obj->block_name = 0x0A;//只是举例
            //obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            p++;
            p++;
            p++;
            memcpy((uint8_t*)&mbase_cb->i_over_event_threshold, p, 0x04);
            p += 4;
            p++;
            memcpy((uint8_t*)&mbase_cb->i_over_event_duration_para, p, 0x01);
            p++;
            return (p - buf);
        }
    case 0x30050B00://上报标识
    {
        p++;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_type, p, 0x01);
        p++;
        return (p - buf);
    }

    case 0x30050F00://上报方式
    {
        p++;
        memcpy((uint8_t*)&mbase_cb->i_over_event_report_mode, p, 0x01);
        p++;
        return (p - buf);
    }

    break;
    
    default:
        break;
    }
    return 0x01;
}
