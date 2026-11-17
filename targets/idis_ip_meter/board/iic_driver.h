#ifndef ND_TARGETS_STATEGRID_METER_BOARD_IIC_DRIVER_H
#define ND_TARGETS_STATEGRID_METER_BOARD_IIC_DRIVER_H

#include <stdint.h>

void nd_board_iic_init(void);
int nd_board_iic_write(uint8_t device, const uint8_t *prefix,
                       uint16_t prefix_len, const uint8_t *buf, uint16_t len);
int nd_board_iic_read(uint8_t device, const uint8_t *prefix,
                      uint16_t prefix_len, uint8_t *buf, uint16_t len);

#endif
