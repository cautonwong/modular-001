#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_EEPROM_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_EEPROM_H

#include <stdint.h>

#include "infra/drivers/storage/eeprom/eeprom.h"
#include "targets/stategrid_meter/board/board_state.h"

#define BOARD_EEPROM_SIZE 0xFFFFu

nd_eeprom_t *board_eeprom(stategrid_board_state_t *state);
/* Bit 1 of DEVICE_ERR, the EEPROM fault the platform port reports. */
uint8_t nd_board_device_error(const stategrid_board_state_t *state);

#endif
