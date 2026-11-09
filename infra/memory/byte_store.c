#include "infra/memory/byte_store.h"

#include <string.h>

static int store_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    nd_byte_store_t *store = ctx;

    if (offset > store->size || length > store->size - offset) {
        return -1;
    }
    memcpy(buffer, store->data + offset, length);
    return 0;
}

static int store_write(void *ctx, uint32_t offset, const uint8_t *buffer, size_t length)
{
    nd_byte_store_t *store = ctx;

    if (offset > store->size || length > store->size - offset) {
        return -1;
    }
    memcpy(store->data + offset, buffer, length);
    return 0;
}

void nd_byte_store_init(nd_byte_store_t *store, void *data, size_t size)
{
    store->port.ctx = store;
    store->port.read = store_read;
    store->port.write = store_write;
    store->data = data;
    store->size = size;
}

nd_store_t *nd_byte_store_port(nd_byte_store_t *store)
{
    return &store->port;
}
