#include "targets/stategrid_meter/board/board_esam.h"

#include <stddef.h>

#include "board_state.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/board/esam_pins.h"
#include "targets/stategrid_meter/board/spi_driver.h"
#include "lib_ana.h"

static int bus_transfer(void *ctx, uint8_t tx, uint8_t *rx)
{
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;

    if (state == NULL) {
        return -1;
    }
    if (rx == NULL) {
        (void)nd_board_spi_transfer(state, ND_BOARD_SPI_ESAM, &tx, NULL, 1);
        return 0;
    }
    if (ANA_GetStatus(ANA_STATUS_VDCINDROP)) {
        return ND_ESAM_BUS_POWER_DOWN;
    }
    mcu_clear_watchdog();
    (void)nd_board_spi_transfer(state, ND_BOARD_SPI_ESAM, NULL, rx, 1);
    return 0;
}

static void bus_cs(void *ctx, int high)
{
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;

    if (state == NULL) {
        return;
    }
    if (high) {
        nd_board_spi_flush(state, ND_BOARD_SPI_ESAM);
    }
    nd_esam_cs_set(high);
}

static void bus_power(void *ctx, int on)
{
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;

    if (state == NULL) {
        return;
    }
    if (on) {
        nd_esam_power_set(1);
        nd_board_spi_select(state, ND_BOARD_SPI_ESAM, 1);
        (void)mcu_delay_ms_power_down_quit(100);
        return;
    }
    nd_esam_power_set(0);
    nd_board_spi_select(state, ND_BOARD_SPI_ESAM, 0);
}

static void bus_delay(void *ctx, uint32_t us)
{
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;

    if (state == NULL) {
        return;
    }
    /* The driver holds CS after the last request byte; clock staged DMA
       before that hold so the chip sees the same transfer-then-hold order. */
    nd_board_spi_flush(state, ND_BOARD_SPI_ESAM);
    mcu_clear_watchdog();
    mcu_delay_us((uint16_t)us);
}

nd_esam_t *board_esam(struct stategrid_board_state *state)
{
    if (state == NULL) {
        return NULL;
    }
    if (state->esam == NULL) {
        nd_esam_config_t config = {
            .bus = {
                .ctx = state,
                .transfer = bus_transfer,
                .cs = bus_cs,
                .power = bus_power,
                .delay_us = bus_delay,
            },
        };

        state->esam = nd_esam_init(&state->esam_storage, &config);
    }
    return state->esam;
}
