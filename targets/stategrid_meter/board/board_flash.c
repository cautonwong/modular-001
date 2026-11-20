#include "targets/stategrid_meter/board/board_flash.h"

#include "cpu_surport.h"
#include "targets/stategrid_meter/board/spi_driver.h"
#include "targets/stategrid_meter/board/spi_pins.h"

static int bus_select(void *ctx, int active)
{
    (void)ctx;
    nd_board_spi_select(ND_BOARD_SPI_FLASH, active);
    return 0;
}

static int bus_transfer(void *ctx, const uint8_t *tx, uint16_t tx_len,
                        uint8_t *rx, uint16_t rx_len)
{
    (void)ctx;
    if (tx_len != 0
        && nd_board_spi_transfer(ND_BOARD_SPI_FLASH, tx, NULL, tx_len) != 0) {
        return -1;
    }
    if (rx_len != 0
        && nd_board_spi_transfer(ND_BOARD_SPI_FLASH, NULL, rx, rx_len) != 0) {
        return -1;
    }
    return 0;
}

static void bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    mcu_delay_us((uint16_t)us);
}

static nd_mx25l3206e_storage_t flash_storage;
static nd_mx25l3206e_t *flash;

_Static_assert(BOARD_FLASH_SIZE % BOARD_FLASH_SECTOR == 0,
               "flash capacity must be a whole number of sectors");

static const nd_mx25l3206e_config_t flash_config = {
    .bus = {
        .ctx = NULL,
        .select = bus_select,
        .transfer = bus_transfer,
        .delay_us = bus_delay,
    },
    .size = BOARD_FLASH_SIZE,
};

nd_mx25l3206e_t *board_flash(void)
{
    if (flash == NULL) {
        nd_spi_flash_power_set(1);
        nd_board_spi_init(ND_BOARD_SPI_FLASH);
        flash = nd_mx25l3206e_init(&flash_storage, &flash_config);
    }
    return flash;
}
