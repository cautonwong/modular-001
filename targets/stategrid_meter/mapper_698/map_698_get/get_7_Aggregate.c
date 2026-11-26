#include "fr_698.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"



int GET_7_Aggregate(void *instance, uint32_t id, uint8_t* buf, void *ctx)
{   
    
    if(!nd_mb_valid(instance))
    {
        return -DAR_OTHERS_ERROR;
    }
    return -DAR_OBJECT_NOT_EXIST;
}
