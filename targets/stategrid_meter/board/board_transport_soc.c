#include "board_transport.h"
#include "board_state.h"

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

static void uart_rx_isr(void *para)
{
    struct {
        stategrid_board_state_t *state;
        uint8_t index;
    } *ctx = para;
    uint8_t byte;

    if (ctx == NULL || ctx->state == NULL) {
        return;
    }
    if (mcu_uart_recive_byte(&byte, ctx->index)) {
        nd_board_uart_rx_isr(&ctx->state->uart_rx[ctx->index], byte);
    }
}

static void uart_tx_isr(void *para)
{
    struct {
        stategrid_board_state_t *state;
        uint8_t index;
    } *ctx = para;

    if (ctx == NULL || ctx->state == NULL) {
        return;
    }
    nd_board_op_complete(&ctx->state->uart_tx[ctx->index], ND_BOARD_OK);
}

void nd_board_uart_irq_init(struct stategrid_board_state *state, size_t index,
                            const void *config)
{
    struct isr_reg_ctx_t reg;

    if (state == NULL || index >= ND_BOARD_UART_CHANNELS) {
        return;
    }
    nd_board_uart_rx_init(&state->uart_rx[index]);
    (void)nd_board_op_init(&state->uart_tx[index]);
    state->uart_irq[index].state = state;
    state->uart_irq[index].index = (uint8_t)index;

    reg.IsrFunctionRegister = uart_rx_isr;
    reg.para = &state->uart_irq[index];
    reg.irq_index = 0;
    (void)Isr_handle_regsit(&reg, IRQ_UART0_RX + (index << 1));

    reg.IsrFunctionRegister = uart_tx_isr;
    reg.para = &state->uart_irq[index];
    reg.irq_index = 0;
    (void)Isr_handle_regsit(&reg, IRQ_UART0_TX + (index << 1));

    (void)mcu_uart_init((void *)config, (uint8_t)index, 0);
    mcu_uart_recive_enable(1, index);
}

void nd_board_uart_irq_tx_enable(struct stategrid_board_state *state,
                                 size_t index, uint8_t enable)
{
    (void)state;
    if (index < ND_BOARD_UART_CHANNELS) {
        mcu_uart_send_enable(enable != 0 ? 1 : 0, index);
    }
}

void nd_board_uart_irq_rx_enable(struct stategrid_board_state *state,
                                 size_t index, uint8_t enable)
{
    (void)state;
    if (index < ND_BOARD_UART_CHANNELS) {
        mcu_uart_recive_enable(enable != 0 ? 1 : 0, index);
    }
}

int nd_board_uart_irq_write(struct stategrid_board_state *state, size_t index,
                            uint8_t byte)
{
    if (state == NULL || index >= ND_BOARD_UART_CHANNELS) {
        return -1;
    }
    nd_board_op_reset(&state->uart_tx[index]);
    return mcu_uart_send_byte(byte, index) ? 0 : -1;
}

int nd_board_uart_irq_tx_done(struct stategrid_board_state *state,
                              size_t index)
{
    if (state == NULL || index >= ND_BOARD_UART_CHANNELS) {
        return 0;
    }
    return nd_board_op_take(&state->uart_tx[index]);
}

int nd_board_uart_irq_read(struct stategrid_board_state *state, size_t index,
                           uint8_t *byte)
{
    if (state == NULL || index >= ND_BOARD_UART_CHANNELS) {
        return -1;
    }
    return nd_board_uart_rx_read(&state->uart_rx[index], byte,
                              ND_BOARD_UART_RX_TIMEOUT_US)
               == ND_BOARD_OK
           ? 0
           : -1;
}

int nd_board_uart_irq_rx_done(struct stategrid_board_state *state,
                              size_t index)
{
    if (state == NULL || index >= ND_BOARD_UART_CHANNELS) {
        return 0;
    }
    return nd_board_uart_rx_statue(&state->uart_rx[index]);
}

#endif

#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA

#define ND_BOARD_SPI_DMA_TX_CH DMA_CHANNEL_2
#define ND_BOARD_SPI_DMA_RX_CH DMA_CHANNEL_3

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

static void spi_dma_stop(stategrid_board_state_t *state)
{
    if (state == NULL) {
        return;
    }
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
    stategrid_board_state_t *state = (stategrid_board_state_t *)para;

    if (state == NULL) {
        return;
    }
    if (DMA_GetINTStatus(DMA_INTSTS_C2DA)) {
        DMA_ClearINTStatus(DMA_INTSTS_C2DA);
        DMA_INTConfig(DMA_INT_C2DA, DISABLE);
        nd_board_spi_dma_tx_done(&state->spi_dma);
    }
    if (DMA_GetINTStatus(DMA_INTSTS_C3DA)) {
        DMA_ClearINTStatus(DMA_INTSTS_C3DA);
        DMA_INTConfig(DMA_INT_C3DA, DISABLE);
        nd_board_spi_dma_rx_done(&state->spi_dma);
    }
    if (state->spi_dma.tx_active == 0 && state->spi_dma.rx_active == 0) {
        spi_dma_stop(state);
    }
}

int nd_board_spi_dma_hw_start(void *ctx, const uint8_t *tx, uint8_t *rx,
                              uint16_t len)
{
    stategrid_board_state_t *state = (stategrid_board_state_t *)ctx;
    DMA_InitType cfg;

    if (state == NULL) {
        return -1;
    }
    if (state->spi_dma_ready == 0) {
        struct isr_reg_ctx_t reg;

        reg.IsrFunctionRegister = spi_dma_isr;
        reg.para = state;
        reg.irq_index = 0;
        (void)Isr_handle_regsit(&reg, IRQ_DMA);
        state->spi_dma_ready = 1;
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

void nd_board_spi_dma_hw_stop(void *ctx)
{
    spi_dma_stop((stategrid_board_state_t *)ctx);
}

void nd_board_spi_dma_hw_wait_idle(void *ctx)
{
    uint32_t timeout = ND_BOARD_SPI_TIMEOUT_US;

    (void)ctx;

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

int nd_board_spi_dma_hw_start(void *ctx, const uint8_t *tx, uint8_t *rx,
                              uint16_t len)
{
    (void)ctx;
    (void)tx;
    (void)rx;
    (void)len;
    return -1;
}

void nd_board_spi_dma_hw_stop(void *ctx)
{
    (void)ctx;
}

void nd_board_spi_dma_hw_wait_idle(void *ctx)
{
    (void)ctx;
}

#endif
