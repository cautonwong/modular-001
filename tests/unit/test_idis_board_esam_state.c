#include "unity.h"

#include <string.h>

#include "cpu_surport.h"
#include "lib_ana.h"
#include "targets/idis_ip_meter/board/board_esam.h"
#include "targets/idis_ip_meter/board/board_state.h"
#include "targets/idis_ip_meter/board/spi_driver.h"

static nd_idis_board_state_t *select_state;
static int select_calls;

uint8_t ANA_GetStatus(uint32_t StatusMask)
{
    (void)StatusMask;
    return 0;
}

void nd_esam_cs_set(int high)
{
    (void)high;
}

void nd_esam_power_set(int on)
{
    (void)on;
}

void nd_board_spi_select(nd_idis_board_state_t *state, nd_board_spi_t bus,
                         int active)
{
    (void)bus;
    (void)active;
    select_state = state;
    select_calls++;
}

void nd_board_spi_flush(nd_idis_board_state_t *state, nd_board_spi_t bus)
{
    (void)state;
    (void)bus;
}

int nd_board_spi_transfer(nd_idis_board_state_t *state, nd_board_spi_t bus,
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
    select_state = NULL;
    select_calls = 0;
}

void tearDown(void) {}

static void test_null_state_is_rejected(void)
{
    TEST_ASSERT_NULL(board_esam(NULL));
}

static void test_power_on_forwards_the_bound_state(void)
{
    nd_idis_board_state_t state;
    nd_esam_t *esam;

    memset(&state, 0, sizeof(state));
    esam = board_esam(&state);
    TEST_ASSERT_NOT_NULL(esam);
    TEST_ASSERT_EQUAL_PTR(esam, board_esam(&state));
    TEST_ASSERT_EQUAL_INT(0, nd_esam_power_on(esam));
    TEST_ASSERT_GREATER_THAN_INT(0, select_calls);
    TEST_ASSERT_EQUAL_PTR(&state, select_state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_state_is_rejected);
    RUN_TEST(test_power_on_forwards_the_bound_state);
    return UNITY_END();
}
