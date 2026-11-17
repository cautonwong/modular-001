#include "board_transport.h"

#if defined(__arm__) || defined(__ICCARM__)
#include "target.h"
#include "core_cm0.h"
#define ND_BOARD_WFI() __WFI()
#define ND_BOARD_BARRIER() __DMB()
#else
#define ND_BOARD_WFI() ((void)0)
#define ND_BOARD_BARRIER() ((void)0)
#endif

#if ND_BOARD_USE_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
#endif

int nd_board_op_init(nd_board_op_t *op)
{
    if (op == NULL) {
        return ND_BOARD_INVALID;
    }
#if ND_BOARD_USE_FREERTOS
    if (op->sem == NULL) {
        op->sem = xSemaphoreCreateBinary();
        if (op->sem == NULL) {
            return ND_BOARD_INVALID;
        }
    }
#endif
    nd_board_op_reset(op);
    return ND_BOARD_OK;
}

void nd_board_op_reset(nd_board_op_t *op)
{
#if ND_BOARD_USE_FREERTOS
    if (op->sem != NULL) {
        (void)xSemaphoreTake((SemaphoreHandle_t)op->sem, 0);
    }
#endif
    op->status = ND_BOARD_OK;
    ND_BOARD_BARRIER();
    op->done = 0;
}

int nd_board_op_wait(nd_board_op_t *op, uint32_t timeout_us)
{
#if ND_BOARD_USE_FREERTOS
    TickType_t ticks;

    if (op->sem == NULL) {
        return ND_BOARD_TIMEOUT;
    }
    ticks = pdMS_TO_TICKS(timeout_us / 1000u);
    if (ticks == 0) {
        ticks = 1;
    }
    if (xSemaphoreTake((SemaphoreHandle_t)op->sem, ticks) != pdTRUE) {
        return ND_BOARD_TIMEOUT;
    }
    return op->status;
#else
    uint32_t steps = (timeout_us + ND_BOARD_SYSTICK_US - 1u) / ND_BOARD_SYSTICK_US;

    while (op->done == 0) {
        if (steps == 0) {
            return ND_BOARD_TIMEOUT;
        }
        steps--;
        nd_board_watchdog_kick();
        ND_BOARD_WFI();
    }
    ND_BOARD_BARRIER();
    return op->status;
#endif
}

int nd_board_op_take(nd_board_op_t *op)
{
#if ND_BOARD_USE_FREERTOS
    if (op->sem == NULL) {
        return 0;
    }
    return xSemaphoreTake((SemaphoreHandle_t)op->sem, 0) == pdTRUE
               ? 1
               : 0;
#else
    if (op->done == 0) {
        return 0;
    }
    ND_BOARD_BARRIER();
    op->done = 0;
    return 1;
#endif
}

void nd_board_op_complete(nd_board_op_t *op, int status)
{
    op->status = status;
    ND_BOARD_BARRIER();
#if ND_BOARD_USE_FREERTOS
    if (op->sem == NULL) {
        return;
    }
    {
        BaseType_t woken = pdFALSE;

        xSemaphoreGiveFromISR((SemaphoreHandle_t)op->sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
#else
    op->done = 1;
#endif
}

void nd_board_uart_rx_init(nd_board_uart_rx_t *rx)
{
    rx->head = 0;
    rx->tail = 0;
    (void)nd_board_op_init(&rx->rx_ready);
}

void nd_board_uart_rx_isr(nd_board_uart_rx_t *rx, uint8_t byte)
{
    uint16_t next = (uint16_t)((rx->head + 1u) % ND_BOARD_UART_RX_RING);

    /* Overrun keeps the oldest byte and drops the newest. */
    if (next != rx->tail) {
        rx->buf[rx->head] = byte;
        rx->head = next;
    }
    nd_board_op_complete(&rx->rx_ready, ND_BOARD_OK);
}

int nd_board_uart_rx_statue(const nd_board_uart_rx_t *rx)
{
    return rx->head != rx->tail;
}

int nd_board_uart_rx_pop(nd_board_uart_rx_t *rx, uint8_t *byte)
{
    if (rx->head == rx->tail) {
        return 0;
    }
    *byte = rx->buf[rx->tail];
    rx->tail = (uint16_t)((rx->tail + 1u) % ND_BOARD_UART_RX_RING);
    return 1;
}

int nd_board_uart_rx_read(nd_board_uart_rx_t *rx, uint8_t *byte,
                       uint32_t timeout_us)
{
    for (;;) {
        if (nd_board_uart_rx_pop(rx, byte) != 0) {
            return ND_BOARD_OK;
        }
        /*
         * Reset before the second check so a byte that arrives between the
         * check and the reset still wakes this loop.
         */
        nd_board_op_reset(&rx->rx_ready);
        if (rx->head != rx->tail) {
            continue;
        }
        if (nd_board_op_wait(&rx->rx_ready, timeout_us) != ND_BOARD_OK) {
            return ND_BOARD_TIMEOUT;
        }
    }
}

void nd_board_spi_dma_init(nd_board_spi_dma_t *dma)
{
    dma->tx = NULL;
    dma->rx = NULL;
    dma->len = 0;
    dma->tx_active = 0;
    dma->rx_active = 0;
    dma->busy = 0;
    (void)nd_board_op_init(&dma->op);
}

int nd_board_spi_dma_start(nd_board_spi_dma_t *dma, const uint8_t *tx,
                           uint8_t *rx, uint16_t len)
{
    if (dma == NULL || len == 0 || len > ND_BOARD_SPI_DMA_CHUNK
        || (tx == NULL && rx == NULL) || dma->busy) {
        return ND_BOARD_INVALID;
    }
    dma->tx = tx;
    dma->rx = rx;
    dma->len = len;
    dma->tx_active = tx != NULL ? 1 : 0;
    dma->rx_active = rx != NULL ? 1 : 0;
    dma->busy = 1;
    nd_board_op_reset(&dma->op);
    if (nd_board_spi_dma_hw_start(tx, rx, len) != 0) {
        dma->tx_active = 0;
        dma->rx_active = 0;
        dma->busy = 0;
        return ND_BOARD_INVALID;
    }
    return ND_BOARD_OK;
}

void nd_board_spi_dma_tx_done(nd_board_spi_dma_t *dma)
{
    if (dma->tx_active == 0) {
        return;
    }
    dma->tx_active = 0;
    if (dma->rx_active == 0) {
        dma->busy = 0;
        nd_board_op_complete(&dma->op, ND_BOARD_OK);
    }
}

void nd_board_spi_dma_rx_done(nd_board_spi_dma_t *dma)
{
    if (dma->rx_active == 0) {
        return;
    }
    dma->rx_active = 0;
    if (dma->tx_active == 0) {
        dma->busy = 0;
        nd_board_op_complete(&dma->op, ND_BOARD_OK);
    }
}

int nd_board_spi_dma_transfer(nd_board_spi_dma_t *dma, const uint8_t *tx,
                              uint8_t *rx, uint16_t len, uint32_t timeout_us)
{
    uint16_t offset = 0;

    if (dma == NULL || len == 0 || (tx == NULL && rx == NULL) || dma->busy) {
        return ND_BOARD_INVALID;
    }
    while (offset < len) {
        uint16_t chunk = (uint16_t)(len - offset);
        int result;

        if (chunk > ND_BOARD_SPI_DMA_CHUNK) {
            chunk = ND_BOARD_SPI_DMA_CHUNK;
        }
        result = nd_board_spi_dma_start(dma, tx != NULL ? tx + offset : NULL,
                                        rx != NULL ? rx + offset : NULL, chunk);
        if (result != ND_BOARD_OK) {
            return result;
        }
        result = nd_board_op_wait(&dma->op, timeout_us);
        if (result != ND_BOARD_OK) {
            nd_board_spi_dma_hw_stop();
            dma->tx_active = 0;
            dma->rx_active = 0;
            dma->busy = 0;
            return result;
        }
        offset = (uint16_t)(offset + chunk);
    }
    nd_board_spi_dma_hw_wait_idle();
    return ND_BOARD_OK;
}
