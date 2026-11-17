#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_FLASH_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_FLASH_H

#include <stdint.h>

#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"

/* The legacy dev_info advertised this capacity, not the chip's 4 MiB. */
#define BOARD_FLASH_SIZE 0x7FFFFFu
#define BOARD_FLASH_SECTOR 4096u

nd_mx25l3206e_t *board_flash(void);

#endif
