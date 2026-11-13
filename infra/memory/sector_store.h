#ifndef ND_INFRA_MEMORY_SECTOR_STORE_H
#define ND_INFRA_MEMORY_SECTOR_STORE_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/store.h"

/*
 * Byte store over a NOR-like device: reads are direct, writes are
 * read-modify-erase-write of one sector. The caller owns the sector buffer.
 */
typedef int (*nd_sector_read_fn)(void *ctx, uint32_t offset, uint8_t *buffer,
                                 size_t length);
typedef int (*nd_sector_erase_fn)(void *ctx, uint32_t offset);
typedef int (*nd_sector_write_fn)(void *ctx, uint32_t offset,
                                  const uint8_t *buffer, size_t length);

typedef struct {
    nd_store_t port;
    void *ctx;
    nd_sector_read_fn read;
    nd_sector_erase_fn erase;
    nd_sector_write_fn write;
    uint32_t capacity;
    uint32_t sector_size;
    uint8_t *sector;
} nd_sector_store_t;

void nd_sector_store_init(nd_sector_store_t *store, void *ctx,
                          nd_sector_read_fn read, nd_sector_erase_fn erase,
                          nd_sector_write_fn write, uint32_t capacity,
                          uint32_t sector_size, uint8_t *sector_buffer);
nd_store_t *nd_sector_store_port(nd_sector_store_t *store);

#endif
