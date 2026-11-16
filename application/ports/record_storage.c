#include "application/ports/record_storage.h"

#include <stddef.h>

static const nd_record_storage_port_t *bound;

void nd_record_storage_bind(const nd_record_storage_port_t *port)
{
    bound = port;
}

int nd_record_storage_save_index(uint32_t oad, uint32_t *index)
{
    if (bound == NULL || bound->save_index == NULL) {
        return -1;
    }
    return bound->save_index(bound->ctx, oad, index);
}

int nd_record_storage_change_format(uint8_t *des, const uint8_t *src,
                                    uint32_t oad_req, uint32_t oad_src,
                                    int mode)
{
    if (bound == NULL || bound->change_format == NULL) {
        return -1;
    }
    return bound->change_format(bound->ctx, des, src, oad_req, oad_src, mode);
}

int nd_record_storage_clear_program_list(void *pipeline, void *mesg)
{
    if (bound == NULL || bound->clear_program_list == NULL) {
        return -1;
    }
    return bound->clear_program_list(bound->ctx, pipeline, mesg);
}

int nd_record_storage_trigger_645_record(uint32_t di, uint8_t tag)
{
    if (bound == NULL || bound->trigger_645_record == NULL) {
        return -1;
    }
    return bound->trigger_645_record(bound->ctx, di, tag);
}
