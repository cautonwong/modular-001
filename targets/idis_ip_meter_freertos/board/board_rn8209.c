#include "targets/idis_ip_meter_freertos/board/board_rn8209.h"

#include <stddef.h>

#include "cpu_surport.h"
#include "targets/idis_ip_meter_freertos/board/rn8209_pins.h"
#include "targets/idis_ip_meter_freertos/board/uart_driver.h"
#include "lib_ana.h"
#include "lib_uart.h"

/* 6 MHz crystal on this board, the same factor the target calibration uses. */
#define ND_BOARD_RN8209_FR_K 1.676191
#define ND_BOARD_RN8209_UART 3

static const UART_InitType rn8209_uart_config = {
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    4800 * ND_BOARD_RN8209_FR_K,
};

static int uart_ready;

static void uart_prepare(void)
{
    nd_board_uart_init(ND_BOARD_RN8209_UART, &rn8209_uart_config);
    uart_ready = 1;
}

static int bus_write(void *ctx, const uint8_t *buf, uint16_t len, int hold_tx)
{
    size_t index = ND_BOARD_RN8209_UART;
    int result = 0;
    uint16_t i;

    (void)ctx;
    if (!uart_ready) {
        uart_prepare();
    }
    nd_board_uart_tx_enable(index, 2);
    for (i = 0; i < len; i++) {
        long timeout = 2000;

        if (nd_board_uart_write(index, buf[i]) != 0) {
            result = -1;
            break;
        }
        while (!nd_board_uart_tx_done(index) && timeout > 1) {
            if (ANA_GetStatus(ANA_STATUS_VDCINDROP)) {
                return ND_RN8209_BUS_POWER_DOWN;
            }
            timeout--;
        }
    }
    if (!hold_tx) {
        nd_board_uart_tx_enable(index, 0);
    }
    return result;
}

static int bus_read(void *ctx, uint8_t *buf, uint16_t len)
{
    size_t index = ND_BOARD_RN8209_UART;
    uint16_t i;

    (void)ctx;
    nd_board_uart_rx_enable(index, 2);
    nd_board_uart_tx_enable(index, 0);
    for (i = 0; i < len; i++) {
        long timeout = 5000;

        while (!nd_board_uart_rx_done(index) && timeout > 1) {
            if (ANA_GetStatus(ANA_STATUS_VDCINDROP)) {
                return ND_RN8209_BUS_POWER_DOWN;
            }
            timeout--;
        }
        if (nd_board_uart_read(index, &buf[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static void bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    mcu_delay_us((uint16_t)us);
}

static void reset_line(void)
{
    nd_rn8209_reset_line_output();
    nd_rn8209_reset_line_set(1);
    mcu_clear_watchdog();
    mcu_delay_us(30000);
    nd_rn8209_reset_line_set(0);
    mcu_clear_watchdog();
    mcu_delay_us(30000);
    nd_rn8209_reset_line_set(1);
    mcu_delay_us(10000);
    nd_rn8209_reset_line_input();
    mcu_clear_watchdog();
    mcu_delay_us(50000);
    mcu_clear_watchdog();
}

static void bus_reset(void *ctx, int power_on)
{
    (void)ctx;
    reset_line();
    if (power_on) {
        nd_rn8209_tx_line_output();
        nd_rn8209_tx_line_set(1);
        uart_prepare();
    }
}

static nd_rn8209_storage_t storage;
static nd_rn8209_t *dev;

static const nd_rn8209_config_t config = {
    .bus = {
        .ctx = NULL,
        .write = bus_write,
        .read = bus_read,
        .delay_us = bus_delay,
        .reset = bus_reset,
    },
};

nd_rn8209_t *board_rn8209(void)
{
    if (dev == NULL) {
        dev = nd_rn8209_init(&storage, &config);
    }
    return dev;
}
