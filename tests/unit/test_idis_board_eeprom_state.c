#include "unity.h"

#include <string.h>

#include "cpu_surport.h"
#include "targets/idis_ip_meter/board/board_eeprom.h"
#include "targets/idis_ip_meter/board/board_state.h"

static int iic_read_result;
static int iic_write_result;
static int iic_read_calls;
static uint8_t iic_read_byte;

void nd_board_iic_init(void)
{
}

int nd_board_iic_write(uint8_t device, const uint8_t *prefix,
                       uint16_t prefix_len, const uint8_t *buf, uint16_t len)
{
    (void)device;
    (void)prefix;
    (void)prefix_len;
    (void)buf;
    (void)len;
    return iic_write_result;
}

int nd_board_iic_read(uint8_t device, const uint8_t *prefix,
                      uint16_t prefix_len, uint8_t *buf, uint16_t len)
{
    (void)device;
    (void)prefix;
    (void)prefix_len;
    iic_read_calls++;
    if (iic_read_result == 0) {
        memset(buf, iic_read_byte, len);
    }
    return iic_read_result;
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
    iic_read_result = 0;
    iic_write_result = 0;
    iic_read_calls = 0;
    iic_read_byte = 0x5a;
}

void tearDown(void) {}

static void test_null_state_is_rejected(void)
{
    TEST_ASSERT_NULL(board_eeprom(NULL));
}

static void test_lazy_init_binds_state_storage(void)
{
    nd_idis_board_state_t state;
    nd_eeprom_t *eeprom;

    memset(&state, 0, sizeof(state));
    eeprom = board_eeprom(&state);
    TEST_ASSERT_NOT_NULL(eeprom);
    TEST_ASSERT_EQUAL_PTR(eeprom, state.eeprom);
    TEST_ASSERT_EQUAL_PTR(&state.eeprom_storage, (void *)eeprom);
    TEST_ASSERT_EQUAL_PTR(eeprom, board_eeprom(&state));
}

static void test_state_receives_read_results(void)
{
    nd_idis_board_state_t state;
    uint8_t byte = 0;

    memset(&state, 0, sizeof(state));
    TEST_ASSERT_NOT_NULL(board_eeprom(&state));
    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_read(state.eeprom, 0, &byte, 1));
    TEST_ASSERT_EQUAL_HEX8(0x5a, byte);
    TEST_ASSERT_EQUAL_UINT8(0, state.error_streak);
    TEST_ASSERT_EQUAL_UINT8(0, state.DEVICE_ERR);
}

static void test_errors_update_the_bound_state(void)
{
    nd_idis_board_state_t state;
    uint8_t byte = 0;

    memset(&state, 0, sizeof(state));
    TEST_ASSERT_NOT_NULL(board_eeprom(&state));
    iic_read_result = -1;
    TEST_ASSERT_EQUAL_INT(-1, nd_eeprom_read(state.eeprom, 0, &byte, 1));
    TEST_ASSERT_EQUAL_UINT8(5, state.error_streak);
    TEST_ASSERT_EQUAL_UINT8(0x02, state.DEVICE_ERR);

    iic_read_result = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_read(state.eeprom, 0, &byte, 1));
    TEST_ASSERT_EQUAL_UINT8(0, state.error_streak);
    TEST_ASSERT_EQUAL_UINT8(0, state.DEVICE_ERR);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_state_is_rejected);
    RUN_TEST(test_lazy_init_binds_state_storage);
    RUN_TEST(test_state_receives_read_results);
    RUN_TEST(test_errors_update_the_bound_state);
    return UNITY_END();
}
