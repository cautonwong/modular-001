#include "infra/memory/file_store.h"

#include <string.h>

static int file_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    nd_file_store_t *store = ctx;

    if (store == NULL || store->file == NULL || buffer == NULL
        || offset > store->size || length > store->size - offset) {
        return -1;
    }
    if (fseek(store->file, (long)offset, SEEK_SET) != 0) {
        return -1;
    }
    return fread(buffer, 1, length, store->file) == length ? 0 : -1;
}

static int file_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                      size_t length)
{
    nd_file_store_t *store = ctx;

    if (store == NULL || store->file == NULL || buffer == NULL
        || offset > store->size || length > store->size - offset) {
        return -1;
    }
    if (fseek(store->file, (long)offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fwrite(buffer, 1, length, store->file) != length) {
        return -1;
    }
    return fflush(store->file) == 0 ? 0 : -1;
}

int nd_file_store_open(nd_file_store_t *store, const char *path, uint32_t size)
{
    static const uint8_t zeros[256] = {0};
    uint32_t remaining;

    if (store == NULL || path == NULL || size == 0) {
        return -1;
    }
    memset(store, 0, sizeof(*store));
    store->file = fopen(path, "r+b");
    if (store->file == NULL) {
        store->file = fopen(path, "w+b");
    }
    if (store->file == NULL) {
        return -1;
    }
    if (fseek(store->file, 0, SEEK_END) != 0) {
        nd_file_store_close(store);
        return -1;
    }
    long current = ftell(store->file);
    if (current < 0) {
        nd_file_store_close(store);
        return -1;
    }
    remaining = (uint32_t)current < size ? size - (uint32_t)current : 0;
    while (remaining > 0) {
        size_t chunk = remaining > sizeof(zeros) ? sizeof(zeros) : remaining;
        if (fwrite(zeros, 1, chunk, store->file) != chunk) {
            nd_file_store_close(store);
            return -1;
        }
        remaining -= (uint32_t)chunk;
    }
    if (fflush(store->file) != 0) {
        nd_file_store_close(store);
        return -1;
    }
    store->size = size;
    store->port.ctx = store;
    store->port.read = file_read;
    store->port.write = file_write;
    return 0;
}

int nd_file_store_close(nd_file_store_t *store)
{
    int result = 0;

    if (store == NULL) {
        return -1;
    }
    if (store->file != NULL) {
        result = fclose(store->file);
        store->file = NULL;
    }
    store->size = 0;
    return result;
}

nd_store_t *nd_file_store_port(nd_file_store_t *store)
{
    return store == NULL ? NULL : &store->port;
}
