#include "unity.h"

#include <string.h>

#include "cpu_surport.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/app/storage_device.h"
#include "targets/stategrid_meter/board/board_eeprom.h"
#include "targets/stategrid_meter/board/board_flash.h"
#include "targets/stategrid_meter/board/spi_driver.h"
#include "targets/stategrid_meter/board/board_state.h"

static int iic_read_result;
static int iic_write_result;
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

static uint8_t flash_mem[4096];
static uint8_t flash_opcode;
static uint32_t flash_addr;

void nd_board_spi_init(nd_board_spi_t bus)
{
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
    if (tx != NULL) {
        flash_opcode = tx[0];
        if (len >= 4 && flash_opcode == 0x03) {
            flash_addr = ((uint32_t)tx[1] << 16) | ((uint32_t)tx[2] << 8)
                         | (uint32_t)tx[3];
        }
        return 0;
    }
    if (rx != NULL) {
        if (flash_opcode == 0x9F) {
            rx[0] = 0xC2;
            if (len > 1) {
                rx[1] = 0x20;
            }
            if (len > 2) {
                rx[2] = 0x16;
            }
        } else if (flash_opcode == 0x03) {
            for (uint16_t i = 0; i < len; i++) {
                rx[i] = flash_mem[(flash_addr++) & 0xFFFu];
            }
        } else {
            memset(rx, 0, len);
        }
    }
    return 0;
}

void nd_spi_flash_power_set(int on)
{
    (void)on;
}

void setUp(void)
{
    iic_read_result = 0;
    iic_write_result = 0;
    iic_read_byte = 0x5a;
}

void tearDown(void) {}

static void test_null_state_is_rejected(void)
{
    stategrid_context_t context;
    uint8_t byte = 0;

    memset(&context, 0, sizeof(context));
    TEST_ASSERT_NULL(board_eeprom(NULL));
    TEST_ASSERT_EQUAL_UINT8(0, nd_board_device_error(NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_storage_read(NULL, RES_DEV_EEPROM, 0, &byte,
                                              1));
    TEST_ASSERT_EQUAL_INT(-1, nd_storage_write(NULL, RES_DEV_EEPROM, 0, &byte,
                                               1));
    TEST_ASSERT_EQUAL_INT(-1, nd_storage_erase(NULL, RES_DEV_FLASH, 0));
    TEST_ASSERT_EQUAL_UINT32(0, nd_storage_size(NULL, RES_DEV_EEPROM));
    context.board = NULL;
    TEST_ASSERT_EQUAL_INT(-1, nd_storage_read(&context, RES_DEV_EEPROM, 0,
                                              &byte, 1));
    TEST_ASSERT_EQUAL_UINT32(0, nd_storage_size(&context, RES_DEV_EEPROM));
}

static void test_lazy_init_binds_state_storage(void)
{
    stategrid_board_state_t state;
    nd_eeprom_t *eeprom;

    memset(&state, 0, sizeof(state));
    eeprom = board_eeprom(&state);
    TEST_ASSERT_NOT_NULL(eeprom);
    TEST_ASSERT_EQUAL_PTR(eeprom, state.eeprom);
    TEST_ASSERT_EQUAL_PTR(&state.eeprom_storage, (void *)eeprom);
    TEST_ASSERT_EQUAL_PTR(eeprom, board_eeprom(&state));
}

static void test_errors_update_the_bound_state(void)
{
    stategrid_board_state_t state;
    uint8_t byte = 0;

    memset(&state, 0, sizeof(state));
    TEST_ASSERT_NOT_NULL(board_eeprom(&state));
    iic_read_result = -1;
    TEST_ASSERT_EQUAL_INT(-1, nd_eeprom_read(state.eeprom, 0, &byte, 1));
    TEST_ASSERT_EQUAL_UINT8(5, state.error_streak);
    TEST_ASSERT_EQUAL_UINT8(0x02, state.DEVICE_ERR);
    TEST_ASSERT_EQUAL_UINT8(1, nd_board_device_error(&state));

    iic_read_result = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_read(state.eeprom, 0, &byte, 1));
    TEST_ASSERT_EQUAL_UINT8(0, state.error_streak);
    TEST_ASSERT_EQUAL_UINT8(0, state.DEVICE_ERR);
    TEST_ASSERT_EQUAL_UINT8(0, nd_board_device_error(&state));
}

static void test_storage_read_reaches_the_bound_eeprom(void)
{
    stategrid_context_t context;
    stategrid_board_state_t state;
    uint8_t byte = 0;

    memset(&context, 0, sizeof(context));
    memset(&state, 0, sizeof(state));
    context.board = &state;
    TEST_ASSERT_NOT_NULL(board_eeprom(&state));
    TEST_ASSERT_EQUAL_INT(0, nd_storage_read(&context, RES_DEV_EEPROM, 0,
                                             &byte, 1));
    TEST_ASSERT_EQUAL_HEX8(0x5a, byte);
    TEST_ASSERT_EQUAL_UINT8(0, state.DEVICE_ERR);
}

static void test_flash_null_state_is_rejected(void)
{
    TEST_ASSERT_NULL(board_flash(NULL));
}

static void test_flash_lazy_init_binds_state_storage(void)
{
    stategrid_board_state_t state;
    nd_mx25l3206e_t *flash;

    memset(&state, 0, sizeof(state));
    flash = board_flash(&state);
    TEST_ASSERT_NOT_NULL(flash);
    TEST_ASSERT_EQUAL_PTR(flash, state.flash);
    TEST_ASSERT_EQUAL_PTR(&state.flash_storage, (void *)flash);
    TEST_ASSERT_EQUAL_PTR(flash, board_flash(&state));
}

static void test_flash_states_are_independent(void)
{
    stategrid_board_state_t first;
    stategrid_board_state_t second;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    TEST_ASSERT_TRUE(board_flash(&first) != board_flash(&second));
    TEST_ASSERT_EQUAL_PTR(&first.flash_storage, (void *)first.flash);
    TEST_ASSERT_EQUAL_PTR(&second.flash_storage, (void *)second.flash);
}

static void test_storage_read_reaches_the_bound_flash(void)
{
    stategrid_context_t context;
    stategrid_board_state_t state;
    uint8_t byte = 0;

    memset(&context, 0, sizeof(context));
    memset(&state, 0, sizeof(state));
    context.board = &state;
    flash_mem[0] = 0xA7;
    TEST_ASSERT_NOT_NULL(board_flash(&state));
    TEST_ASSERT_EQUAL_INT(0, nd_storage_read(&context, RES_DEV_FLASH, 0,
                                             &byte, 1));
    TEST_ASSERT_EQUAL_HEX8(0xA7, byte);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_state_is_rejected);
    RUN_TEST(test_flash_null_state_is_rejected);
    RUN_TEST(test_flash_lazy_init_binds_state_storage);
    RUN_TEST(test_flash_states_are_independent);
    RUN_TEST(test_storage_read_reaches_the_bound_flash);
    RUN_TEST(test_lazy_init_binds_state_storage);
    RUN_TEST(test_errors_update_the_bound_state);
    RUN_TEST(test_storage_read_reaches_the_bound_eeprom);
    return UNITY_END();
}
