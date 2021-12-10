#include "nidavellir.h"
#include "fr_698.h"



int GET_7_Aggregate(struct instance_t* instance, uint32_t id, uint8_t* buf)
{   
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    return 0x01;
}
