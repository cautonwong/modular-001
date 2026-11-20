#include "infra/memory/sector_store.h"

#include <string.h>

static int store_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    nd_sector_store_t *store = ctx;
    if (store == NULL || store->read == NULL || store->sector == NULL
        || offset > store->capacity || length > store->capacity - offset) {
        return -1;
    }
    if (store->window_enabled == 0u || length == 0u
        || length > store->sector_size) {
        return store->read(store->ctx, offset, buffer, length);
    }
    uint32_t base = offset & ~(store->sector_size - 1u);
    uint32_t last = (offset + (uint32_t)length - 1u)
                    & ~(store->sector_size - 1u);
    if (last != base || store->capacity - base < store->sector_size) {
        return store->read(store->ctx, offset, buffer, length);
    }
    if (store->window_valid == 0u || store->window_base != base) {
        if (store->read(store->ctx, base, store->sector,
                        store->sector_size) != 0) {
            store->window_valid = 0u;
            return -1;
        }
        store->window_base = base;
        store->window_valid = 1u;
    }
    memcpy(buffer, store->sector + (offset - base), length);
    return 0;
}

static int store_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    nd_sector_store_t *store = ctx;
    if (store == NULL || store->read == NULL || store->erase == NULL
        || store->write == NULL || store->sector == NULL
        || offset > store->capacity || length > store->capacity - offset) {
        return -1;
    }
    store->window_valid = 0u;
    while (length > 0u) {
        uint32_t sector_base = offset & ~(store->sector_size - 1u);
        uint32_t in_sector = offset - sector_base;
        size_t chunk = store->sector_size - in_sector;
        if (chunk > length) {
            chunk = length;
        }
        if (store->read(store->ctx, sector_base, store->sector,
                        store->sector_size) != 0) {
            return -1;
        }
        if (memcmp(store->sector + in_sector, buffer, chunk) != 0) {
            int blank = 1;
            for (size_t index = 0; index < chunk; index++) {
                if (store->sector[in_sector + index] != 0xFFu) {
                    blank = 0;
                    break;
                }
            }
            if (blank) {
                if (store->write(store->ctx, offset, buffer, chunk) != 0) {
                    return -1;
                }
            } else {
                memcpy(store->sector + in_sector, buffer, chunk);
                if (store->erase(store->ctx, sector_base) != 0
                    || store->write(store->ctx, sector_base, store->sector,
                                    store->sector_size) != 0) {
                    return -1;
                }
            }
        }
        offset += (uint32_t)chunk;
        buffer += chunk;
        length -= chunk;
    }
    return 0;
}

static int store_erase(void *ctx, uint32_t offset, size_t length)
{
    nd_sector_store_t *store = ctx;
    if (store == NULL || store->erase == NULL || store->sector == NULL
        || (offset & (store->sector_size - 1u)) != 0
        || (length % store->sector_size) != 0
        || offset > store->capacity || length > store->capacity - offset) {
        return -1;
    }
    store->window_valid = 0u;
    while (length > 0u) {
        if (store->erase(store->ctx, offset) != 0) {
            return -1;
        }
        offset += store->sector_size;
        length -= store->sector_size;
    }
    return 0;
}

void nd_sector_store_init(nd_sector_store_t *store, void *ctx,
                          nd_sector_read_fn read, nd_sector_erase_fn erase,
                          nd_sector_write_fn write, uint32_t capacity,
                          uint32_t sector_size, uint8_t *sector_buffer,
                          uint8_t enable_window)
{
    if (store == NULL) {
        return;
    }
    /* The sector arithmetic relies on a nonzero power-of-two sector size, a
     * whole number of sectors, and a buffer that holds a sector. Invalid
     * geometry zeroes the device half but keeps the port wired, so every call
     * fails closed with -1. */
    int geometry_ok = sector_buffer != NULL && sector_size != 0u
                      && (sector_size & (sector_size - 1u)) == 0u
                      && capacity % sector_size == 0u;
    nd_sector_store_t valid = {
        .ctx = ctx,
        .read = read,
        .erase = erase,
        .write = write,
        .capacity = capacity,
        .sector_size = sector_size,
        .sector = sector_buffer,
        .window_enabled = enable_window,
        .port = {
            .ctx = store,
            .read = store_read,
            .write = store_write,
            .erase = store_erase,
        },
    };
    *store = geometry_ok ? valid : (nd_sector_store_t){
        .port = {
            .ctx = store,
            .read = store_read,
            .write = store_write,
            .erase = store_erase,
        },
    };
}

nd_store_t *nd_sector_store_port(nd_sector_store_t *store)
{
    return store != NULL ? &store->port : NULL;
}
