#include "nidavellir.h"
#include "fr_698.h"



int GET_1_Demand(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{    
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }

    return -DAR_OBJECT_NOT_EXIST;
}
