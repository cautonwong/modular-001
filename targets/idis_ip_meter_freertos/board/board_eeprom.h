#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_EEPROM_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_EEPROM_H

#include <stdint.h>

#include "infra/drivers/storage/eeprom/eeprom.h"

struct nd_idis_board_state;

#define BOARD_EEPROM_SIZE 0xFFFFu

nd_eeprom_t *board_eeprom(struct nd_idis_board_state *state);

#endif
