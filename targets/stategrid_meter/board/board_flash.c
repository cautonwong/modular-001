#include "targets/stategrid_meter/board/board_flash.h"

#include "board_state.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/board/spi_driver.h"
#include "targets/stategrid_meter/board/spi_pins.h"

static int bus_select(void *ctx, int active)
{
    if (ctx == NULL) {
        return -1;
    }
    nd_board_spi_select((stategrid_board_state_t *)ctx, ND_BOARD_SPI_FLASH,
                        active);
    return 0;
}

static int bus_transfer(void *ctx, const uint8_t *tx, uint16_t tx_len,
                        uint8_t *rx, uint16_t rx_len)
{
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;

    if (state == NULL) {
        return -1;
    }
    if (tx_len != 0
        && nd_board_spi_transfer(state, ND_BOARD_SPI_FLASH, tx, NULL,
                                 tx_len) != 0) {
        return -1;
    }
    if (rx_len != 0
        && nd_board_spi_transfer(state, ND_BOARD_SPI_FLASH, NULL, rx,
                                 rx_len) != 0) {
        return -1;
    }
    return 0;
}

static void bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    mcu_delay_us((uint16_t)us);
}

_Static_assert(BOARD_FLASH_SIZE % BOARD_FLASH_SECTOR == 0,
               "flash capacity must be a whole number of sectors");

nd_mx25l3206e_t *board_flash(struct stategrid_board_state *state)
{
    if (state == NULL) {
        return NULL;
    }
    if (state->flash == NULL) {
        nd_mx25l3206e_config_t config = {
            .bus = {
                .ctx = state,
                .select = bus_select,
                .transfer = bus_transfer,
                .delay_us = bus_delay,
            },
            .size = BOARD_FLASH_SIZE,
        };

        nd_spi_flash_power_set(1);
        nd_board_spi_init(ND_BOARD_SPI_FLASH);
        state->flash = nd_mx25l3206e_init(&state->flash_storage, &config);
    }
    return state->flash;
}
