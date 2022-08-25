#include "nidavellir.h"
#include "fr_698.h"


int _record_sel_1(struct instance_t* instance, uint32_t oad, uint8_t* buf, uint8_t last_no, void *info)
{
    return 1;
}

int GET_record_sel_1(struct instance_t* instance, uint32_t oad, uint8_t* buf, struct rsd_info_t *rsd_info, uint32_t *rcsd_info, uint16_t buf_size,  void *ctx)
{
    return record_data_package(instance, oad, buf, rsd_info, rcsd_info,  buf_size,  ctx);
}
