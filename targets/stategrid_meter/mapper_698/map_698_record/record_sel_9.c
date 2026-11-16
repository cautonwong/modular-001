#include "fr_698.h"
#include "application/adapters/dl698/meter_base_view.h"




int GET_record_sel_9(void *instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t buf_size, void *ctx)
{
    return record_data_package(instance, oad, buf, rsd_info, rcsd_info, buf_size,  ctx);
}
