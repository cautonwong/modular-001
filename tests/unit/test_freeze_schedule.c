#include "unity.h"

#include "domain/freeze/schedule.h"

void setUp(void) {}

void tearDown(void) {}

static nd_datetime_t datetime(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second)
{
    nd_datetime_t value = {second, minute, hour, day, month, year};
    return value;
}

static nd_freeze_period_t period(uint16_t interval)
{
    nd_freeze_period_t value = {1, interval};
    return value;
}

static void test_period_needs_a_valid_interval_and_unit(void)
{
    nd_datetime_t now = datetime(2000, 1, 1, 0, 0, 0);
    nd_freeze_period_t invalid = {0, 15};
    nd_freeze_period_t zero = {1, 0};
    nd_freeze_period_t fifteen = period(15);

    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&now, &invalid, 1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&now, &zero, 1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&now, &fifteen, 0));
}

static void test_minute_period_fires_on_its_interval(void)
{
    nd_freeze_period_t fifteen = period(15);
    nd_datetime_t epoch = datetime(2000, 1, 1, 0, 0, 0);
    nd_datetime_t at_15 = datetime(2000, 1, 1, 0, 15, 0);
    nd_datetime_t not_15 = datetime(2000, 1, 1, 0, 14, 0);
    nd_datetime_t off_boundary = datetime(2000, 1, 1, 0, 15, 1);

    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&epoch, &fifteen, 1));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&at_15, &fifteen, 1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&not_15, &fifteen, 1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&off_boundary, &fifteen, 1));
}

static void test_period_keeps_the_epoch_phase_for_odd_intervals(void)
{
    nd_freeze_period_t seven = period(7);
    nd_datetime_t epoch = datetime(2000, 1, 1, 0, 0, 0);
    nd_datetime_t at_7 = datetime(2000, 1, 1, 0, 7, 0);
    nd_datetime_t at_1 = datetime(2000, 1, 1, 0, 1, 0);

    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&epoch, &seven, 1));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&at_7, &seven, 1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&at_1, &seven, 1));
}

static void test_hour_period_is_measured_in_wall_clock_hours(void)
{
    nd_freeze_period_t hourly = period(1);
    nd_datetime_t midnight = datetime(2000, 1, 1, 0, 0, 0);
    nd_datetime_t one = datetime(2000, 1, 1, 1, 0, 0);
    nd_datetime_t one_oh_one = datetime(2000, 1, 1, 1, 1, 0);
    nd_datetime_t two = datetime(2000, 1, 1, 2, 0, 0);

    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&midnight, &hourly, 60));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&one, &hourly, 60));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&one_oh_one, &hourly, 60));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&two, &hourly, 60));
}

static void test_day_period_is_measured_in_calendar_days(void)
{
    nd_freeze_period_t daily = period(1);
    nd_datetime_t first = datetime(2000, 1, 1, 0, 0, 0);
    nd_datetime_t first_after = datetime(2000, 1, 1, 0, 1, 0);
    nd_datetime_t second = datetime(2000, 1, 2, 0, 0, 0);
    nd_datetime_t second_noon = datetime(2000, 1, 2, 12, 0, 0);

    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&first, &daily, 1440));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&first_after, &daily, 1440));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_period_due(&second, &daily, 1440));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_period_due(&second_noon, &daily, 1440));
}

static void test_month_freezes_at_the_first_midnight(void)
{
    nd_datetime_t first = datetime(2024, 6, 1, 0, 0, 0);
    nd_datetime_t off_second = datetime(2024, 6, 1, 0, 0, 1);
    nd_datetime_t second_day = datetime(2024, 6, 2, 0, 0, 0);
    nd_datetime_t first_noon = datetime(2024, 6, 1, 12, 0, 0);

    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_month_due(&first));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_month_due(&off_second));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_month_due(&second_day));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_month_due(&first_noon));
}

static void test_bill_fires_once_per_scheduled_hour(void)
{
    nd_datetime_t now = datetime(2024, 6, 15, 8, 0, 0);
    nd_datetime_t previous = datetime(2024, 5, 15, 8, 0, 0);
    nd_freeze_bill_date_t bill = {15, 8};

    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_bill_due(&now, &previous, &bill));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&now, &now, &bill));
}

static void test_bill_must_match_day_hour_and_minute_boundary(void)
{
    nd_datetime_t previous = datetime(2024, 5, 15, 8, 0, 0);
    nd_freeze_bill_date_t bill = {15, 8};
    nd_datetime_t wrong_hour = datetime(2024, 6, 15, 7, 0, 0);
    nd_datetime_t wrong_day = datetime(2024, 6, 14, 8, 0, 0);
    nd_datetime_t wrong_minute = datetime(2024, 6, 15, 8, 1, 0);
    nd_datetime_t wrong_second = datetime(2024, 6, 15, 8, 0, 1);

    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&wrong_hour, &previous, &bill));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&wrong_day, &previous, &bill));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&wrong_minute, &previous, &bill));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&wrong_second, &previous, &bill));
}

static void test_bill_rejects_disabled_and_out_of_range_dates(void)
{
    nd_datetime_t now = datetime(2024, 6, 15, 8, 0, 0);
    nd_datetime_t previous = datetime(2024, 5, 15, 8, 0, 0);
    nd_freeze_bill_date_t disabled = {0x63, 0x63};
    nd_freeze_bill_date_t disabled_ff = {0xFF, 0xFF};
    nd_freeze_bill_date_t zero_day = {0, 8};
    nd_freeze_bill_date_t late_hour = {15, 24};

    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&now, &previous, &disabled));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&now, &previous, &disabled_ff));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&now, &previous, &zero_day));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&now, &previous, &late_hour));
}

static void test_bill_uses_the_current_month(void)
{
    nd_datetime_t previous = datetime(2024, 6, 30, 8, 0, 0);
    nd_freeze_bill_date_t bill = {31, 8};
    nd_datetime_t first_of_july = datetime(2024, 7, 1, 8, 0, 0);
    nd_datetime_t last_of_july = datetime(2024, 7, 31, 8, 0, 0);

    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_bill_due(&first_of_july, &previous, &bill));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_bill_due(&last_of_july, &previous, &bill));
}

static void test_instant_counts_down_on_each_step(void)
{
    nd_freeze_instant_t instant = {0, 0};

    nd_freeze_instant_arm(&instant, 3);
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_instant_step(&instant));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_instant_step(&instant));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_instant_step(&instant));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_instant_step(&instant));
}

static void test_instant_zero_delay_fires_on_the_next_step(void)
{
    nd_freeze_instant_t instant = {0, 0};

    nd_freeze_instant_arm(&instant, 0);
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_instant_step(&instant));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_instant_step(&instant));
}

static void test_instant_idle_never_fires(void)
{
    nd_freeze_instant_t instant = {0, 0};

    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_instant_step(&instant));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_period_needs_a_valid_interval_and_unit);
    RUN_TEST(test_minute_period_fires_on_its_interval);
    RUN_TEST(test_period_keeps_the_epoch_phase_for_odd_intervals);
    RUN_TEST(test_hour_period_is_measured_in_wall_clock_hours);
    RUN_TEST(test_day_period_is_measured_in_calendar_days);
    RUN_TEST(test_month_freezes_at_the_first_midnight);
    RUN_TEST(test_bill_fires_once_per_scheduled_hour);
    RUN_TEST(test_bill_must_match_day_hour_and_minute_boundary);
    RUN_TEST(test_bill_rejects_disabled_and_out_of_range_dates);
    RUN_TEST(test_bill_uses_the_current_month);
    RUN_TEST(test_instant_counts_down_on_each_step);
    RUN_TEST(test_instant_zero_delay_fires_on_the_next_step);
    RUN_TEST(test_instant_idle_never_fires);
    return UNITY_END();
}
