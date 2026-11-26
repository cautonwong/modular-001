#include "unity.h"

#include <string.h>

#include "cpu_surport.h"
#include "lib_ana.h"
#include "targets/stategrid_meter/board/board_rn8209.h"
#include "targets/stategrid_meter/board/board_state.h"
#include "targets/stategrid_meter/board/rn8209_pins.h"
#include "targets/stategrid_meter/board/uart_driver.h"

static int uart_init_calls;

uint8_t ANA_GetStatus(uint32_t status_mask)
{
    (void)status_mask;
    return 0;
}

void nd_board_uart_init(stategrid_board_state_t *state, size_t index,
                        const void *config)
{
    (void)state;
    (void)index;
    (void)config;
    uart_init_calls++;
}

void nd_board_uart_tx_enable(stategrid_board_state_t *state, size_t index,
                             uint8_t enable)
{
    (void)state;
    (void)index;
    (void)enable;
}

void nd_board_uart_rx_enable(stategrid_board_state_t *state, size_t index,
                             uint8_t enable)
{
    (void)state;
    (void)index;
    (void)enable;
}

int nd_board_uart_write(stategrid_board_state_t *state, size_t index,
                        uint8_t byte)
{
    (void)state;
    (void)index;
    (void)byte;
    return 0;
}

int nd_board_uart_tx_done(stategrid_board_state_t *state, size_t index)
{
    (void)state;
    (void)index;
    return 1;
}

int nd_board_uart_read(stategrid_board_state_t *state, size_t index,
                       uint8_t *byte)
{
    (void)state;
    (void)index;
    if (byte != NULL) {
        *byte = 0;
    }
    return 0;
}

int nd_board_uart_rx_done(stategrid_board_state_t *state, size_t index)
{
    (void)state;
    (void)index;
    return 1;
}

void nd_rn8209_reset_line_output(void)
{
}

void nd_rn8209_reset_line_input(void)
{
}

void nd_rn8209_reset_line_set(int high)
{
    (void)high;
}

void nd_rn8209_tx_line_output(void)
{
}

void nd_rn8209_tx_line_set(int high)
{
    (void)high;
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
    uart_init_calls = 0;
}

void tearDown(void) {}

static void test_null_state_is_rejected(void)
{
    TEST_ASSERT_NULL(board_rn8209(NULL));
}

static void test_lazy_init_binds_state_storage(void)
{
    stategrid_board_state_t state;
    nd_rn8209_t *device;

    memset(&state, 0, sizeof(state));
    device = board_rn8209(&state);
    TEST_ASSERT_NOT_NULL(device);
    TEST_ASSERT_EQUAL_PTR(device, state.rn8209);
    TEST_ASSERT_EQUAL_PTR(&state.rn8209_storage, (void *)device);
    TEST_ASSERT_EQUAL_PTR(device, board_rn8209(&state));
}

static void test_two_states_have_independent_devices(void)
{
    stategrid_board_state_t first;
    stategrid_board_state_t second;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    TEST_ASSERT_TRUE(board_rn8209(&first) != board_rn8209(&second));
    TEST_ASSERT_EQUAL_PTR(&first.rn8209_storage, (void *)first.rn8209);
    TEST_ASSERT_EQUAL_PTR(&second.rn8209_storage, (void *)second.rn8209);
}

static void test_power_on_prepares_the_bound_state_uart(void)
{
    stategrid_board_state_t state;
    nd_rn8209_control_param_t param;

    memset(&state, 0, sizeof(state));
    memset(&param, 0, sizeof(param));
    TEST_ASSERT_NOT_NULL(board_rn8209(&state));
    TEST_ASSERT_EQUAL_INT(0, state.uart_ready);
    (void)nd_rn8209_control(state.rn8209, ND_RN8209_CONTROL_POWER_ON,
                            &param);
    TEST_ASSERT_EQUAL_INT(1, state.uart_ready);
    TEST_ASSERT_GREATER_THAN_INT(0, uart_init_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_state_is_rejected);
    RUN_TEST(test_lazy_init_binds_state_storage);
    RUN_TEST(test_two_states_have_independent_devices);
    RUN_TEST(test_power_on_prepares_the_bound_state_uart);
    return UNITY_END();
}
