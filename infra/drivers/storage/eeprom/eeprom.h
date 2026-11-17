#ifndef ND_INFRA_DRIVERS_STORAGE_EEPROM_EEPROM_H
#define ND_INFRA_DRIVERS_STORAGE_EEPROM_EEPROM_H

#include <stdint.h>

/* The bus is injected by the board. The driver knows registers and pages,
 * never pins, SoC headers or the legacy device model. */
typedef struct nd_eeprom_bus {
    void *ctx;
    int (*write)(void *ctx, uint8_t device, uint16_t addr,
                 const uint8_t *buf, uint16_t len);
    int (*read)(void *ctx, uint8_t device, uint16_t addr,
                uint8_t *buf, uint16_t len);
    void (*delay_us)(void *ctx, uint32_t us);
} nd_eeprom_bus_t;

typedef struct {
    nd_eeprom_bus_t bus;
    uint8_t device_address;
    uint16_t page_size;
    uint32_t size;
} nd_eeprom_config_t;

#define ND_EEPROM_STORAGE_SIZE 64

typedef union {
    uint64_t align;
    uint8_t bytes[ND_EEPROM_STORAGE_SIZE];
} nd_eeprom_storage_t;

typedef struct nd_eeprom nd_eeprom_t;

nd_eeprom_t *nd_eeprom_init(nd_eeprom_storage_t *storage,
                            const nd_eeprom_config_t *config);
int nd_eeprom_read(nd_eeprom_t *dev, uint32_t addr, uint8_t *buf, uint16_t len);
int nd_eeprom_write(nd_eeprom_t *dev, uint32_t addr, const uint8_t *buf,
                    uint16_t len);

#endif
