#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_EEPROM_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_EEPROM_H

#include <stdint.h>

#include "infra/drivers/storage/eeprom/eeprom.h"

/* Bit 1 reports an EEPROM fault to the platform port. */
extern uint8_t DEVICE_ERR;

nd_eeprom_t *board_eeprom(void);

#endif
