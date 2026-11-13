#include "unity.h"

#include "domain/events/power_failure.h"

void setUp(void) {}

void tearDown(void) {}

static void test_long_and_short_outages(void)
{
    nd_power_failure_state_t state;
    nd_power_failure_init(&state, 60);
    nd_datetime_t off = {.year = 2026, .month = 11, .day = 11,
                         .hour = 10, .minute = 0, .second = 0};
    nd_datetime_t on = {.year = 2026, .month = 11, .day = 11,
                        .hour = 10, .minute = 1, .second = 5};
    TEST_ASSERT_EQUAL_INT(0, nd_power_failure_off(&state, &off));
    TEST_ASSERT_EQUAL_UINT32(65, nd_power_failure_on(&state, &on));
    TEST_ASSERT_EQUAL_UINT16(1, state.count);
    TEST_ASSERT_EQUAL_UINT16(1, state.long_count);
    TEST_ASSERT_EQUAL_UINT32(65, state.last_duration);
    TEST_ASSERT_EQUAL_UINT8(0, state.active);

    off.hour = 11;
    off.minute = 0;
    off.second = 0;
    on.hour = 11;
    on.minute = 0;
    on.second = 10;
    TEST_ASSERT_EQUAL_INT(0, nd_power_failure_off(&state, &off));
    TEST_ASSERT_EQUAL_UINT32(10, nd_power_failure_on(&state, &on));
    TEST_ASSERT_EQUAL_UINT16(2, state.count);
    TEST_ASSERT_EQUAL_UINT16(1, state.long_count);
}

static void test_duplicate_off_keeps_first_time(void)
{
    nd_power_failure_state_t state;
    nd_power_failure_init(&state, 10);
    nd_datetime_t off = {.year = 2026, .month = 11, .day = 11,
                         .hour = 8, .minute = 0, .second = 0};
    nd_datetime_t later = off;
    later.second = 30;
    nd_datetime_t on = off;
    on.minute = 1;
    TEST_ASSERT_EQUAL_INT(0, nd_power_failure_off(&state, &off));
    TEST_ASSERT_EQUAL_INT(0, nd_power_failure_off(&state, &later));
    TEST_ASSERT_EQUAL_UINT32(60, nd_power_failure_on(&state, &on));
    TEST_ASSERT_EQUAL_UINT16(1, state.long_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_long_and_short_outages);
    RUN_TEST(test_duplicate_off_keeps_first_time);
    return UNITY_END();
}
