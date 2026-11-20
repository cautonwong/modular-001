#include "infra/drivers/storage/block/block_ops.h"

#include <string.h>

#define ND_BLOCK_ERASE_CHUNK 32u

static int eeprom_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    nd_eeprom_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset
        || length > 0xFFFFu) {
        return -1;
    }
    return nd_eeprom_read(block->device, offset, buffer, (uint16_t)length);
}

static int eeprom_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                          size_t length)
{
    nd_eeprom_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset
        || length > 0xFFFFu) {
        return -1;
    }
    return nd_eeprom_write(block->device, offset, buffer, (uint16_t)length);
}

static int eeprom_erase(void *ctx, uint32_t offset, uint32_t length)
{
    nd_eeprom_block_t *block = ctx;
    uint8_t blank[ND_BLOCK_ERASE_CHUNK];
    uint32_t done = 0;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memset(blank, 0xFF, sizeof(blank));
    while (done < length) {
        uint32_t chunk = length - done < ND_BLOCK_ERASE_CHUNK
                             ? length - done
                             : ND_BLOCK_ERASE_CHUNK;

        if (nd_eeprom_write(block->device, offset + done, blank,
                            (uint16_t)chunk) != 0) {
            return -1;
        }
        done += chunk;
    }
    return 0;
}

void nd_eeprom_block_init(nd_eeprom_block_t *block, nd_eeprom_t *device,
                          uint32_t size)
{
    if (block == NULL || device == NULL) {
        return;
    }
    *block = (nd_eeprom_block_t){
        .ops = {
            .ctx = block,
            .read = eeprom_read,
            .program = eeprom_program,
            .erase = eeprom_erase,
            .size = size,
            .erase_unit = 1u,
        },
        .device = device,
    };
}

nd_block_ops_t *nd_eeprom_block_port(nd_eeprom_block_t *block)
{
    return block != NULL ? &block->ops : NULL;
}

static int flash_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    nd_flash_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset
        || length > 0xFFFFu) {
        return -1;
    }
    return nd_mx25l3206e_read(block->device, offset, buffer, (uint16_t)length);
}

static int flash_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    nd_flash_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset
        || length > 0xFFFFu) {
        return -1;
    }
    return nd_mx25l3206e_write(block->device, offset, buffer,
                               (uint16_t)length);
}

static int flash_erase(void *ctx, uint32_t offset, uint32_t length)
{
    nd_flash_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    return nd_mx25l3206e_erase(block->device, offset, length);
}

void nd_flash_block_init(nd_flash_block_t *block, nd_mx25l3206e_t *device,
                         uint32_t size)
{
    if (block == NULL || device == NULL) {
        return;
    }
    *block = (nd_flash_block_t){
        .ops = {
            .ctx = block,
            .read = flash_read,
            .program = flash_program,
            .erase = flash_erase,
            .size = size,
            .erase_unit = ND_MX25L3206E_SECTOR_SIZE,
        },
        .device = device,
    };
}

nd_block_ops_t *nd_flash_block_port(nd_flash_block_t *block)
{
    return block != NULL ? &block->ops : NULL;
}
