#include "unity.h"

#include "domain/freeze/backfill.h"

void setUp(void) {}

void tearDown(void) {}

static nd_datetime_t datetime(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t minute, uint8_t second)
{
    nd_datetime_t value = {second, minute, hour, day, month, year};
    return value;
}

static void test_day_backfill_skips_the_power_down_day(void)
{
    nd_datetime_t now = datetime(2024, 6, 1, 0, 0, 0);
    nd_datetime_t power_down = datetime(2024, 6, 1, 12, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 20, 0, 0, 0);
    nd_datetime_t out[ND_FREEZE_DAY_CATCH_UP_MAX];

    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_day_backfill(&now, &power_down, &last, 1,
                                                      out, ND_FREEZE_DAY_CATCH_UP_MAX));
}

static void test_day_backfill_fills_each_missed_period(void)
{
    nd_datetime_t now = datetime(2024, 6, 4, 5, 0, 0);
    nd_datetime_t power_down = datetime(2024, 6, 1, 12, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 31, 0, 0, 0);
    nd_datetime_t out[ND_FREEZE_DAY_CATCH_UP_MAX];

    TEST_ASSERT_EQUAL_UINT8(3, nd_freeze_day_backfill(&now, &power_down, &last, 1,
                                                      out, ND_FREEZE_DAY_CATCH_UP_MAX));
    TEST_ASSERT_EQUAL_UINT8(2, out[0].day);
    TEST_ASSERT_EQUAL_UINT8(3, out[1].day);
    TEST_ASSERT_EQUAL_UINT8(4, out[2].day);
    TEST_ASSERT_EQUAL_UINT8(0, out[0].hour);
}

static void test_day_backfill_starts_after_the_last_record(void)
{
    nd_datetime_t now = datetime(2024, 6, 4, 0, 0, 0);
    nd_datetime_t power_down = datetime(2024, 6, 1, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 6, 2, 0, 0, 0);
    nd_datetime_t out[ND_FREEZE_DAY_CATCH_UP_MAX];

    TEST_ASSERT_EQUAL_UINT8(2, nd_freeze_day_backfill(&now, &power_down, &last, 1,
                                                      out, ND_FREEZE_DAY_CATCH_UP_MAX));
    TEST_ASSERT_EQUAL_UINT8(3, out[0].day);
    TEST_ASSERT_EQUAL_UINT8(4, out[1].day);
}

static void test_day_backfill_caps_a_long_outage_at_seven_periods(void)
{
    nd_datetime_t now = datetime(2024, 6, 11, 0, 0, 0);
    nd_datetime_t power_down = datetime(2024, 6, 1, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 1, 0, 0, 0);
    nd_datetime_t out[ND_FREEZE_DAY_CATCH_UP_MAX];

    TEST_ASSERT_EQUAL_UINT8(7, nd_freeze_day_backfill(&now, &power_down, &last, 1,
                                                      out, ND_FREEZE_DAY_CATCH_UP_MAX));
    TEST_ASSERT_EQUAL_UINT8(5, out[0].day);
    TEST_ASSERT_EQUAL_UINT8(11, out[6].day);
}

static void test_day_backfill_honours_the_period_and_the_output_limit(void)
{
    nd_datetime_t now = datetime(2024, 6, 10, 0, 0, 0);
    nd_datetime_t power_down = datetime(2024, 6, 1, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 1, 0, 0, 0);
    nd_datetime_t out[ND_FREEZE_DAY_CATCH_UP_MAX];

    TEST_ASSERT_EQUAL_UINT8(2, nd_freeze_day_backfill(&now, &power_down, &last, 3,
                                                      out, 2));
    TEST_ASSERT_EQUAL_UINT8(4, out[0].day);
    TEST_ASSERT_EQUAL_UINT8(7, out[1].day);
}

static void test_day_backfill_crosses_a_month_boundary(void)
{
    nd_datetime_t now = datetime(2024, 2, 2, 0, 0, 0);
    nd_datetime_t power_down = datetime(2024, 1, 31, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 1, 1, 0, 0, 0);
    nd_datetime_t out[ND_FREEZE_DAY_CATCH_UP_MAX];

    TEST_ASSERT_EQUAL_UINT8(2, nd_freeze_day_backfill(&now, &power_down, &last, 1,
                                                      out, ND_FREEZE_DAY_CATCH_UP_MAX));
    TEST_ASSERT_EQUAL_UINT8(1, out[0].day);
    TEST_ASSERT_EQUAL_UINT8(2, out[0].month);
    TEST_ASSERT_EQUAL_UINT8(2, out[1].day);
}

static void test_bill_backfill_replays_settlements_since_power_down(void)
{
    nd_datetime_t now = datetime(2024, 6, 20, 12, 0, 0);
    nd_datetime_t power_down = datetime(2024, 5, 10, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 4, 15, 8, 0, 0);
    nd_freeze_bill_date_t dates[ND_FREEZE_BILL_DATES] = {{1, 8}, {15, 8}, {28, 8}};
    nd_freeze_bill_event_t out[ND_FREEZE_BILL_DATES * 12];

    TEST_ASSERT_EQUAL_UINT8(4, nd_freeze_bill_backfill(&now, &power_down, &last, dates,
                                                       out, sizeof(out) / sizeof(out[0])));
    TEST_ASSERT_EQUAL_UINT8(5, out[0].when.month);
    TEST_ASSERT_EQUAL_UINT8(15, out[0].when.day);
    TEST_ASSERT_EQUAL_UINT8(1, out[0].slot);
    TEST_ASSERT_EQUAL_UINT8(5, out[1].when.month);
    TEST_ASSERT_EQUAL_UINT8(28, out[1].when.day);
    TEST_ASSERT_EQUAL_UINT8(2, out[1].slot);
    TEST_ASSERT_EQUAL_UINT8(6, out[2].when.month);
    TEST_ASSERT_EQUAL_UINT8(1, out[2].when.day);
    TEST_ASSERT_EQUAL_UINT8(6, out[3].when.month);
    TEST_ASSERT_EQUAL_UINT8(15, out[3].when.day);
}

static void test_bill_backfill_does_not_repeat_the_last_record(void)
{
    nd_datetime_t now = datetime(2024, 6, 20, 12, 0, 0);
    nd_datetime_t power_down = datetime(2024, 5, 10, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 28, 8, 0, 0);
    nd_freeze_bill_date_t dates[ND_FREEZE_BILL_DATES] = {{15, 8}, {1, 8}, {28, 8}};
    nd_freeze_bill_event_t out[ND_FREEZE_BILL_DATES * 12];

    TEST_ASSERT_EQUAL_UINT8(2, nd_freeze_bill_backfill(&now, &power_down, &last, dates,
                                                       out, sizeof(out) / sizeof(out[0])));
    TEST_ASSERT_EQUAL_UINT8(6, out[0].when.month);
    TEST_ASSERT_EQUAL_UINT8(1, out[0].when.day);
    TEST_ASSERT_EQUAL_UINT8(6, out[1].when.month);
    TEST_ASSERT_EQUAL_UINT8(15, out[1].when.day);
}

static void test_bill_backfill_skips_unused_slots(void)
{
    nd_datetime_t now = datetime(2024, 6, 20, 12, 0, 0);
    nd_datetime_t power_down = datetime(2024, 5, 10, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 1, 0, 0, 0);
    nd_freeze_bill_date_t dates[ND_FREEZE_BILL_DATES] = {{0x63, 0x63}, {15, 8}, {0xFF, 0xFF}};
    nd_freeze_bill_event_t out[ND_FREEZE_BILL_DATES * 12];

    TEST_ASSERT_EQUAL_UINT8(2, nd_freeze_bill_backfill(&now, &power_down, &last, dates,
                                                       out, sizeof(out) / sizeof(out[0])));
    TEST_ASSERT_EQUAL_UINT8(5, out[0].when.month);
    TEST_ASSERT_EQUAL_UINT8(6, out[1].when.month);
}

static void test_bill_backfill_stops_at_the_first_future_date(void)
{
    nd_datetime_t now = datetime(2024, 6, 10, 12, 0, 0);
    nd_datetime_t power_down = datetime(2024, 5, 10, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 1, 0, 0, 0);
    nd_freeze_bill_date_t dates[ND_FREEZE_BILL_DATES] = {{1, 8}, {15, 8}, {28, 8}};
    nd_freeze_bill_event_t out[ND_FREEZE_BILL_DATES * 12];

    TEST_ASSERT_EQUAL_UINT8(3, nd_freeze_bill_backfill(&now, &power_down, &last, dates,
                                                       out, sizeof(out) / sizeof(out[0])));
    TEST_ASSERT_EQUAL_UINT8(5, out[0].when.month);
    TEST_ASSERT_EQUAL_UINT8(15, out[0].when.day);
    TEST_ASSERT_EQUAL_UINT8(5, out[1].when.month);
    TEST_ASSERT_EQUAL_UINT8(28, out[1].when.day);
    TEST_ASSERT_EQUAL_UINT8(6, out[2].when.month);
    TEST_ASSERT_EQUAL_UINT8(1, out[2].when.day);
}

static void test_bill_backfill_stops_after_twelve_months(void)
{
    nd_datetime_t now = datetime(2024, 6, 20, 12, 0, 0);
    nd_datetime_t power_down = datetime(2020, 1, 1, 0, 0, 0);
    nd_datetime_t last = datetime(2019, 1, 1, 0, 0, 0);
    nd_freeze_bill_date_t dates[ND_FREEZE_BILL_DATES] = {{1, 8}, {0x63, 0x63}, {0xFF, 0xFF}};
    nd_freeze_bill_event_t out[ND_FREEZE_BILL_DATES * 12];

    TEST_ASSERT_EQUAL_UINT8(12, nd_freeze_bill_backfill(&now, &power_down, &last, dates,
                                                        out, sizeof(out) / sizeof(out[0])));
    TEST_ASSERT_EQUAL_UINT8(2020, out[0].when.year);
    TEST_ASSERT_EQUAL_UINT8(1, out[0].when.month);
    TEST_ASSERT_EQUAL_UINT8(12, out[11].when.month);
}

static void test_bill_backfill_honours_the_output_limit(void)
{
    nd_datetime_t now = datetime(2024, 6, 20, 12, 0, 0);
    nd_datetime_t power_down = datetime(2024, 5, 10, 0, 0, 0);
    nd_datetime_t last = datetime(2024, 5, 1, 0, 0, 0);
    nd_freeze_bill_date_t dates[ND_FREEZE_BILL_DATES] = {{15, 8}, {1, 8}, {28, 8}};
    nd_freeze_bill_event_t out[2];

    TEST_ASSERT_EQUAL_UINT8(2, nd_freeze_bill_backfill(&now, &power_down, &last, dates,
                                                       out, 2));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_day_backfill_skips_the_power_down_day);
    RUN_TEST(test_day_backfill_fills_each_missed_period);
    RUN_TEST(test_day_backfill_starts_after_the_last_record);
    RUN_TEST(test_day_backfill_caps_a_long_outage_at_seven_periods);
    RUN_TEST(test_day_backfill_honours_the_period_and_the_output_limit);
    RUN_TEST(test_day_backfill_crosses_a_month_boundary);
    RUN_TEST(test_bill_backfill_replays_settlements_since_power_down);
    RUN_TEST(test_bill_backfill_does_not_repeat_the_last_record);
    RUN_TEST(test_bill_backfill_skips_unused_slots);
    RUN_TEST(test_bill_backfill_stops_at_the_first_future_date);
    RUN_TEST(test_bill_backfill_stops_after_twelve_months);
    RUN_TEST(test_bill_backfill_honours_the_output_limit);
    return UNITY_END();
}
