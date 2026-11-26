#ifndef ND_TARGETS_STATEGRID_METER_BOARD_SPI_DRIVER_H
#define ND_TARGETS_STATEGRID_METER_BOARD_SPI_DRIVER_H

#include <stdint.h>

#include "board_state.h"

typedef enum {
    ND_BOARD_SPI_FLASH = 0,
    ND_BOARD_SPI_ESAM,
} nd_board_spi_t;

void nd_board_spi_init(nd_board_spi_t bus);
void nd_board_spi_select(stategrid_board_state_t *state, nd_board_spi_t bus,
                         int active);
/* Clock any bytes staged by the DMA transport before chip select rises. */
void nd_board_spi_flush(stategrid_board_state_t *state, nd_board_spi_t bus);
int nd_board_spi_transfer(stategrid_board_state_t *state, nd_board_spi_t bus,
                          const uint8_t *tx, uint8_t *rx, uint16_t len);

#endif
