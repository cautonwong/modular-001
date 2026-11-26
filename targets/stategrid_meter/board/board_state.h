#ifndef ND_TARGET_STATEGRID_BOARD_STATE_H
#define ND_TARGET_STATEGRID_BOARD_STATE_H

#include <stdint.h>

#include "board_transport.h"
#include "uart_driver.h"
#include "components/core/fifo/fifo.h"
#include "infra/drivers/metering/rn8209/rn8209.h"
#include "infra/drivers/storage/eeprom/eeprom.h"
#include "infra/drivers/security/esam/esam.h"
#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"

typedef struct stategrid_board_state {
    nd_eeprom_storage_t eeprom_storage;
    nd_eeprom_t *eeprom;
    uint8_t DEVICE_ERR;
    uint8_t error_streak;
    nd_mx25l3206e_storage_t flash_storage;
    nd_mx25l3206e_t *flash;
    nd_esam_storage_t esam_storage;
    nd_esam_t *esam;
    struct uart_chl_init_t uart_dev[7];
    nd_rn8209_storage_t rn8209_storage;
    nd_rn8209_t *rn8209;
    int uart_ready;
    uint8_t irq_fifo_buf[32];
    struct fifo_t irq_fifo;
    uint8_t meter_cover_cnt;
    uint8_t wpulse_light_flag;
    uint8_t wpulse_light_open;
    uint8_t wpulse_light_open_flag;
    uint8_t wplse_light_width;
#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ
    nd_board_uart_rx_t uart_rx[ND_BOARD_UART_CHANNELS];
    nd_board_op_t uart_tx[ND_BOARD_UART_CHANNELS];
    struct {
        struct stategrid_board_state *state;
        uint8_t index;
    } uart_irq[ND_BOARD_UART_CHANNELS];
#endif
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    nd_board_spi_dma_t spi_dma;
    uint8_t spi_dma_ready;
    nd_board_spi_dma_t esam_dma;
    uint8_t esam_dma_ready;
    uint8_t esam_tx_stage[ND_BOARD_SPI_DMA_CHUNK];
    uint8_t esam_rx_sink[ND_BOARD_SPI_DMA_CHUNK];
    uint16_t esam_tx_used;
#endif
} stategrid_board_state_t;

void stategrid_board_init(stategrid_board_state_t *state);
stategrid_board_state_t *stategrid_board_root(void);
stategrid_board_state_t *stategrid_context_board(void *instance);

#endif
