#ifndef ND_BOARD_TRANSPORT_H
#define ND_BOARD_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

/*
 * Transport selection is a board compile-time decision. Products override
 * these on the board target; every product defaults to the polling path that
 * has HIL history. Plain integers because the selection is made by the
 * preprocessor.
 */
#define ND_BOARD_MODE_POLL 0
#define ND_BOARD_MODE_IRQ 1
#define ND_BOARD_MODE_DMA 2

#ifndef ND_BOARD_UART_MODE
#define ND_BOARD_UART_MODE ND_BOARD_MODE_POLL
#endif
#ifndef ND_BOARD_SPI_MODE
#define ND_BOARD_SPI_MODE ND_BOARD_MODE_POLL
#endif
#ifndef ND_BOARD_IIC_MODE
#define ND_BOARD_IIC_MODE ND_BOARD_MODE_POLL
#endif
#ifndef ND_BOARD_USE_FREERTOS
#define ND_BOARD_USE_FREERTOS 0
#endif

#if (ND_BOARD_UART_MODE < ND_BOARD_MODE_POLL) || (ND_BOARD_UART_MODE > ND_BOARD_MODE_DMA)
#error "ND_BOARD_UART_MODE is not a valid transport mode"
#endif
#if (ND_BOARD_SPI_MODE < ND_BOARD_MODE_POLL) || (ND_BOARD_SPI_MODE > ND_BOARD_MODE_DMA)
#error "ND_BOARD_SPI_MODE is not a valid transport mode"
#endif
#if (ND_BOARD_IIC_MODE < ND_BOARD_MODE_POLL) || (ND_BOARD_IIC_MODE > ND_BOARD_MODE_DMA)
#error "ND_BOARD_IIC_MODE is not a valid transport mode"
#endif

/* mcu_uart_* only accepts indices below 6. */
#define ND_BOARD_UART_CHANNELS 6
#define ND_BOARD_UART_RX_RING 128

#define ND_BOARD_OK 0
#define ND_BOARD_TIMEOUT (-1)
#define ND_BOARD_INVALID (-2)

/*
 * One __WFI wake is one SysTick. The V8500 programs SysTick at cpu_clk / 100,
 * so a wait step is 10 ms; the loop counts wakes, not wall time.
 */
#define ND_BOARD_SYSTICK_US 10000u

#ifndef ND_BOARD_UART_RX_TIMEOUT_US
#define ND_BOARD_UART_RX_TIMEOUT_US 100000u
#endif
#ifndef ND_BOARD_SPI_TIMEOUT_US
#define ND_BOARD_SPI_TIMEOUT_US 20000u
#endif

/*
 * Completion primitive. Baremetal uses a volatile flag plus WFI; FreeRTOS
 * blocks on a binary semaphore. The ISR side is the same call.
 */
typedef struct {
    volatile uint8_t done;
    volatile int status;
    void *sem;
} nd_board_op_t;

int nd_board_op_init(nd_board_op_t *op);
void nd_board_op_reset(nd_board_op_t *op);
int nd_board_op_wait(nd_board_op_t *op, uint32_t timeout_us);
int nd_board_op_take(nd_board_op_t *op);
void nd_board_op_complete(nd_board_op_t *op, int status);

/* UART receive ring filled from the RX interrupt. */
typedef struct {
    uint8_t buf[ND_BOARD_UART_RX_RING];
    volatile uint16_t head;
    volatile uint16_t tail;
    nd_board_op_t rx_ready;
} nd_board_uart_rx_t;

void nd_board_uart_rx_init(nd_board_uart_rx_t *rx);
void nd_board_uart_rx_isr(nd_board_uart_rx_t *rx, uint8_t byte);
int nd_board_uart_rx_statue(const nd_board_uart_rx_t *rx);
int nd_board_uart_rx_pop(nd_board_uart_rx_t *rx, uint8_t *byte);
int nd_board_uart_rx_read(nd_board_uart_rx_t *rx, uint8_t *byte,
                       uint32_t timeout_us);

/*
 * Hardware SPI transfer driven by two DMA channels. TX and RX are armed
 * together because the SPI clock only runs while TX feeds the shifter; each
 * channel reports completion separately. The length fields are 8-bit, so a
 * transfer is split into chunks of at most 256 bytes and the timeout applies
 * to each chunk.
 */
#define ND_BOARD_SPI_DMA_CHUNK 256u

typedef struct {
    nd_board_op_t op;
    const uint8_t *tx;
    uint8_t *rx;
    uint16_t len;
    volatile uint8_t tx_active;
    volatile uint8_t rx_active;
    volatile uint8_t busy;
    void *hw_ctx;
} nd_board_spi_dma_t;

void nd_board_spi_dma_init(nd_board_spi_dma_t *dma);
int nd_board_spi_dma_start(nd_board_spi_dma_t *dma, const uint8_t *tx,
                           uint8_t *rx, uint16_t len);
void nd_board_spi_dma_tx_done(nd_board_spi_dma_t *dma);
void nd_board_spi_dma_rx_done(nd_board_spi_dma_t *dma);
int nd_board_spi_dma_transfer(nd_board_spi_dma_t *dma, const uint8_t *tx,
                              uint8_t *rx, uint16_t len, uint32_t timeout_us);

/* Implemented by the SoC glue in firmware and by a fake in host tests. */
int nd_board_spi_dma_hw_start(void *ctx, const uint8_t *tx, uint8_t *rx,
                              uint16_t len);
void nd_board_spi_dma_hw_stop(void);
/*
 * TX DMA completion means the bytes reached the SPI FIFO, not that they left
 * the shifter. The caller waits here before releasing chip select.
 */
void nd_board_spi_dma_hw_wait_idle(void);
void nd_board_watchdog_kick(void);

struct nd_idis_board_state;
typedef struct nd_idis_board_state nd_idis_board_state_t;

#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ
void nd_board_uart_irq_init(nd_idis_board_state_t *state, size_t index,
                            const void *config);
void nd_board_uart_irq_tx_enable(nd_idis_board_state_t *state, size_t index,
                                 uint8_t enable);
void nd_board_uart_irq_rx_enable(nd_idis_board_state_t *state, size_t index,
                                 uint8_t enable);
int nd_board_uart_irq_write(nd_idis_board_state_t *state, size_t index,
                            uint8_t byte);
int nd_board_uart_irq_tx_done(nd_idis_board_state_t *state, size_t index);
int nd_board_uart_irq_read(nd_idis_board_state_t *state, size_t index,
                           uint8_t *byte);
int nd_board_uart_irq_rx_done(nd_idis_board_state_t *state, size_t index);
#endif

#endif
