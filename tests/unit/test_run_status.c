#include "unity.h"

#include "domain/metering/run_status.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static nd_run_status_inputs_t all_on(void)
{
    nd_run_status_inputs_t in;

    memset(&in, 0, sizeof(in));
    in.battery_mv = 299;
    in.power_direction = 1;
    in.switch_error = 1;
    in.esam_error = 1;
    in.memory_error = 1;
    in.rtc_error = 1;
    in.over_current = 1;
    in.infrared_authenticated = 1;
    in.terminal_authenticated = 1;
    in.terminal_security = 1;
    in.master_security = 1;
    in.relay_alarm = 1;
    in.relay_commanded_off = 1;
    in.protected_supply = 0;
    in.relay_off = 1;
    in.cover_open = 1;
    in.power_off = 1;
    return in;
}

static void test_all_conditions_set_their_bits(void)
{
    nd_run_status_inputs_t in = all_on();
    uint16_t words[8] = {0};

    nd_run_status_update(&in, words);

    TEST_ASSERT_EQUAL_HEX16(0xC528, words[1]);
    TEST_ASSERT_EQUAL_HEX16(0x0080, words[2]);
    TEST_ASSERT_EQUAL_HEX16(0x341B, words[3]);
    TEST_ASSERT_EQUAL_HEX16(0x0008, words[4]);
    TEST_ASSERT_EQUAL_HEX16(0x0000, words[0]);
    TEST_ASSERT_EQUAL_HEX16(0x0000, words[5]);
    TEST_ASSERT_EQUAL_HEX16(0x4004, words[7]);
}

static void test_protected_supply_clears_relay_command(void)
{
    nd_run_status_inputs_t in = all_on();
    uint16_t words[8] = {0};

    in.protected_supply = 1;
    nd_run_status_update(&in, words);

    TEST_ASSERT_EQUAL_HEX16(0x3C19, words[3]);
}

static void test_false_conditions_clear_only_owned_bits(void)
{
    nd_run_status_inputs_t in;
    uint16_t words[8];

    memset(&in, 0, sizeof(in));
    in.battery_mv = 300;
    memset(words, 0xFF, sizeof(words));

    nd_run_status_update(&in, words);

    TEST_ASSERT_EQUAL_HEX16(0x3AD7, words[1]);
    TEST_ASSERT_EQUAL_HEX16(0xFF7F, words[2]);
    TEST_ASSERT_EQUAL_HEX16(0xC3E4, words[3]);
    TEST_ASSERT_EQUAL_HEX16(0xFFF7, words[4]);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, words[0]);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, words[5]);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, words[6]);
    TEST_ASSERT_EQUAL_HEX16(0xBFFB, words[7]);
}

static void test_battery_threshold(void)
{
    nd_run_status_inputs_t in;
    uint16_t words[8] = {0};

    memset(&in, 0, sizeof(in));
    in.battery_mv = 299;
    nd_run_status_update(&in, words);
    TEST_ASSERT_BITS_HIGH(0x0020, words[1]);

    in.battery_mv = 300;
    nd_run_status_update(&in, words);
    TEST_ASSERT_BITS_LOW(0x0020, words[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_all_conditions_set_their_bits);
    RUN_TEST(test_protected_supply_clears_relay_command);
    RUN_TEST(test_false_conditions_clear_only_owned_bits);
    RUN_TEST(test_battery_threshold);
    return UNITY_END();
}
