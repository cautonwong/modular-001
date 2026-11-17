#include "targets/stategrid_meter/board/board_eeprom.h"

#include "cpu_surport.h"
#include "targets/stategrid_meter/board/iic_driver.h"

#define ND_EEPROM_ERROR_JUDGE 5u

uint8_t DEVICE_ERR = 0;
static uint8_t error_streak;

static void note_result(int ok)
{
    if (ok) {
        error_streak = 0;
        DEVICE_ERR &= (uint8_t)~(1u << 1);
        return;
    }
    if (error_streak < ND_EEPROM_ERROR_JUDGE) {
        error_streak++;
    }
    if (error_streak >= ND_EEPROM_ERROR_JUDGE) {
        DEVICE_ERR |= (uint8_t)(1u << 1);
    }
}

static int bus_write(void *ctx, uint8_t device, uint16_t addr,
                     const uint8_t *buf, uint16_t len)
{
    uint8_t prefix[2] = { (uint8_t)(addr >> 8), (uint8_t)addr };
    int result;

    (void)ctx;
    result = nd_board_iic_write(device, prefix, sizeof(prefix), buf, len);
    note_result(result == 0);
    return result;
}

static int bus_read(void *ctx, uint8_t device, uint16_t addr, uint8_t *buf,
                    uint16_t len)
{
    uint8_t prefix[2] = { (uint8_t)(addr >> 8), (uint8_t)addr };
    int result;

    (void)ctx;
    result = nd_board_iic_read(device, prefix, sizeof(prefix), buf, len);
    note_result(result == 0);
    return result;
}

static void bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    mcu_clear_watchdog();
    mcu_delay_us((uint16_t)us);
}

static nd_eeprom_storage_t eeprom_storage;
static nd_eeprom_t *eeprom;

static const nd_eeprom_config_t eeprom_config = {
    .bus = {
        .ctx = NULL,
        .write = bus_write,
        .read = bus_read,
        .delay_us = bus_delay,
    },
    .device_address = 0xA0,
    .page_size = 0x80,
    .size = BOARD_EEPROM_SIZE,
};

nd_eeprom_t *board_eeprom(void)
{
    if (eeprom == NULL) {
        nd_board_iic_init();
        eeprom = nd_eeprom_init(&eeprom_storage, &eeprom_config);
    }
    return eeprom;
}
