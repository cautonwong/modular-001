/**
 * @ingroup    drivers_spi_bus
 * @brief      spi 总线驱动
 *
 * @{
 * @file
 *
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "target.h"
#include "core_cm0.h"
#include "cpu_surport.h"
#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "targets/idis_ip_meter_freertos/board/board_transport.h"
#include "board_state.h"
#include "targets/idis_ip_meter_freertos/board/spi_driver.h"
#include "targets/idis_ip_meter_freertos/board/spi_pins.h"




#define SPI_DELAY()        {__NOP(); __NOP(); __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();}




static int spi_bus_init(void)
{
    //开漏
    nd_spi_cs_attention();

    nd_spi_clk_attention();

    nd_spi_mosi_attention();

    nd_spi_miso_input();

    nd_spi_mosi_output();

    nd_spi_mosi_set(1);

    nd_spi_sck_output();

    nd_spi_sck_set(0);



    if (nd_spi_miso_read())
    {
        return true;
    }

    return false;
}


static int spi_send_byte_mode3(uint8_t byte)
{
    register uint32_t send_byte = byte;
    nd_spi_sck_set(0);
    if (send_byte & 0x80)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x40)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x20)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x10)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x08)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x04)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x02)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x01)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);

    return true;
}


static int spi_recive_byte_mode3(uint8_t *byte)
{
    register uint32_t rcv_byte = 0;

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 7);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 6);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 5);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 4);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 3);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 2);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 1);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += nd_spi_miso_read();
    nd_spi_sck_set(0);

    *byte = rcv_byte & 0x000000ff;

    return true;
}




static void spi_ctrl(uint8_t ctrl)
{
    switch(ctrl)
    {
        case START:
              nd_spi_sck_set(0);
              nd_spi_cs_set(0);
              break;
        case STOP:
              nd_spi_cs_set(1);
              nd_spi_sck_set(0);
              break;
        default:
          break;
    }
}


#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
/*
 * The ESAM driver still sends one byte per bus call, so the request frame is
 * staged here and clocked as DMA chunks: on fill, on the driver's pre-CS hold
 * delay, or on CS release. The slave shifts data during the request too, hence
 * the RX sink.
 */
static int esam_flush(nd_idis_board_state_t *state)
{
    uint16_t len;
    int result;

    if (state == NULL) {
        return -1;
    }
    len = state->esam_tx_used;
    if (len == 0) {
        return 0;
    }
    state->esam_tx_used = 0;
    if (!state->esam_dma_ready) {
        nd_board_spi_dma_init(&state->esam_dma);
        state->esam_dma_ready = 1;
    }
    result = nd_board_spi_dma_transfer(&state->esam_dma, state->esam_tx_stage,
                                       state->esam_rx_sink, len,
                                       ND_BOARD_SPI_TIMEOUT_US);
    return result == ND_BOARD_OK ? 0 : -1;
}
#endif


void nd_board_spi_flush(nd_idis_board_state_t *state, nd_board_spi_t bus)
{
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    if (bus == ND_BOARD_SPI_ESAM) {
        (void)esam_flush(state);
    }
#else
    (void)state;
    (void)bus;
#endif
}


void nd_board_spi_init(nd_board_spi_t bus)
{
    if (bus == ND_BOARD_SPI_FLASH) {
        (void)spi_bus_init();
    }
}


void nd_board_spi_select(nd_idis_board_state_t *state, nd_board_spi_t bus,
                         int active)
{
    if (bus == ND_BOARD_SPI_FLASH) {
        spi_ctrl(active ? START : STOP);
        return;
    }
    if (state == NULL) {
        return;
    }
    if (active) {
        v85xxp_spi_ctrl(START, 0);
        (void)mcu_spi_bus_init(0, 0);
    } else {
        nd_board_spi_flush(state, ND_BOARD_SPI_ESAM);
        v85xxp_spi_ctrl(STOP, 0);
    }
}


int nd_board_spi_transfer(nd_idis_board_state_t *state, nd_board_spi_t bus,
                          const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (state == NULL) {
        return -1;
    }
    if (bus == ND_BOARD_SPI_FLASH) {
        if (tx != NULL) {
            while (len > 0) {
                if (!spi_send_byte_mode3(*tx)) {
                    return -1;
                }
                tx++;
                len--;
            }
            return 0;
        }
        while (rx != NULL && len > 0) {
            if (!spi_recive_byte_mode3(rx)) {
                return -1;
            }
            rx++;
            len--;
        }
        return rx != NULL ? 0 : -1;
    }
    if (tx == NULL && rx == NULL) {
        return -1;
    }
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    if (tx != NULL && rx == NULL) {
        while (len > 0) {
            state->esam_tx_stage[state->esam_tx_used] = *tx;
            state->esam_tx_used = (uint16_t)(state->esam_tx_used + 1u);
            tx++;
            len--;
            if (state->esam_tx_used == ND_BOARD_SPI_DMA_CHUNK
                && esam_flush(state) != 0) {
                return -1;
            }
        }
        return 0;
    }
    if (esam_flush(state) != 0) {
        return -1;
    }
#endif
    while (len > 0) {
        if (tx != NULL) {
            if (!mcu_spi_send_byte(*tx, 0)) {
                return -1;
            }
            tx++;
        } else {
            if (!mcu_spi_recive_byte(rx, 0)) {
                return -1;
            }
            rx++;
        }
        len--;
    }
    return 0;
}

void nd_board_spi_bind(nd_idis_board_state_t *state)
{
#if ND_BOARD_SPI_MODE == ND_BOARD_MODE_DMA
    if (state != NULL) {
        state->esam_dma.hw_ctx = state;
    }
#else
    (void)state;
#endif
}

/** @} */
