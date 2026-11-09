#ifndef ND_DOMAIN_COMMON_PARAMETER_STORE_H
#define ND_DOMAIN_COMMON_PARAMETER_STORE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Consumer-defined record store. The legacy parameter area addresses records
 * by a 16-bit id, so the port speaks ids instead of byte offsets.
 */
typedef struct nd_parameter_store {
    void *ctx;
    int (*read)(void *ctx, uint16_t id, uint8_t *buffer, size_t length);
    int (*write)(void *ctx, uint16_t id, const uint8_t *buffer, size_t length);
} nd_parameter_store_t;

static inline int nd_param_read(const nd_parameter_store_t *store, uint16_t id,
                                uint8_t *buffer, size_t length)
{
    if (store == NULL || store->read == NULL) {
        return -1;
    }
    return store->read(store->ctx, id, buffer, length);
}

static inline int nd_param_write(const nd_parameter_store_t *store, uint16_t id,
                                 const uint8_t *buffer, size_t length)
{
    if (store == NULL || store->write == NULL) {
        return -1;
    }
    return store->write(store->ctx, id, buffer, length);
}

#endif
