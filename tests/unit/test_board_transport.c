#include "unity.h"

#include <string.h>

#include "targets/stategrid_meter/board/board_transport.h"

static int hw_start_calls;
static int hw_stop_calls;
static int hw_wait_idle_calls;
static int hw_start_result;
static uint16_t hw_len;
static const uint8_t *hw_txs[8];
static uint8_t *hw_rxs[8];
static uint16_t hw_lens[8];
static nd_board_spi_dma_t *hw_active;
static int hw_complete_in_start;
static int watchdog_kicks;
static void *hw_ctxs[8];

int nd_board_spi_dma_hw_start(void *ctx, const uint8_t *tx, uint8_t *rx,
                              uint16_t len)
{
    if (hw_start_calls < 8) {
        hw_ctxs[hw_start_calls] = ctx;
        hw_txs[hw_start_calls] = tx;
        hw_rxs[hw_start_calls] = rx;
        hw_lens[hw_start_calls] = len;
    }
    hw_start_calls++;
    hw_len = len;
    if (hw_complete_in_start && hw_active != NULL) {
        nd_board_spi_dma_tx_done(hw_active);
        if (rx != NULL) {
            nd_board_spi_dma_rx_done(hw_active);
        }
    }
    return hw_start_result;
}

void nd_board_spi_dma_hw_stop(void *ctx)
{
    (void)ctx;
    hw_stop_calls++;
}

void nd_board_spi_dma_hw_wait_idle(void *ctx)
{
    (void)ctx;
    hw_wait_idle_calls++;
}

void nd_board_watchdog_kick(void)
{
    watchdog_kicks++;
}

void setUp(void)
{
    hw_start_calls = 0;
    hw_stop_calls = 0;
    hw_wait_idle_calls = 0;
    hw_start_result = 0;
    hw_len = 0;
    hw_active = NULL;
    hw_complete_in_start = 0;
    watchdog_kicks = 0;
    memset(hw_txs, 0, sizeof(hw_txs));
    memset(hw_rxs, 0, sizeof(hw_rxs));
    memset(hw_lens, 0, sizeof(hw_lens));
    memset(hw_ctxs, 0, sizeof(hw_ctxs));
}

void tearDown(void) {}

static void test_completion_waits_then_takes(void)
{
    nd_board_op_t op;

    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_op_init(&op));
    nd_board_op_reset(&op);
    TEST_ASSERT_EQUAL_INT(0, nd_board_op_take(&op));

    nd_board_op_complete(&op, ND_BOARD_OK);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_op_wait(&op, 1000));
    TEST_ASSERT_EQUAL_INT(1, nd_board_op_take(&op));
    TEST_ASSERT_EQUAL_INT(0, nd_board_op_take(&op));
}

static void test_completion_reports_isr_status(void)
{
    nd_board_op_t op;

    nd_board_op_init(&op);
    nd_board_op_reset(&op);
    nd_board_op_complete(&op, -7);
    TEST_ASSERT_EQUAL_INT(-7, nd_board_op_wait(&op, 1000));
}

static void test_completion_timeout_is_bounded_and_kicks_watchdog(void)
{
    nd_board_op_t op;

    nd_board_op_init(&op);
    nd_board_op_reset(&op);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_TIMEOUT,
                          nd_board_op_wait(&op, 3u * ND_BOARD_SYSTICK_US));
    TEST_ASSERT_EQUAL_INT(3, watchdog_kicks);
}

static void test_ring_preserves_order_across_wrap(void)
{
    nd_board_uart_rx_t rx;
    uint8_t byte;

    nd_board_uart_rx_init(&rx);
    for (uint8_t i = 0; i < 26; i++) {
        nd_board_uart_rx_isr(&rx, (uint8_t)('a' + i));
    }
    for (uint8_t i = 0; i < 26; i++) {
        TEST_ASSERT_EQUAL_INT(1, nd_board_uart_rx_pop(&rx, &byte));
        TEST_ASSERT_EQUAL_UINT8((uint8_t)('a' + i), byte);
    }
    TEST_ASSERT_EQUAL_INT(0, nd_board_uart_rx_statue(&rx));
}

static void test_ring_overrun_keeps_oldest_and_drops_newest(void)
{
    nd_board_uart_rx_t rx;
    uint8_t byte;
    uint16_t count = 0;

    nd_board_uart_rx_init(&rx);
    for (uint16_t i = 1; i <= ND_BOARD_UART_RX_RING - 1; i++) {
        nd_board_uart_rx_isr(&rx, (uint8_t)i);
    }
    nd_board_uart_rx_isr(&rx, 0xFFu);
    TEST_ASSERT_EQUAL_INT(1, nd_board_uart_rx_statue(&rx));

    while (nd_board_uart_rx_pop(&rx, &byte)) {
        count++;
        if (count < ND_BOARD_UART_RX_RING - 1) {
            TEST_ASSERT_EQUAL_UINT8((uint8_t)count, byte);
        } else {
            TEST_ASSERT_EQUAL_UINT8(ND_BOARD_UART_RX_RING - 1, byte);
        }
    }
    TEST_ASSERT_EQUAL_UINT16(ND_BOARD_UART_RX_RING - 1, count);
}

static void test_isr_completes_the_ready_primitive(void)
{
    nd_board_uart_rx_t rx;

    nd_board_uart_rx_init(&rx);
    nd_board_op_reset(&rx.rx_ready);
    nd_board_uart_rx_isr(&rx, 0x42u);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_op_wait(&rx.rx_ready, 1000));
}

static void test_uart_read_returns_buffered_byte(void)
{
    nd_board_uart_rx_t rx;
    uint8_t byte = 0;

    nd_board_uart_rx_init(&rx);
    nd_board_uart_rx_isr(&rx, 0x42u);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_uart_rx_read(&rx, &byte, 1000));
    TEST_ASSERT_EQUAL_UINT8(0x42u, byte);
}

static void test_uart_read_times_out_on_empty_ring(void)
{
    nd_board_uart_rx_t rx;
    uint8_t byte = 0;

    nd_board_uart_rx_init(&rx);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_TIMEOUT,
                          nd_board_uart_rx_read(&rx, &byte,
                                             4u * ND_BOARD_SYSTICK_US));
    TEST_ASSERT_EQUAL_INT(4, watchdog_kicks);
}

static void test_spi_two_channels_complete_in_any_order(void)
{
    nd_board_spi_dma_t dma;
    uint8_t tx[4] = {1, 2, 3, 4};
    uint8_t rx[4] = {0};

    nd_board_spi_dma_init(&dma);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_spi_dma_start(&dma, tx, rx, 4));
    TEST_ASSERT_EQUAL_INT(1, hw_start_calls);
    TEST_ASSERT_EQUAL_INT(4, hw_len);
    TEST_ASSERT_EQUAL_INT(0, nd_board_op_take(&dma.op));

    nd_board_spi_dma_rx_done(&dma);
    TEST_ASSERT_EQUAL_INT(0, nd_board_op_take(&dma.op));
    nd_board_spi_dma_tx_done(&dma);
    TEST_ASSERT_EQUAL_INT(1, nd_board_op_take(&dma.op));
    TEST_ASSERT_EQUAL_INT(0, dma.busy);
}

static void test_spi_tx_only_completes_on_tx_done(void)
{
    nd_board_spi_dma_t dma;
    uint8_t tx[4] = {1, 2, 3, 4};

    nd_board_spi_dma_init(&dma);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_spi_dma_start(&dma, tx, NULL, 4));
    nd_board_spi_dma_rx_done(&dma);
    TEST_ASSERT_EQUAL_INT(0, nd_board_op_take(&dma.op));
    nd_board_spi_dma_tx_done(&dma);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_op_wait(&dma.op, 1000));
}

static void test_spi_rejects_empty_and_busy_transfers(void)
{
    nd_board_spi_dma_t dma;
    uint8_t tx[2] = {1, 2};
    uint8_t rx[2] = {0};

    nd_board_spi_dma_init(&dma);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_INVALID,
                          nd_board_spi_dma_start(&dma, tx, rx, 0));
    TEST_ASSERT_EQUAL_INT(ND_BOARD_INVALID,
                          nd_board_spi_dma_start(&dma, NULL, NULL, 2));
    TEST_ASSERT_EQUAL_INT(ND_BOARD_INVALID,
                          nd_board_spi_dma_start(&dma, tx, rx, 257));
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK,
                          nd_board_spi_dma_start(&dma, tx, rx, 2));
    TEST_ASSERT_EQUAL_INT(ND_BOARD_INVALID,
                          nd_board_spi_dma_start(&dma, tx, rx, 2));
    nd_board_spi_dma_tx_done(&dma);
    nd_board_spi_dma_rx_done(&dma);
}

static void test_spi_transfer_timeout_stops_the_hardware(void)
{
    nd_board_spi_dma_t dma;
    uint8_t tx[4] = {1, 2, 3, 4};
    uint8_t rx[4] = {0};

    nd_board_spi_dma_init(&dma);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_TIMEOUT,
                          nd_board_spi_dma_transfer(&dma, tx, rx, 4,
                                                ND_BOARD_SYSTICK_US));
    TEST_ASSERT_EQUAL_INT(1, hw_stop_calls);
    TEST_ASSERT_EQUAL_INT(0, hw_wait_idle_calls);
    TEST_ASSERT_EQUAL_INT(0, dma.busy);
}

static void test_spi_transfer_chunks_long_buffers(void)
{
    static nd_board_spi_dma_t dma;
    static uint8_t tx[300];
    static uint8_t rx[300];

    nd_board_spi_dma_init(&dma);
    hw_active = &dma;
    hw_complete_in_start = 1;
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK,
                          nd_board_spi_dma_transfer(&dma, tx, rx, 300, 1000));
    TEST_ASSERT_EQUAL_INT(2, hw_start_calls);
    TEST_ASSERT_EQUAL_PTR(tx, hw_txs[0]);
    TEST_ASSERT_EQUAL_PTR(rx, hw_rxs[0]);
    TEST_ASSERT_EQUAL_UINT16(256, hw_lens[0]);
    TEST_ASSERT_EQUAL_PTR(tx + 256, hw_txs[1]);
    TEST_ASSERT_EQUAL_PTR(rx + 256, hw_rxs[1]);
    TEST_ASSERT_EQUAL_UINT16(44, hw_lens[1]);
    TEST_ASSERT_EQUAL_INT(1, hw_wait_idle_calls);
    TEST_ASSERT_EQUAL_INT(0, dma.busy);
    TEST_ASSERT_EQUAL_INT(0, hw_stop_calls);
}

static void test_spi_hw_ctx_reaches_the_hw_start(void)
{
    nd_board_spi_dma_t dma;
    uint8_t tx[4] = { 1, 2, 3, 4 };
    static int marker;

    nd_board_spi_dma_init(&dma);
    TEST_ASSERT_NULL(dma.hw_ctx);
    dma.hw_ctx = &marker;
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK, nd_board_spi_dma_start(&dma, tx, NULL,
                                                              4));
    TEST_ASSERT_EQUAL_PTR(&marker, hw_ctxs[0]);
    nd_board_spi_dma_tx_done(&dma);
}

static void test_spi_start_failure_is_reported(void)
{
    nd_board_spi_dma_t dma;
    uint8_t tx[2] = {1, 2};

    nd_board_spi_dma_init(&dma);
    hw_start_result = -1;
    TEST_ASSERT_EQUAL_INT(ND_BOARD_INVALID,
                          nd_board_spi_dma_start(&dma, tx, NULL, 2));
    TEST_ASSERT_EQUAL_INT(0, dma.busy);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_completion_waits_then_takes);
    RUN_TEST(test_completion_reports_isr_status);
    RUN_TEST(test_completion_timeout_is_bounded_and_kicks_watchdog);
    RUN_TEST(test_ring_preserves_order_across_wrap);
    RUN_TEST(test_ring_overrun_keeps_oldest_and_drops_newest);
    RUN_TEST(test_isr_completes_the_ready_primitive);
    RUN_TEST(test_uart_read_returns_buffered_byte);
    RUN_TEST(test_uart_read_times_out_on_empty_ring);
    RUN_TEST(test_spi_hw_ctx_reaches_the_hw_start);
    RUN_TEST(test_spi_two_channels_complete_in_any_order);
    RUN_TEST(test_spi_tx_only_completes_on_tx_done);
    RUN_TEST(test_spi_rejects_empty_and_busy_transfers);
    RUN_TEST(test_spi_transfer_timeout_stops_the_hardware);
    RUN_TEST(test_spi_transfer_chunks_long_buffers);
    RUN_TEST(test_spi_start_failure_is_reported);
    return UNITY_END();
}
