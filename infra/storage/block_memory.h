#ifndef ND_INFRA_STORAGE_BLOCK_MEMORY_H
#define ND_INFRA_STORAGE_BLOCK_MEMORY_H

#include <stdint.h>

#include "infra/storage/block.h"

typedef struct {
    nd_block_ops_t ops;
    uint8_t *data;
} nd_block_ram_t;

void nd_block_ram_init(nd_block_ram_t *block, uint8_t *data, uint32_t size);
nd_block_ops_t *nd_block_ram_port(nd_block_ram_t *block);

/* Host builds only. The cache is not unbuffered, so a crash test must reopen
 * the file between runs to observe the bytes a real device would keep. */
typedef struct {
    nd_block_ops_t ops;
    void *handle;
} nd_block_file_t;

int nd_block_file_open(nd_block_file_t *block, const char *path, uint32_t size);
void nd_block_file_close(nd_block_file_t *block);
nd_block_ops_t *nd_block_file_port(nd_block_file_t *block);

#endif
