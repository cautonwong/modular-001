#ifndef ND_DOMAIN_COMMON_STORE_H
#define ND_DOMAIN_COMMON_STORE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Consumer-defined persistence port. The domain asks for byte ranges at an
 * offset. Infra maps that to EEPROM, flash, or a RAM buffer in tests.
 */
typedef struct nd_store {
    void *ctx;
    int (*read)(void *ctx, uint32_t offset, uint8_t *buffer, size_t length);
    int (*write)(void *ctx, uint32_t offset, const uint8_t *buffer, size_t length);
} nd_store_t;

static inline int nd_store_read(const nd_store_t *store, uint32_t offset,
                                uint8_t *buffer, size_t length)
{
    if (store == NULL || store->read == NULL) {
        return -1;
    }
    return store->read(store->ctx, offset, buffer, length);
}

static inline int nd_store_write(const nd_store_t *store, uint32_t offset,
                                 const uint8_t *buffer, size_t length)
{
    if (store == NULL || store->write == NULL) {
        return -1;
    }
    return store->write(store->ctx, offset, buffer, length);
}

#endif
