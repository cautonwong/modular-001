#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_EEPROM_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_EEPROM_H

#include <stdint.h>

struct instance_t;
struct dev_info_t;

/* Device health bits: BIT0 ESAM, BIT1 EEPROM. */
extern uint8_t DEVICE_ERR;

int eeprom_initialize(struct instance_t *instance, const struct dev_info_t *dev);
int eeprom_read_chk(struct instance_t *instance, const struct dev_info_t *dev,
                    uint32_t addr, uint16_t len, void *src);
int eeprom_write_chk(struct instance_t *instance, const struct dev_info_t *dev,
                     uint32_t addr, uint16_t len, void *src);

#endif
