#ifndef ND_IDIS_BOARD_STATE_H
#define ND_IDIS_BOARD_STATE_H

#include <stdint.h>

#include "uart_driver.h"
#include "board_transport.h"
#include "domain/common/store.h"
#include "infra/drivers/metering/rn8209/rn8209.h"
#include "infra/drivers/storage/eeprom/eeprom.h"
#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"
#include "infra/memory/sector_store.h"

typedef struct {
    nd_board_uart_rx_t rx;
    nd_board_op_t tx;
    uint8_t index;
} nd_idis_uart_irq_t;

struct nd_idis_board_state {
    struct uart_chl_init_t uart_dev[MCU_UART_NUM];
#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ
    nd_idis_uart_irq_t uart[ND_BOARD_UART_CHANNELS];
#endif
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    nd_board_spi_dma_t spi_dma;
    uint8_t spi_dma_ready;
#endif
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    nd_board_spi_dma_t esam_dma;
    uint8_t esam_dma_ready;
    uint8_t esam_tx_stage[ND_BOARD_SPI_DMA_CHUNK];
    uint8_t esam_rx_sink[ND_BOARD_SPI_DMA_CHUNK];
    uint16_t esam_tx_used;
#endif
    uint8_t uart_ready;
    uint8_t DEVICE_ERR;
    uint8_t error_streak;
    volatile uint8_t key_event;
    nd_eeprom_storage_t eeprom_storage;
    nd_eeprom_t *eeprom;
    nd_store_t store;
    nd_mx25l3206e_storage_t flash_storage;
    nd_mx25l3206e_t *flash;
    nd_sector_store_t flash_store;
    nd_rn8209_storage_t rn8209_storage;
    nd_rn8209_t *rn8209;
};

#endif
