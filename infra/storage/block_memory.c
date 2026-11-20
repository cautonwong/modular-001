#include "infra/storage/block_memory.h"

#include <stdio.h>
#include <string.h>

static int ram_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    nd_block_ram_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memcpy(buffer, block->data + offset, length);
    return 0;
}

static int ram_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    nd_block_ram_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memcpy(block->data + offset, buffer, length);
    return 0;
}

static int ram_erase(void *ctx, uint32_t offset, uint32_t length)
{
    nd_block_ram_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memset(block->data + offset, 0xFF, length);
    return 0;
}

void nd_block_ram_init(nd_block_ram_t *block, uint8_t *data, uint32_t size)
{
    if (block == NULL || data == NULL) {
        return;
    }
    *block = (nd_block_ram_t){
        .ops = {
            .ctx = block,
            .read = ram_read,
            .program = ram_program,
            .erase = ram_erase,
            .size = size,
            .erase_unit = 1u,
        },
        .data = data,
    };
}

nd_block_ops_t *nd_block_ram_port(nd_block_ram_t *block)
{
    return block != NULL ? &block->ops : NULL;
}

static FILE *file_of(nd_block_file_t *block)
{
    return block->handle;
}

static int file_read(void *ctx, uint32_t offset, uint8_t *buffer,
                     size_t length)
{
    nd_block_file_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    if (fseek(file_of(block), (long)offset, SEEK_SET) != 0) {
        return -1;
    }
    return fread(buffer, 1u, length, file_of(block)) == length ? 0 : -1;
}

static int file_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    nd_block_file_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    if (fseek(file_of(block), (long)offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fwrite(buffer, 1u, length, file_of(block)) != length) {
        return -1;
    }
    return fflush(file_of(block)) == 0 ? 0 : -1;
}

static int file_erase(void *ctx, uint32_t offset, uint32_t length)
{
    nd_block_file_t *block = ctx;
    uint8_t blank[32];
    uint32_t done = 0;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memset(blank, 0xFF, sizeof(blank));
    if (fseek(file_of(block), (long)offset, SEEK_SET) != 0) {
        return -1;
    }
    while (done < length) {
        size_t chunk = length - done < sizeof(blank) ? length - done
                                                     : sizeof(blank);

        if (fwrite(blank, 1u, chunk, file_of(block)) != chunk) {
            return -1;
        }
        done += (uint32_t)chunk;
    }
    return fflush(file_of(block)) == 0 ? 0 : -1;
}

int nd_block_file_open(nd_block_file_t *block, const char *path, uint32_t size)
{
    FILE *file;
    uint8_t blank[32];
    long current;

    if (block == NULL || path == NULL) {
        return -1;
    }
    file = fopen(path, "r+b");
    if (file == NULL) {
        file = fopen(path, "w+b");
    }
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        (void)fclose(file);
        return -1;
    }
    current = ftell(file);
    memset(blank, 0xFF, sizeof(blank));
    while (current >= 0 && (uint32_t)current < size) {
        uint32_t remaining = size - (uint32_t)current;
        size_t chunk = remaining < sizeof(blank) ? remaining : sizeof(blank);

        if (fwrite(blank, 1u, chunk, file) != chunk) {
            (void)fclose(file);
            return -1;
        }
        current += (long)chunk;
    }
    *block = (nd_block_file_t){
        .ops = {
            .ctx = block,
            .read = file_read,
            .program = file_program,
            .erase = file_erase,
            .size = size,
            .erase_unit = 1u,
        },
        .handle = file,
    };
    return 0;
}

void nd_block_file_close(nd_block_file_t *block)
{
    if (block == NULL || block->handle == NULL) {
        return;
    }
    (void)fclose(block->handle);
    block->handle = NULL;
}

nd_block_ops_t *nd_block_file_port(nd_block_file_t *block)
{
    return block != NULL ? &block->ops : NULL;
}
