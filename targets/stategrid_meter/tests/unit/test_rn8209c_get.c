/*
 * rn8209c_get and rn8209c_set serve the two 30-byte self-heating compensation
 * blocks. The 1.0L block is selected by sub-address 0x90..0x9E and the 0.5L
 * block by 0xA0..0xAE, both indexed from the start of the block. This drives
 * the public register entries and checks the exact bytes read and written,
 * and that an address outside either block neither reads nor writes.
 */
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "application/adapters/dl698/meter_base_view.h"
#include "components/core/types/utils.h"
#include "domain/metering/metering_input.h"
#include "infra/drivers/metering/rn8209/rn8209.h"
#include "infra/soc/v85xxp/v8500/cpu_surport.h"
#include "targets/stategrid_meter/app/metering/rn8209_target.h"
#include "targets/stategrid_meter/app/port.h"
#include "targets/stategrid_meter/board/board_rn8209.h"
#include "targets/stategrid_meter/board/meter_status.h"

int nd_get_index(void *instance, uint32_t index, uint8_t *buffer,
                 uint16_t length);
int nd_set_index(void *instance, uint32_t index, const uint8_t *buffer,
                 uint16_t length);
int start_emu_signal_pro(void);
uint8_t rn8209c_get(void *instance, uint8_t *p_rx, uint8_t *p_tx);
int rn8209c_set(void *instance, uint8_t *p_rx);

static int16_t block[15];
static int16_t written[15];
static uint32_t last_index;
static uint32_t last_set_index;
static uint16_t last_length;
static uint16_t last_set_length;
static unsigned int reads;
static unsigned int sets;

static uint32_t mb_active_avg_power;
static uint32_t mb_active_power;
static uint32_t mb_apparent_power;
static uint32_t mb_avg_cal_start_flag;
static uint32_t mb_current_l;
static uint32_t mb_current_n;
static uint32_t mb_curt_33;
static uint32_t mb_emu_err_s_status;
static uint32_t mb_frequency;
static uint32_t mb_meter_start_cnt;
static uint32_t mb_metertype;
static uint32_t mb_n_pluse;
static uint32_t mb_no_filt;
static uint32_t mb_p_pluse;
static uint32_t mb_power_33;
static uint32_t mb_power_dir;
static uint32_t mb_power_factor;
static uint32_t mb_temperature;
static uint32_t mb_volt_120;
static uint32_t mb_volt_60;
static uint32_t mb_volt_78;
static uint32_t mb_voltage_ac;
static uint32_t mb_voltage_battery;

int32_t *nd_mb_active_avg_power_ptr(void *instance)
{
    (void)instance;
    return (int32_t *)&mb_active_avg_power;
}

int32_t *nd_mb_active_power_ptr(void *instance)
{
    (void)instance;
    return (int32_t *)&mb_active_power;
}

uint32_t *nd_mb_apparent_power_ptr(void *instance)
{
    (void)instance;
    return &mb_apparent_power;
}

uint8_t *nd_mb_avg_cal_start_flag_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_avg_cal_start_flag;
}

int32_t *nd_mb_current_l_ptr(void *instance)
{
    (void)instance;
    return (int32_t *)&mb_current_l;
}

int32_t *nd_mb_current_n_ptr(void *instance)
{
    (void)instance;
    return (int32_t *)&mb_current_n;
}

uint32_t *nd_mb_curt_33_ptr(void *instance)
{
    (void)instance;
    return &mb_curt_33;
}

uint8_t *nd_mb_emu_err_s_status_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_emu_err_s_status;
}

uint32_t *nd_mb_frequency_ptr(void *instance)
{
    (void)instance;
    return &mb_frequency;
}

uint8_t *nd_mb_meter_start_cnt_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_meter_start_cnt;
}

uint16_t *nd_mb_metertype_ptr(void *instance)
{
    (void)instance;
    return (uint16_t *)&mb_metertype;
}

uint8_t *nd_mb_n_pluse_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_n_pluse;
}

uint8_t *nd_mb_no_filt_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_no_filt;
}

uint8_t *nd_mb_p_pluse_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_p_pluse;
}

uint32_t *nd_mb_power_33_ptr(void *instance)
{
    (void)instance;
    return &mb_power_33;
}

uint8_t *nd_mb_power_dir_ptr(void *instance)
{
    (void)instance;
    return (uint8_t *)&mb_power_dir;
}

int32_t *nd_mb_power_factor_ptr(void *instance)
{
    (void)instance;
    return (int32_t *)&mb_power_factor;
}

int32_t *nd_mb_temperature_ptr(void *instance)
{
    (void)instance;
    return (int32_t *)&mb_temperature;
}

uint32_t *nd_mb_volt_120_ptr(void *instance)
{
    (void)instance;
    return &mb_volt_120;
}

uint32_t *nd_mb_volt_60_ptr(void *instance)
{
    (void)instance;
    return &mb_volt_60;
}

uint32_t *nd_mb_volt_78_ptr(void *instance)
{
    (void)instance;
    return &mb_volt_78;
}

uint32_t *nd_mb_voltage_ac_ptr(void *instance)
{
    (void)instance;
    return &mb_voltage_ac;
}

uint32_t *nd_mb_voltage_battery_ptr(void *instance)
{
    (void)instance;
    return &mb_voltage_battery;
}

int nd_get_index(void *instance, uint32_t index, uint8_t *buffer,
                 uint16_t length)
{
    (void)instance;
    reads++;
    last_index = index;
    last_length = length;
    size_t bytes = length < sizeof(block) ? length : sizeof(block);
    memcpy(buffer, block, bytes);
    return 0;
}

int nd_set_index(void *instance, uint32_t index, const uint8_t *buffer,
                 uint16_t length)
{
    (void)instance;
    sets++;
    last_set_index = index;
    last_set_length = length;
    size_t bytes = length < sizeof(written) ? length : sizeof(written);
    memcpy(written, buffer, bytes);
    return 0;
}

int start_emu_signal_pro(void)
{
    return 0;
}

nd_rn8209_t *board_rn8209(void)
{
    return NULL;
}

int nd_rn8209_read(nd_rn8209_t *dev, uint8_t reg, void *buf, uint8_t len)
{
    (void)dev;
    (void)reg;
    (void)buf;
    (void)len;
    return -1;
}

int nd_rn8209_write(nd_rn8209_t *dev, uint8_t reg, const void *buf, uint8_t len)
{
    (void)dev;
    (void)reg;
    (void)buf;
    (void)len;
    return -1;
}

int nd_rn8209_write_raw(nd_rn8209_t *dev, uint8_t reg, const void *buf,
                        uint8_t len)
{
    (void)dev;
    (void)reg;
    (void)buf;
    (void)len;
    return -1;
}

int nd_rn8209_command(nd_rn8209_t *dev, uint8_t command)
{
    (void)dev;
    (void)command;
    return -1;
}

void nd_rn8209_delay_us(nd_rn8209_t *dev, uint32_t us)
{
    (void)dev;
    (void)us;
}

int nd_rn8209_control(nd_rn8209_t *dev, nd_rn8209_control_t control,
                      const nd_rn8209_control_param_t *param)
{
    (void)dev;
    (void)control;
    (void)param;
    return -1;
}

void mcu_clear_watchdog(void) {}

uint8_t mcu_delay_us(volatile uint16_t count)
{
    (void)count;
    return 0;
}

int nd_metering_input_read(nd_metering_input_t input, int32_t *value)
{
    (void)input;
    (void)value;
    return -1;
}

uint16_t nd_meter_status_metertype(void *ctx)
{
    (void)ctx;
    return 0;
}

void nd_meter_status_emu_error_set(void *ctx, int is_error)
{
    (void)ctx;
    (void)is_error;
}

uint32_t long_bcd_tohex(uint32_t bcd)
{
    return bcd;
}

uint8_t mem_compare(void *data1, void *data2, uint16_t length)
{
    return (uint8_t)memcmp(data1, data2, length);
}

struct ADJ_POFFSET_T adj_poffset;

static void test_05l_blocks_use_their_own_registers(void)
{
    for (uint8_t addr = 0xA0; addr <= 0xAE; addr++) {
        uint8_t rx = addr;
        uint8_t tx[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        TEST_ASSERT_EQUAL_UINT8(2, rn8209c_get(NULL, &rx, tx));
        TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)&block[addr - 0xA0],
                                      (const uint8_t *)(tx + 1), 2);
        TEST_ASSERT_EQUAL_UINT32(Heat_Value_05L, last_index & 0xFFFFu);
        TEST_ASSERT_EQUAL_UINT16(sizeof(block), last_length);
    }
}

static void test_10l_blocks_use_their_own_registers(void)
{
    for (uint8_t addr = 0x90; addr <= 0x9E; addr++) {
        uint8_t rx = addr;
        uint8_t tx[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        TEST_ASSERT_EQUAL_UINT8(2, rn8209c_get(NULL, &rx, tx));
        TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)&block[addr - 0x90],
                                      (const uint8_t *)(tx + 1), 2);
        TEST_ASSERT_EQUAL_UINT32(Heat_Value_10L, last_index & 0xFFFFu);
        TEST_ASSERT_EQUAL_UINT16(sizeof(block), last_length);
    }
}

static void test_out_of_range_sub_addresses_are_rejected(void)
{
    const uint8_t rejected[] = {0x9F, 0xAF, 0xB0, 0xFF};
    for (size_t index = 0; index < sizeof(rejected); index++) {
        unsigned int before = reads;
        uint8_t rx = rejected[index];
        uint8_t tx[8];
        memset(tx, 0xFF, sizeof(tx));
        TEST_ASSERT_EQUAL_UINT8(0, rn8209c_get(NULL, &rx, tx));
        TEST_ASSERT_EQUAL_UINT(before, reads);
    }
}

static void test_set_writes_the_05l_block(void)
{
    const uint8_t addr = 0xA5;
    uint8_t request[5] = {0x34, 0x12, 0, 0, addr};
    int16_t expected[15];
    memcpy(expected, block, sizeof(expected));
    expected[addr - 0xA0] = 0x1234;

    TEST_ASSERT_EQUAL_INT(1, rn8209c_set(NULL, request));
    TEST_ASSERT_EQUAL_UINT(1u, sets);
    TEST_ASSERT_EQUAL_UINT32(Heat_Value_05L, last_set_index & 0xFFFFu);
    TEST_ASSERT_EQUAL_UINT16(sizeof(written), last_set_length);
    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, written, 15);
}

static void test_set_writes_the_10l_block(void)
{
    const uint8_t addr = 0x93;
    uint8_t request[5] = {0x78, 0x56, 0, 0, addr};
    int16_t expected[15];
    memcpy(expected, block, sizeof(expected));
    expected[addr - 0x90] = 0x5678;

    TEST_ASSERT_EQUAL_INT(1, rn8209c_set(NULL, request));
    TEST_ASSERT_EQUAL_UINT(1u, sets);
    TEST_ASSERT_EQUAL_UINT32(Heat_Value_10L, last_set_index & 0xFFFFu);
    TEST_ASSERT_EQUAL_UINT16(sizeof(written), last_set_length);
    TEST_ASSERT_EQUAL_INT16_ARRAY(expected, written, 15);
}

static void test_set_rejects_out_of_range_sub_addresses(void)
{
    const uint8_t rejected[] = {0x9F, 0xAF, 0xB0, 0xBD, 0xBE, 0xFF};
    for (size_t index = 0; index < sizeof(rejected); index++) {
        unsigned int reads_before = reads;
        unsigned int sets_before = sets;
        uint8_t request[5] = {0x34, 0x12, 0, 0, rejected[index]};
        TEST_ASSERT_EQUAL_INT(0, rn8209c_set(NULL, request));
        TEST_ASSERT_EQUAL_UINT(reads_before, reads);
        TEST_ASSERT_EQUAL_UINT(sets_before, sets);
    }
}

void setUp(void)
{
    for (size_t index = 0; index < 15; index++) {
        block[index] = (int16_t)(0x1000 + index * 0x0101);
    }
    memset(written, 0, sizeof(written));
    last_index = 0;
    last_set_index = 0;
    last_length = 0;
    last_set_length = 0;
    reads = 0;
    sets = 0;
}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_05l_blocks_use_their_own_registers);
    RUN_TEST(test_10l_blocks_use_their_own_registers);
    RUN_TEST(test_out_of_range_sub_addresses_are_rejected);
    RUN_TEST(test_set_writes_the_05l_block);
    RUN_TEST(test_set_writes_the_10l_block);
    RUN_TEST(test_set_rejects_out_of_range_sub_addresses);
    return UNITY_END();
}
