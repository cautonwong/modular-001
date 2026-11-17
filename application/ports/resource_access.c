#include "application/ports/resource_access.h"

#include <stddef.h>

static const nd_resource_port_t *bound;

void nd_resource_bind(const nd_resource_port_t *port)
{
    bound = port;
}

/* The ctx argument stays on each accessor until the legacy call sites stop
 * passing the kernel instance; the adapter needs the bound product context. */

int16_t nd_log_output(void *ctx, uint32_t index, uint8_t *buf,
                      uint16_t buf_size, struct circular_info_t *info)
{
    (void)ctx;
    if (bound == NULL || bound->log_output == NULL) {
        return -1;
    }
    return bound->log_output(bound->ctx, index, buf, buf_size, info);
}

int nd_log_clean(void *ctx, uint32_t index)
{
    (void)ctx;
    if (bound == NULL || bound->log_clean == NULL) {
        return -1;
    }
    return bound->log_clean(bound->ctx, index);
}

int16_t nd_log_append(void *ctx, uint32_t index, uint8_t *buf,
                      uint16_t unit_size)
{
    (void)ctx;
    if (bound == NULL || bound->log_append == NULL) {
        return -1;
    }
    return bound->log_append(bound->ctx, index, buf, unit_size);
}

int16_t nd_log_append_revise(void *ctx, uint32_t index, uint8_t *buf,
                             uint16_t offset, uint16_t unit_size)
{
    (void)ctx;
    if (bound == NULL || bound->log_append_revise == NULL) {
        return -1;
    }
    return bound->log_append_revise(bound->ctx, index, buf, offset, unit_size);
}

int16_t nd_get_index(void *ctx, uint32_t index, uint8_t *buf, uint16_t size)
{
    (void)ctx;
    if (bound == NULL || bound->get_index == NULL) {
        return -1;
    }
    return bound->get_index(bound->ctx, index, buf, size);
}

int16_t nd_set_index(void *ctx, uint32_t index, uint8_t *buf, uint16_t size)
{
    (void)ctx;
    if (bound == NULL || bound->set_index == NULL) {
        return -1;
    }
    return bound->set_index(bound->ctx, index, buf, size);
}

int16_t nd_get_offset(void *ctx, uint32_t offset, uint8_t *buf, uint16_t len)
{
    (void)ctx;
    if (bound == NULL || bound->get_offset == NULL) {
        return -1;
    }
    return bound->get_offset(bound->ctx, offset, buf, len);
}

int16_t nd_set_offset(void *ctx, uint32_t offset, uint8_t *buf, uint16_t len)
{
    (void)ctx;
    if (bound == NULL || bound->set_offset == NULL) {
        return -1;
    }
    return bound->set_offset(bound->ctx, offset, buf, len);
}

int16_t nd_get_index_e2p(void *ctx, uint8_t *buf, uint8_t block_name,
                         uint16_t id, uint16_t len)
{
    (void)ctx;
    if (bound == NULL || bound->get_index_e2p == NULL) {
        return -1;
    }
    return bound->get_index_e2p(bound->ctx, buf, block_name, id, len);
}

int16_t nd_set_index_e2p(void *ctx, uint8_t *buf, uint8_t block_name,
                         uint16_t id, uint16_t len)
{
    (void)ctx;
    if (bound == NULL || bound->set_index_e2p == NULL) {
        return -1;
    }
    return bound->set_index_e2p(bound->ctx, buf, block_name, id, len);
}

int16_t nd_set_offset_flash(void *ctx, uint8_t blockname,
                            uint32_t addr_offset, uint8_t *buf, uint16_t len,
                            uint8_t check_erase)
{
    (void)ctx;
    if (bound == NULL || bound->set_offset_flash == NULL) {
        return -1;
    }
    return bound->set_offset_flash(bound->ctx, blockname, addr_offset, buf,
                                   len, check_erase);
}

int16_t nd_get_offset_flash(void *ctx, uint8_t blockname,
                            uint32_t addr_offset, uint8_t *buf, uint16_t len)
{
    (void)ctx;
    if (bound == NULL || bound->get_offset_flash == NULL) {
        return -1;
    }
    return bound->get_offset_flash(bound->ctx, blockname, addr_offset, buf,
                                   len);
}
