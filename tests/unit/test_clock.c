#include "unity.h"

#include "domain/common/clock.h"

static nd_clock_config_t config;

void setUp(void)
{
    nd_clock_config_init(&config);
    config.daylight_enabled = 1;
    config.time_zone = 60;
    config.daylight_deviation = 60;
    config.daylight_begin = (nd_clock_datetime_t){
        .month = 3, .day = 25, .hour = 2, .minute = 0,
        .wildcard_mask = ND_CLOCK_WILD_SECOND,
    };
    config.daylight_end = (nd_clock_datetime_t){
        .month = 10, .day = 25, .hour = 3, .minute = 0,
        .wildcard_mask = ND_CLOCK_WILD_SECOND,
    };
}

void tearDown(void) {}

static void test_dst_window_and_deviation(void)
{
    nd_datetime_t summer = {.year = 2026, .month = 6, .day = 1};
    nd_datetime_t winter = {.year = 2026, .month = 12, .day = 1};
    int16_t deviation = 0;
    TEST_ASSERT_TRUE(nd_clock_dst_active(&config, &summer));
    TEST_ASSERT_FALSE(nd_clock_dst_active(&config, &winter));
    TEST_ASSERT_EQUAL_INT(0, nd_clock_effective_deviation(&config, &summer,
                                                          &deviation));
    TEST_ASSERT_EQUAL_INT16(120, deviation);
    TEST_ASSERT_EQUAL_INT(0, nd_clock_effective_deviation(&config, &winter,
                                                          &deviation));
    TEST_ASSERT_EQUAL_INT16(60, deviation);
    config.daylight_enabled = 0;
    TEST_ASSERT_FALSE(nd_clock_dst_active(&config, &summer));
}

static void test_quarter_and_minute_rounding(void)
{
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11, .hour = 10,
                          .minute = 7, .second = 29};
    TEST_ASSERT_EQUAL_INT(0, nd_clock_adjust_to_quarter(&time));
    TEST_ASSERT_EQUAL_UINT8(10, time.hour);
    TEST_ASSERT_EQUAL_UINT8(0, time.minute);
    time.minute = 7;
    time.second = 31;
    TEST_ASSERT_EQUAL_INT(0, nd_clock_adjust_to_quarter(&time));
    TEST_ASSERT_EQUAL_UINT8(15, time.minute);
    time.minute = 7;
    time.second = 29;
    TEST_ASSERT_EQUAL_INT(0, nd_clock_adjust_to_minute(&time));
    TEST_ASSERT_EQUAL_UINT8(7, time.minute);
    time.second = 30;
    TEST_ASSERT_EQUAL_INT(0, nd_clock_adjust_to_minute(&time));
    TEST_ASSERT_EQUAL_UINT8(8, time.minute);
}

static void test_period_and_shift(void)
{
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11, .hour = 10,
                          .minute = 7, .second = 29};
    TEST_ASSERT_EQUAL_INT(0, nd_clock_adjust_to_period(&time, 900));
    TEST_ASSERT_EQUAL_UINT8(0, time.minute);
    TEST_ASSERT_EQUAL_UINT8(0, time.second);
    TEST_ASSERT_EQUAL_INT(0, nd_clock_adjust_to_period(&time, 900));
    TEST_ASSERT_EQUAL_UINT8(0, time.minute);
    TEST_ASSERT_EQUAL_INT(0, nd_clock_shift(&time, 3661));
    TEST_ASSERT_EQUAL_UINT8(11, time.hour);
    TEST_ASSERT_EQUAL_UINT8(1, time.minute);
    TEST_ASSERT_EQUAL_UINT8(1, time.second);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dst_window_and_deviation);
    RUN_TEST(test_quarter_and_minute_rounding);
    RUN_TEST(test_period_and_shift);
    return UNITY_END();
}
