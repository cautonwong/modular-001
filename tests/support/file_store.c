#define _POSIX_C_SOURCE 200809L

#include "support/file_store.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int store_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    file_store_t *store = ctx;
    size_t done = 0;

    if (store == NULL || store->fd < 0 ||
        offset > store->size || length > store->size - offset) {
        return -1;
    }
    while (done < length) {
        ssize_t count = pread(store->fd, buffer + done, length - done,
                              (off_t)offset + (off_t)done);
        if (count <= 0) {
            return -1;
        }
        done += (size_t)count;
    }
    return 0;
}

static int store_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    file_store_t *store = ctx;
    size_t done = 0;

    if (store == NULL || store->fd < 0 ||
        offset > store->size || length > store->size - offset) {
        return -1;
    }
    while (done < length) {
        ssize_t count = pwrite(store->fd, buffer + done, length - done,
                               (off_t)offset + (off_t)done);
        if (count <= 0) {
            return -1;
        }
        done += (size_t)count;
    }
    return 0;
}

static int ensure_parent_dir(const char *path)
{
    const char *slash = strrchr(path, '/');
    char dir[512];
    size_t length;

    if (slash == NULL || slash == path) {
        return 0;
    }
    length = (size_t)(slash - path);
    if (length >= sizeof(dir)) {
        return -1;
    }
    memcpy(dir, path, length);
    dir[length] = '\0';
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int file_store_open(file_store_t *store, const char *path, uint32_t size,
                    uint8_t erase_value)
{
    struct stat status;
    int fd;
    int resized = 0;

    if (store == NULL || path == NULL || size == 0) {
        return -1;
    }
    memset(store, 0, sizeof(*store));
    store->fd = -1;

    if (ensure_parent_dir(path) != 0) {
        return -1;
    }
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        return -1;
    }
    if (fstat(fd, &status) != 0) {
        close(fd);
        return -1;
    }
    if ((uint64_t)status.st_size != size) {
        if (ftruncate(fd, (off_t)size) != 0) {
            close(fd);
            return -1;
        }
        resized = 1;
    }

    store->port.ctx = store;
    store->port.read = store_read;
    store->port.write = store_write;
    store->fd = fd;
    store->size = size;
    store->erase_value = erase_value;

    if (resized && file_store_erase(store, 0, size) != 0) {
        file_store_close(store);
        return -1;
    }
    return 0;
}

void file_store_close(file_store_t *store)
{
    if (store == NULL) {
        return;
    }
    if (store->fd >= 0) {
        close(store->fd);
    }
    store->fd = -1;
}

nd_store_t *file_store_port(file_store_t *store)
{
    return store == NULL ? NULL : &store->port;
}

int file_store_erase(file_store_t *store, uint32_t offset, uint32_t length)
{
    uint8_t fill[256];
    uint32_t done = 0;

    if (store == NULL || store->fd < 0 ||
        offset > store->size || length > store->size - offset) {
        return -1;
    }
    memset(fill, store->erase_value, sizeof(fill));
    while (done < length) {
        uint32_t chunk = length - done;
        if (chunk > sizeof(fill)) {
            chunk = (uint32_t)sizeof(fill);
        }
        if (store_write(store, offset + done, fill, chunk) != 0) {
            return -1;
        }
        done += chunk;
    }
    return 0;
}
