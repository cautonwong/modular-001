#include "targets/stategrid_meter/board/board_eeprom.h"

#include "board_state.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/board/iic_driver.h"

#define ND_EEPROM_ERROR_JUDGE 5u

static void note_result(stategrid_board_state_t *state, int ok)
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
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;
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
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;
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

nd_eeprom_t *board_eeprom(stategrid_board_state_t *state)
{
    if (state == NULL) {
        return NULL;
    }
    if (state->eeprom == NULL) {
        nd_eeprom_config_t config = {
            .bus = {
                .ctx = state,
                .write = bus_write,
                .read = bus_read,
                .delay_us = bus_delay,
            },
            .device_address = 0xA0,
            .page_size = 0x80,
            .size = BOARD_EEPROM_SIZE,
        };

        nd_board_iic_init();
        state->eeprom = nd_eeprom_init(&state->eeprom_storage, &config);
    }
    return state->eeprom;
}

uint8_t nd_board_device_error(const stategrid_board_state_t *state)
{
    return state != NULL ? (uint8_t)((state->DEVICE_ERR >> 1) & 1u) : 0;
}
