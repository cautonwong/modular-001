#include "unity.h"

#include "domain/tariff/tariff.h"

static nd_tariff_table_t table;

void setUp(void)
{
    nd_tariff_table_init(&table);
}

void tearDown(void) {}

static void test_interval_lookup_in_increasing_table(void)
{
    nd_tariff_interval_t intervals[3] = {{0x0101, 1}, {0x0601, 2}, {0x0C01, 3}};

    TEST_ASSERT_EQUAL_UINT8(0, nd_tariff_find_interval(intervals, 3, 0x0301));
    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_find_interval(intervals, 3, 0x0701));
    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_find_interval(intervals, 3, 0x0D01));
    TEST_ASSERT_EQUAL_UINT8(0, nd_tariff_find_interval(intervals, 3, 0x0101));
}

static void test_interval_lookup_wraps_across_the_last_boundary(void)
{
    nd_tariff_interval_t intervals[2] = {{0x0301, 1}, {0x0C01, 2}};

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_find_interval(intervals, 2, 0x0201));
    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_find_interval(intervals, 2, 0x0C01));
    TEST_ASSERT_EQUAL_UINT8(0, nd_tariff_find_interval(intervals, 2, 0x0301));
}

static void test_interval_lookup_handles_single_and_empty_tables(void)
{
    nd_tariff_interval_t intervals[1] = {{0x0601, 1}};

    TEST_ASSERT_EQUAL_UINT8(0, nd_tariff_find_interval(intervals, 1, 0x0101));
    TEST_ASSERT_EQUAL_UINT8(0, nd_tariff_find_interval(intervals, 0, 0x0101));
}

static nd_datetime_t weekday_date(void)
{
    nd_datetime_t date = {0, 0, 12, 5, 6, 2024}; /* Wednesday, 2024-06-05 */
    return date;
}

static void test_day_table_follows_the_season_table(void)
{
    nd_datetime_t date = weekday_date();
    table.workday_mask = 0xFF;
    table.day_table_count = 2;
    table.season_count = 1;
    table.seasons[0].start = 0x0101;
    table.seasons[0].value = 2;

    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_select_day_table(&table, &date));
}

static void test_holiday_overrides_the_season_table(void)
{
    nd_datetime_t date = weekday_date();
    table.workday_mask = 0xFF;
    table.day_table_count = 2;
    table.holiday_count = 1;
    table.holidays[0].year = 2024;
    table.holidays[0].month = 6;
    table.holidays[0].day = 5;
    table.holidays[0].day_table = 1;

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_day_table(&table, &date));
}

static void test_rest_day_uses_the_configured_table(void)
{
    nd_datetime_t sunday = {0, 0, 12, 7, 1, 2001};
    table.workday_mask = 0x7F; /* Sunday is rest */
    table.rest_day_table = 2;
    table.day_table_count = 2;

    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_select_day_table(&table, &sunday));
}

static void test_invalid_day_table_numbers_fall_back_to_one(void)
{
    nd_datetime_t date = weekday_date();
    table.workday_mask = 0xFF;
    table.day_table_count = 2;
    table.holiday_count = 1;
    table.holidays[0].year = 2024;
    table.holidays[0].month = 6;
    table.holidays[0].day = 5;
    table.holidays[0].day_table = 9;

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_day_table(&table, &date));
}

static void test_fee_follows_the_day_periods(void)
{
    table.day_table_count = 2;
    table.fee_count = 2;
    table.period_count[0] = 2;
    table.periods[0][0].start = 0x0000;
    table.periods[0][0].value = 1;
    table.periods[0][1].start = 0x0800;
    table.periods[0][1].value = 2;

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_fee(&table, 1, 7, 59));
    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_select_fee(&table, 1, 8, 0));
    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_select_fee(&table, 1, 23, 59));
}

static void test_fee_falls_back_when_unconfigured_or_out_of_range(void)
{
    table.fee_count = 2;
    table.period_count[0] = 1;
    table.periods[0][0].start = 0x0000;
    table.periods[0][0].value = 9;

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_fee(&table, 1, 12, 0));
    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_fee(&table, 0, 12, 0));
    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_fee(&table, 9, 12, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_interval_lookup_in_increasing_table);
    RUN_TEST(test_interval_lookup_wraps_across_the_last_boundary);
    RUN_TEST(test_interval_lookup_handles_single_and_empty_tables);
    RUN_TEST(test_day_table_follows_the_season_table);
    RUN_TEST(test_holiday_overrides_the_season_table);
    RUN_TEST(test_rest_day_uses_the_configured_table);
    RUN_TEST(test_invalid_day_table_numbers_fall_back_to_one);
    RUN_TEST(test_fee_follows_the_day_periods);
    RUN_TEST(test_fee_falls_back_when_unconfigured_or_out_of_range);
    return UNITY_END();
}
