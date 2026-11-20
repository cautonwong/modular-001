#ifndef ND_INFRA_DRIVERS_STORAGE_BLOCK_BLOCK_OPS_H
#define ND_INFRA_DRIVERS_STORAGE_BLOCK_BLOCK_OPS_H

#include <stdint.h>

#include "infra/drivers/storage/eeprom/eeprom.h"
#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"
#include "infra/storage/block.h"

/* EEPROM is byte addressable (the driver splits pages), so it erases by
 * writing 0xFF and reports erase_unit 1. */
typedef struct {
    nd_block_ops_t ops;
    nd_eeprom_t *device;
} nd_eeprom_block_t;

void nd_eeprom_block_init(nd_eeprom_block_t *block, nd_eeprom_t *device,
                          uint32_t size);
nd_block_ops_t *nd_eeprom_block_port(nd_eeprom_block_t *block);

/* NOR flash programs 1 to 0 and erases 4 KiB sectors. */
typedef struct {
    nd_block_ops_t ops;
    nd_mx25l3206e_t *device;
} nd_flash_block_t;

void nd_flash_block_init(nd_flash_block_t *block, nd_mx25l3206e_t *device,
                         uint32_t size);
nd_block_ops_t *nd_flash_block_port(nd_flash_block_t *block);

#endif
