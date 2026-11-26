#include "application/ports/resource_access.h"

#include <stddef.h>

int16_t nd_log_output_port(const nd_resource_port_t *port, uint32_t index,
                           uint8_t *buf, uint16_t buf_size,
                           struct circular_info_t *info)
{
    if (port == NULL || port->log_output == NULL) {
        return -1;
    }
    return port->log_output(port->ctx, index, buf, buf_size, info);
}

int nd_log_clean_port(const nd_resource_port_t *port, uint32_t index)
{
    if (port == NULL || port->log_clean == NULL) {
        return -1;
    }
    return port->log_clean(port->ctx, index);
}

int16_t nd_log_append_port(const nd_resource_port_t *port, uint32_t index,
                           uint8_t *buf, uint16_t unit_size)
{
    if (port == NULL || port->log_append == NULL) {
        return -1;
    }
    return port->log_append(port->ctx, index, buf, unit_size);
}

int16_t nd_log_append_revise_port(const nd_resource_port_t *port,
                                  uint32_t index, uint8_t *buf,
                                  uint16_t offset, uint16_t unit_size)
{
    if (port == NULL || port->log_append_revise == NULL) {
        return -1;
    }
    return port->log_append_revise(port->ctx, index, buf, offset, unit_size);
}

int16_t nd_get_index_port(const nd_resource_port_t *port, uint32_t index,
                          uint8_t *buf, uint16_t size)
{
    if (port == NULL || port->get_index == NULL) {
        return -1;
    }
    return port->get_index(port->ctx, index, buf, size);
}

int16_t nd_set_index_port(const nd_resource_port_t *port, uint32_t index,
                          uint8_t *buf, uint16_t size)
{
    if (port == NULL || port->set_index == NULL) {
        return -1;
    }
    return port->set_index(port->ctx, index, buf, size);
}

int16_t nd_get_offset_port(const nd_resource_port_t *port, uint32_t offset,
                           uint8_t *buf, uint16_t len)
{
    if (port == NULL || port->get_offset == NULL) {
        return -1;
    }
    return port->get_offset(port->ctx, offset, buf, len);
}

int16_t nd_set_offset_port(const nd_resource_port_t *port, uint32_t offset,
                           uint8_t *buf, uint16_t len)
{
    if (port == NULL || port->set_offset == NULL) {
        return -1;
    }
    return port->set_offset(port->ctx, offset, buf, len);
}

int16_t nd_get_index_e2p_port(const nd_resource_port_t *port, uint8_t *buf,
                              uint8_t block_name, uint16_t id, uint16_t len)
{
    if (port == NULL || port->get_index_e2p == NULL) {
        return -1;
    }
    return port->get_index_e2p(port->ctx, buf, block_name, id, len);
}

int16_t nd_set_index_e2p_port(const nd_resource_port_t *port, uint8_t *buf,
                              uint8_t block_name, uint16_t id, uint16_t len)
{
    if (port == NULL || port->set_index_e2p == NULL) {
        return -1;
    }
    return port->set_index_e2p(port->ctx, buf, block_name, id, len);
}

int16_t nd_set_offset_flash_port(const nd_resource_port_t *port,
                                 uint8_t blockname, uint32_t addr_offset,
                                 uint8_t *buf, uint16_t len,
                                 uint8_t check_erase)
{
    if (port == NULL || port->set_offset_flash == NULL) {
        return -1;
    }
    return port->set_offset_flash(port->ctx, blockname, addr_offset, buf, len,
                                  check_erase);
}

int16_t nd_get_offset_flash_port(const nd_resource_port_t *port,
                                 uint8_t blockname, uint32_t addr_offset,
                                 uint8_t *buf, uint16_t len)
{
    if (port == NULL || port->get_offset_flash == NULL) {
        return -1;
    }
    return port->get_offset_flash(port->ctx, blockname, addr_offset, buf, len);
}
