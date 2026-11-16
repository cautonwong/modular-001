#ifndef ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_OPS_H
#define ND_INFRA_BOARD_STATEGRID_SINGLE_PHASE_BOARD_OPS_H

struct bus_ops_t;
struct driver_ops_t;

extern const struct bus_ops_t iic_bus;
extern const struct bus_ops_t spi_mode1;
extern const struct bus_ops_t spi_mode3;
extern const struct driver_ops_t eeprom_driver;
extern const struct driver_ops_t nor_flash_driver;
extern const struct driver_ops_t rn8209_drv;

#endif
