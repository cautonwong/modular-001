#include "nidavellir.h"
#include "fr_698.h"



int SET_3_Event(struct instance_t* instance, uint32_t id, uint8_t* buf, struct resources_obj_t* obj)
{    
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    switch (id)
    {
    case 0x30050500://过流阈值
        {
            //obj->block_name = 0x0A;//只是举例
            //obj->device_name = 0x0D;//只是举例
            //obj->offset = container_of(mbase_cb,struct mbase_control_block_t, UA);
            buf++;
            buf++;
            buf++;
            memcpy((uint8_t*)&mbase_cb->i_over_event_threshold, buf, 0x04);
            buf++;
            memcpy((uint8_t*)&mbase_cb->i_over_event_duration_para, buf, 0x01);
            return 0x05;
        }
    break;
    
    default:
        break;
    }
    return 0x01;
}
