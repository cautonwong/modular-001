#include "unity.h"

#include <string.h>

#include "targets/idis_ip_meter/board/board_transport.h"

static int hw_start_calls;
static void *hw_ctxs[8];

int nd_board_spi_dma_hw_start(void *ctx, const uint8_t *tx, uint8_t *rx,
                              uint16_t len)
{
    (void)tx;
    (void)rx;
    (void)len;
    if (hw_start_calls < 8) {
        hw_ctxs[hw_start_calls] = ctx;
    }
    hw_start_calls++;
    return ctx == NULL ? -1 : ND_BOARD_OK;
}

void nd_board_spi_dma_hw_stop(void)
{
}

void nd_board_spi_dma_hw_wait_idle(void)
{
}

void nd_board_watchdog_kick(void)
{
}

void setUp(void)
{
    hw_start_calls = 0;
    memset(hw_ctxs, 0, sizeof(hw_ctxs));
}

void tearDown(void) {}

static void test_bound_context_reaches_hw_start(void)
{
    nd_board_spi_dma_t dma;
    uint8_t state;
    uint8_t byte = 0;

    nd_board_spi_dma_init(&dma);
    dma.hw_ctx = &state;
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK,
                          nd_board_spi_dma_start(&dma, &byte, NULL, 1));
    TEST_ASSERT_EQUAL_INT(1, hw_start_calls);
    TEST_ASSERT_EQUAL_PTR(&state, hw_ctxs[0]);
}

static void test_unbound_context_is_not_shared(void)
{
    nd_board_spi_dma_t first;
    nd_board_spi_dma_t second;
    uint8_t state;
    uint8_t byte = 0;

    nd_board_spi_dma_init(&first);
    first.hw_ctx = &state;
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK,
                          nd_board_spi_dma_start(&first, &byte, NULL, 1));

    memset(&second, 0, sizeof(second));
    nd_board_spi_dma_init(&second);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_INVALID,
                          nd_board_spi_dma_start(&second, &byte, NULL, 1));
    TEST_ASSERT_EQUAL_INT(2, hw_start_calls);
    TEST_ASSERT_EQUAL_PTR(&state, hw_ctxs[0]);
    TEST_ASSERT_NULL(hw_ctxs[1]);
    TEST_ASSERT_EQUAL_UINT8(0, second.busy);
    TEST_ASSERT_EQUAL_UINT8(0, second.tx_active);
    TEST_ASSERT_EQUAL_UINT8(0, second.rx_active);
}

static void test_init_keeps_a_context_bound_before_init(void)
{
    nd_board_spi_dma_t dma;
    uint8_t state;
    uint8_t byte = 0;

    memset(&dma, 0, sizeof(dma));
    dma.hw_ctx = &state;
    nd_board_spi_dma_init(&dma);
    TEST_ASSERT_EQUAL_PTR(&state, dma.hw_ctx);
    TEST_ASSERT_EQUAL_INT(ND_BOARD_OK,
                          nd_board_spi_dma_start(&dma, &byte, NULL, 1));
    TEST_ASSERT_EQUAL_PTR(&state, hw_ctxs[0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bound_context_reaches_hw_start);
    RUN_TEST(test_unbound_context_is_not_shared);
    RUN_TEST(test_init_keeps_a_context_bound_before_init);
    return UNITY_END();
}
