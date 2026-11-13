#include "unity.h"

#include "domain/events/mac_suspend.h"

void setUp(void) {}
void tearDown(void) {}

static nd_datetime_t at(uint8_t day, uint8_t hour, uint8_t minute,
                        uint8_t second)
{
    nd_datetime_t now = {
        .second = second,
        .minute = minute,
        .hour = hour,
        .day = day,
        .month = 8,
        .year = 2026,
    };
    return now;
}

static void test_mac_errors_count_up_to_limit(void)
{
    nd_mac_suspend_state_t state = {0};
    nd_datetime_t now = at(11, 10, 0, 0);
    uint16_t i;

    for (i = 0; i < 199; i++) {
        nd_mac_suspend_add(&state, 200, &now);
    }
    TEST_ASSERT_EQUAL_UINT16(199, state.error_count);
    TEST_ASSERT_EQUAL_UINT8(0, state.suspended);

    nd_mac_suspend_add(&state, 200, &now);
    TEST_ASSERT_EQUAL_UINT16(200, state.error_count);
    TEST_ASSERT_EQUAL_UINT8(1, state.suspended);
    TEST_ASSERT_EQUAL_UINT8(11, state.suspend_day);
    TEST_ASSERT_EQUAL_UINT16(2026, state.suspend_year);

    nd_mac_suspend_add(&state, 200, &now);
    TEST_ASSERT_EQUAL_UINT16(200, state.error_count);
}

static void test_mac_rollover_at_midnight_clears_and_reports(void)
{
    nd_mac_suspend_state_t state = {0};
    nd_datetime_t now = at(11, 0, 0, 0);

    state.error_count = 7;
    state.suspended = 1;
    TEST_ASSERT_EQUAL_UINT8(1, nd_mac_suspend_step(&state, 200, &now));
    TEST_ASSERT_EQUAL_UINT16(0, state.error_count);
    TEST_ASSERT_EQUAL_UINT8(0, state.suspended);
}

static void test_mac_rollover_on_date_change_clears_and_reports(void)
{
    nd_mac_suspend_state_t state = {0};
    nd_datetime_t now = at(12, 10, 0, 0);

    state.error_count = 7;
    state.last_date_set = 1;
    state.last_day = 11;
    state.last_month = 8;
    state.last_year = 2026;
    TEST_ASSERT_EQUAL_UINT8(1, nd_mac_suspend_step(&state, 200, &now));
    TEST_ASSERT_EQUAL_UINT16(0, state.error_count);
}

static void test_mac_rollover_on_suspend_date_change_clears(void)
{
    nd_mac_suspend_state_t state = {0};
    nd_datetime_t now = at(11, 10, 0, 0);

    state.error_count = 7;
    state.last_date_set = 1;
    state.last_day = 11;
    state.last_month = 8;
    state.last_year = 2026;
    state.suspend_date_set = 1;
    state.suspend_day = 10;
    state.suspend_month = 8;
    state.suspend_year = 2026;
    TEST_ASSERT_EQUAL_UINT8(0, nd_mac_suspend_step(&state, 200, &now));
    TEST_ASSERT_EQUAL_UINT16(0, state.error_count);
    TEST_ASSERT_EQUAL_UINT8(0, state.suspend_date_set);
}

static void test_mac_same_day_keeps_count_and_suspends_at_limit(void)
{
    nd_mac_suspend_state_t state = {0};
    nd_datetime_t now = at(11, 10, 0, 0);

    state.error_count = 200;
    state.last_date_set = 1;
    state.last_day = 11;
    state.last_month = 8;
    state.last_year = 2026;
    TEST_ASSERT_EQUAL_UINT8(0, nd_mac_suspend_step(&state, 200, &now));
    TEST_ASSERT_EQUAL_UINT16(200, state.error_count);
    TEST_ASSERT_EQUAL_UINT8(1, state.suspended);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mac_errors_count_up_to_limit);
    RUN_TEST(test_mac_rollover_at_midnight_clears_and_reports);
    RUN_TEST(test_mac_rollover_on_date_change_clears_and_reports);
    RUN_TEST(test_mac_rollover_on_suspend_date_change_clears);
    RUN_TEST(test_mac_same_day_keeps_count_and_suspends_at_limit);
    return UNITY_END();
}
