#include "targets/idis_ip_meter_freertos/board/board_eeprom.h"

#include "board_state.h"
#include "cpu_surport.h"
#include "targets/idis_ip_meter_freertos/board/iic_driver.h"

#define ND_EEPROM_ERROR_JUDGE 5u

static void note_result(struct nd_idis_board_state *state, int ok)
{
    if (state == NULL) {
        return;
    }
    if (ok) {
        state->error_streak = 0;
        state->DEVICE_ERR &= (uint8_t)~(1u << 1);
        return;
    }
    if (state->error_streak < ND_EEPROM_ERROR_JUDGE) {
        state->error_streak++;
    }
    if (state->error_streak >= ND_EEPROM_ERROR_JUDGE) {
        state->DEVICE_ERR |= (uint8_t)(1u << 1);
    }
}

static int bus_write(void *ctx, uint8_t device, uint16_t addr,
                     const uint8_t *buf, uint16_t len)
{
    struct nd_idis_board_state *state = (struct nd_idis_board_state *)ctx;
    uint8_t prefix[2] = { (uint8_t)(addr >> 8), (uint8_t)addr };
    int result;

    if (state == NULL) {
        return -1;
    }
    result = nd_board_iic_write(device, prefix, sizeof(prefix), buf, len);
    note_result(state, result == 0);
    return result;
}

static int bus_read(void *ctx, uint8_t device, uint16_t addr, uint8_t *buf,
                    uint16_t len)
{
    struct nd_idis_board_state *state = (struct nd_idis_board_state *)ctx;
    uint8_t prefix[2] = { (uint8_t)(addr >> 8), (uint8_t)addr };
    int result;

    if (state == NULL) {
        return -1;
    }
    result = nd_board_iic_read(device, prefix, sizeof(prefix), buf, len);
    note_result(state, result == 0);
    return result;
}

static void bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    mcu_clear_watchdog();
    mcu_delay_us((uint16_t)us);
}

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

nd_eeprom_t *board_eeprom(struct nd_idis_board_state *state)
{
    if (state == NULL) {
        return NULL;
    }
    if (state->eeprom == NULL) {
        nd_eeprom_config_t config = eeprom_config;

        nd_board_iic_init();
        config.bus.ctx = state;
        state->eeprom = nd_eeprom_init(&state->eeprom_storage, &config);
    }
    return state->eeprom;
}
