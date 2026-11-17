#include "board_transport.h"

#include "cpu_surport.h"
#include "target.h"
#include "components/core/types/type_def.h"
#include "core_cm0.h"
#include "v85xxp_system.h"
#include "lib_dma.h"

void nd_board_watchdog_kick(void)
{
    mcu_clear_watchdog();
}

#if ND_BOARD_UART_MODE == ND_BOARD_MODE_IRQ

static nd_board_uart_rx_t uart_rx[ND_BOARD_UART_CHANNELS];
static nd_board_op_t uart_tx[ND_BOARD_UART_CHANNELS];

static void uart_rx_isr(void *para)
{
    size_t index = (size_t)(uintptr_t)para;
    uint8_t byte;

    if (mcu_uart_recive_byte(&byte, index)) {
        nd_board_uart_rx_isr(&uart_rx[index], byte);
    }
}

static void uart_tx_isr(void *para)
{
    size_t index = (size_t)(uintptr_t)para;

    nd_board_op_complete(&uart_tx[index], ND_BOARD_OK);
}

void nd_board_uart_irq_init(size_t index, const void *config)
{
    struct isr_reg_ctx_t reg;

    if (index >= ND_BOARD_UART_CHANNELS) {
        return;
    }
    nd_board_uart_rx_init(&uart_rx[index]);
    (void)nd_board_op_init(&uart_tx[index]);

    reg.IsrFunctionRegister = uart_rx_isr;
    reg.para = (void *)(uintptr_t)index;
    reg.irq_index = 0;
    (void)Isr_handle_regsit(&reg, IRQ_UART0_RX + (index << 1));

    reg.IsrFunctionRegister = uart_tx_isr;
    reg.para = (void *)(uintptr_t)index;
    reg.irq_index = 0;
    (void)Isr_handle_regsit(&reg, IRQ_UART0_TX + (index << 1));

    (void)mcu_uart_init((void *)config, (uint8_t)index, 0);
    mcu_uart_recive_enable(1, index);
}

void nd_board_uart_irq_tx_enable(size_t index, uint8_t enable)
{
    if (index < ND_BOARD_UART_CHANNELS) {
        mcu_uart_send_enable(enable != 0 ? 1 : 0, index);
    }
}

void nd_board_uart_irq_rx_enable(size_t index, uint8_t enable)
{
    if (index < ND_BOARD_UART_CHANNELS) {
        mcu_uart_recive_enable(enable != 0 ? 1 : 0, index);
    }
}

int nd_board_uart_irq_write(size_t index, uint8_t byte)
{
    if (index >= ND_BOARD_UART_CHANNELS) {
        return -1;
    }
    nd_board_op_reset(&uart_tx[index]);
    return mcu_uart_send_byte(byte, index) ? 0 : -1;
}

int nd_board_uart_irq_tx_done(size_t index)
{
    if (index >= ND_BOARD_UART_CHANNELS) {
        return 0;
    }
    return nd_board_op_take(&uart_tx[index]);
}

int nd_board_uart_irq_read(size_t index, uint8_t *byte)
{
    if (index >= ND_BOARD_UART_CHANNELS) {
        return -1;
    }
    return nd_board_uart_rx_read(&uart_rx[index], byte,
                              ND_BOARD_UART_RX_TIMEOUT_US)
               == ND_BOARD_OK
           ? 0
           : -1;
}

int nd_board_uart_irq_rx_done(size_t index)
{
    if (index >= ND_BOARD_UART_CHANNELS) {
        return 0;
    }
    return nd_board_uart_rx_statue(&uart_rx[index]);
}

#endif

#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA

#define ND_BOARD_SPI_DMA_TX_CH DMA_CHANNEL_2
#define ND_BOARD_SPI_DMA_RX_CH DMA_CHANNEL_3

static nd_board_spi_dma_t spi_dma;
static uint8_t spi_dma_ready;

static void spi_dma_config(DMA_InitType *cfg, uint32_t src, uint32_t dst,
                           uint16_t len, uint32_t request, uint32_t src_mode,
                           uint32_t dst_mode)
{
    cfg->DestAddr = dst;
    cfg->SrcAddr = src;
    /*
     * One byte per request with an 8-bit package count. The caller caps len
     * at 256 so the package counter always covers the whole chunk.
     */
    cfg->FrameLen = 0;
    cfg->PackLen = (uint8_t)(len - 1u);
    cfg->ContMode = DMA_CONTMODE_DISABLE;
    cfg->TransMode = DMA_TRANSMODE_PACK;
    cfg->ReqSrc = request;
    cfg->DestAddrMode = dst_mode;
    cfg->SrcAddrMode = src_mode;
    cfg->TransSize = DMA_TRANSSIZE_BYTE;
}

static void spi_dma_stop(void)
{
    DMA_StopTransmit(ND_BOARD_SPI_DMA_TX_CH, ENABLE);
    DMA_Cmd(ND_BOARD_SPI_DMA_TX_CH, DISABLE);
    DMA_INTConfig(DMA_INT_C2DA, DISABLE);
    DMA_ClearINTStatus(DMA_INTSTS_C2DA);
    DMA_StopTransmit(ND_BOARD_SPI_DMA_RX_CH, ENABLE);
    DMA_Cmd(ND_BOARD_SPI_DMA_RX_CH, DISABLE);
    DMA_INTConfig(DMA_INT_C3DA, DISABLE);
    DMA_ClearINTStatus(DMA_INTSTS_C3DA);
    DMA_StopTransmit(ND_BOARD_SPI_DMA_TX_CH, DISABLE);
    DMA_StopTransmit(ND_BOARD_SPI_DMA_RX_CH, DISABLE);
}

static void spi_dma_isr(void *para)
{
    (void)para;

    if (DMA_GetINTStatus(DMA_INTSTS_C2DA)) {
        DMA_ClearINTStatus(DMA_INTSTS_C2DA);
        DMA_INTConfig(DMA_INT_C2DA, DISABLE);
        nd_board_spi_dma_tx_done(&spi_dma);
    }
    if (DMA_GetINTStatus(DMA_INTSTS_C3DA)) {
        DMA_ClearINTStatus(DMA_INTSTS_C3DA);
        DMA_INTConfig(DMA_INT_C3DA, DISABLE);
        nd_board_spi_dma_rx_done(&spi_dma);
    }
    if (spi_dma.tx_active == 0 && spi_dma.rx_active == 0) {
        spi_dma_stop();
    }
}

int nd_board_spi_dma_hw_start(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    DMA_InitType cfg;

    if (spi_dma_ready == 0) {
        struct isr_reg_ctx_t reg;

        reg.IsrFunctionRegister = spi_dma_isr;
        reg.para = NULL;
        reg.irq_index = 0;
        (void)Isr_handle_regsit(&reg, IRQ_DMA);
        spi_dma_ready = 1;
    }

    DMA_DeInit(ND_BOARD_SPI_DMA_TX_CH);
    DMA_DeInit(ND_BOARD_SPI_DMA_RX_CH);

    if (rx != NULL) {
        spi_dma_config(&cfg, (uint32_t)(uintptr_t)&SPI2->RXDAT,
                       (uint32_t)(uintptr_t)rx, len, DMA_REQSRC_SPI2RX,
                       DMA_SRCADDRMODE_FIX, DMA_DESTADDRMODE_PEND);
        DMA_Init(&cfg, ND_BOARD_SPI_DMA_RX_CH);
        DMA_INTConfig(DMA_INT_C3DA, ENABLE);
    }
    if (tx != NULL) {
        spi_dma_config(&cfg, (uint32_t)(uintptr_t)tx,
                       (uint32_t)(uintptr_t)&SPI2->TXDAT, len,
                       DMA_REQSRC_SPI2TX, DMA_SRCADDRMODE_PEND,
                       DMA_DESTADDRMODE_FIX);
        DMA_Init(&cfg, ND_BOARD_SPI_DMA_TX_CH);
        DMA_INTConfig(DMA_INT_C2DA, ENABLE);
    }

    NVIC_EnableIRQ(DMA_IRQn);
    if (rx != NULL) {
        DMA_Cmd(ND_BOARD_SPI_DMA_RX_CH, ENABLE);
    }
    if (tx != NULL) {
        DMA_Cmd(ND_BOARD_SPI_DMA_TX_CH, ENABLE);
    }
    return 0;
}

void nd_board_spi_dma_hw_stop(void)
{
    spi_dma_stop();
}

void nd_board_spi_dma_hw_wait_idle(void)
{
    uint32_t timeout = ND_BOARD_SPI_TIMEOUT_US;

    while ((SPI2->MISC_ & SPI_MISC_BSY_Msk) != 0
           || (SPI2->MISC_ & SPI_MISC_TFE) == 0) {
        if (timeout == 0) {
            return;
        }
        timeout--;
        if ((timeout % 64u) == 0) {
            nd_board_watchdog_kick();
        }
        (void)mcu_delay_us(1);
    }
}

#else

int nd_board_spi_dma_hw_start(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    (void)tx;
    (void)rx;
    (void)len;
    return -1;
}

void nd_board_spi_dma_hw_stop(void)
{
}

void nd_board_spi_dma_hw_wait_idle(void)
{
}

#endif
