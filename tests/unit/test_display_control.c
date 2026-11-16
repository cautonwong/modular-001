#include "unity.h"

#include "domain/display/backlight_control.h"

void setUp(void) {}
void tearDown(void) {}

static void test_rtc_battery_debounces_three_samples(void)
{
    nd_rtc_battery_state_t state = {0};
    nd_rtc_battery_tick(290, 290, 300, &state);
    nd_rtc_battery_tick(289, 290, 300, &state);
    TEST_ASSERT_EQUAL_UINT8(0, state.undervoltage);
    nd_rtc_battery_tick(288, 290, 300, &state);
    TEST_ASSERT_EQUAL_UINT8(1, state.undervoltage);

    nd_rtc_battery_tick(295, 290, 300, &state);
    nd_rtc_battery_tick(295, 290, 300, &state);
    TEST_ASSERT_EQUAL_UINT8(1, state.undervoltage);
    nd_rtc_battery_tick(300, 290, 300, &state);
    TEST_ASSERT_EQUAL_UINT8(1, state.undervoltage);
    nd_rtc_battery_tick(301, 290, 300, &state);
    nd_rtc_battery_tick(301, 290, 300, &state);
    TEST_ASSERT_EQUAL_UINT8(0, state.undervoltage);
}

static void test_backlight_page_and_alarm_force_on(void)
{
    nd_backlight_inputs_t inputs = {0};
    uint16_t timer = 0;
    inputs.page_id = 0xFFFF;
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));

    inputs.page_id = 1;
    inputs.remote_alarm = 1;
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));

    inputs.remote_alarm = 0;
    inputs.alarm_minutes_active = 1;
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));

    inputs.alarm_minutes_active = 0;
    inputs.current_limit_active = 1;
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));
}

static void test_backlight_battery_alarm_sets_timer(void)
{
    nd_backlight_inputs_t inputs = {0};
    uint16_t timer = 0;
    inputs.alarm_word = 0x00400000u;
    inputs.rtc_battery_undervoltage = 1;
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));
    TEST_ASSERT_EQUAL_UINT16(3, timer);

    inputs.rtc_battery_undervoltage = 0;
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));
    TEST_ASSERT_EQUAL_UINT16(2, timer);
    TEST_ASSERT_EQUAL_INT(1, nd_backlight_tick(&inputs, &timer));
    TEST_ASSERT_EQUAL_UINT16(1, timer);
    TEST_ASSERT_EQUAL_INT(0, nd_backlight_tick(&inputs, &timer));
    TEST_ASSERT_EQUAL_UINT16(0, timer);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rtc_battery_debounces_three_samples);
    RUN_TEST(test_backlight_page_and_alarm_force_on);
    RUN_TEST(test_backlight_battery_alarm_sets_timer);
    return UNITY_END();
}
