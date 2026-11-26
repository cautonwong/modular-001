#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_FLASH_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_FLASH_H

#include <stdint.h>

#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"

/* MX25L3206E is 32 Mbit (4 MiB); the legacy dev_info advertised 0x7FFFFF,
 * which is also not a whole number of sectors. */
#define BOARD_FLASH_SIZE 0x400000u
#define BOARD_FLASH_SECTOR 4096u

struct stategrid_board_state;

nd_mx25l3206e_t *board_flash(struct stategrid_board_state *state);

#endif
