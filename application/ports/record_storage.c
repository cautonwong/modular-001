#include "application/ports/record_storage.h"

#include <stddef.h>

int nd_record_storage_save_index(const nd_record_storage_port_t *port,
                                 uint32_t oad, uint32_t *index)
{
    if (port == NULL || port->save_index == NULL) {
        return -1;
    }
    return port->save_index(port->ctx, oad, index);
}

int nd_record_storage_change_format(const nd_record_storage_port_t *port,
                                    uint8_t *des, const uint8_t *src,
                                    uint32_t oad_req, uint32_t oad_src,
                                    int mode)
{
    if (port == NULL || port->change_format == NULL) {
        return -1;
    }
    return port->change_format(port->ctx, des, src, oad_req, oad_src, mode);
}

int nd_record_storage_clear_program_list(const nd_record_storage_port_t *port,
                                         void *pipeline, void *mesg)
{
    if (port == NULL || port->clear_program_list == NULL) {
        return -1;
    }
    return port->clear_program_list(port->ctx, pipeline, mesg);
}

int nd_record_storage_trigger_645_record(
    const nd_record_storage_port_t *port, uint32_t di, uint8_t tag)
{
    if (port == NULL || port->trigger_645_record == NULL) {
        return -1;
    }
    return port->trigger_645_record(port->ctx, di, tag);
}
