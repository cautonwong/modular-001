#include "unity.h"

#include <string.h>

#include "cpu_surport.h"
#include "lib_ana.h"
#include "targets/stategrid_meter/board/board_esam.h"
#include "targets/stategrid_meter/board/board_state.h"
#include "targets/stategrid_meter/board/spi_driver.h"

static int power_calls;
static int power_state;

uint8_t ANA_GetStatus(uint32_t status_mask)
{
    (void)status_mask;
    return 0;
}

void nd_esam_cs_set(int high)
{
    (void)high;
}

void nd_esam_power_set(int on)
{
    power_calls++;
    power_state = on;
}

void nd_board_spi_flush(stategrid_board_state_t *state, nd_board_spi_t bus)
{
    (void)state;
    (void)bus;
}

void nd_board_spi_select(stategrid_board_state_t *state, nd_board_spi_t bus,
                         int active)
{
    (void)state;
    (void)bus;
    (void)active;
}

int nd_board_spi_transfer(stategrid_board_state_t *state, nd_board_spi_t bus,
                          const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    (void)state;
    (void)bus;
    (void)tx;
    (void)rx;
    (void)len;
    return 0;
}

int mcu_delay_ms_power_down_quit(volatile uint16_t count)
{
    (void)count;
    return 0;
}

void mcu_clear_watchdog(void)
{
}

uint8_t mcu_delay_us(volatile uint16_t count)
{
    (void)count;
    return 0;
}

void setUp(void)
{
    power_calls = 0;
    power_state = -1;
}

void tearDown(void) {}

static void test_null_state_is_rejected(void)
{
    TEST_ASSERT_NULL(board_esam(NULL));
}

static void test_lazy_init_binds_state_storage(void)
{
    stategrid_board_state_t state;
    nd_esam_t *esam;

    memset(&state, 0, sizeof(state));
    esam = board_esam(&state);
    TEST_ASSERT_NOT_NULL(esam);
    TEST_ASSERT_EQUAL_PTR(esam, state.esam);
    TEST_ASSERT_EQUAL_PTR(&state.esam_storage, (void *)esam);
    TEST_ASSERT_EQUAL_PTR(esam, board_esam(&state));
}

static void test_two_states_have_independent_esam(void)
{
    stategrid_board_state_t first;
    stategrid_board_state_t second;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    TEST_ASSERT_TRUE(board_esam(&first) != board_esam(&second));
    TEST_ASSERT_EQUAL_PTR(&first.esam_storage, (void *)first.esam);
    TEST_ASSERT_EQUAL_PTR(&second.esam_storage, (void *)second.esam);
}

static void test_power_on_reaches_the_bound_state(void)
{
    stategrid_board_state_t state;

    memset(&state, 0, sizeof(state));
    TEST_ASSERT_NOT_NULL(board_esam(&state));
    TEST_ASSERT_EQUAL_INT(0, nd_esam_power_on(state.esam));
    TEST_ASSERT_GREATER_THAN_INT(0, power_calls);
    TEST_ASSERT_EQUAL_INT(1, power_state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_state_is_rejected);
    RUN_TEST(test_lazy_init_binds_state_storage);
    RUN_TEST(test_two_states_have_independent_esam);
    RUN_TEST(test_power_on_reaches_the_bound_state);
    return UNITY_END();
}
