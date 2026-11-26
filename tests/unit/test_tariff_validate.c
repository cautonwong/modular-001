#include "unity.h"

#include "domain/tariff/tariff.h"

void setUp(void) {}
void tearDown(void) {}

static nd_tariff_interval_t season(uint8_t month, uint8_t day, uint8_t table)
{
    nd_tariff_interval_t entry = { .start = (uint16_t)((month << 8) | day),
                                   .value = table };
    return entry;
}

static nd_tariff_interval_t period(uint8_t hour, uint8_t minute, uint8_t fee)
{
    nd_tariff_interval_t entry = { .start = (uint16_t)((hour << 8) | minute),
                                   .value = fee };
    return entry;
}

static void test_seasons_accept_a_year_table(void)
{
    nd_tariff_interval_t seasons[] = { season(1, 1, 1), season(5, 1, 2) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_VALID,
                          nd_tariff_validate_seasons(seasons, 2, 2));
}

static void test_seasons_reject_an_empty_table(void)
{
    nd_tariff_interval_t seasons[] = { season(1, 1, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_EMPTY,
                          nd_tariff_validate_seasons(seasons, 0, 1));
}

static void test_seasons_reject_too_many_entries(void)
{
    nd_tariff_interval_t seasons[ND_TARIFF_MAX_SEASONS + 1];

    for (uint8_t index = 0; index < ND_TARIFF_MAX_SEASONS + 1; index++) {
        seasons[index] = season((uint8_t)(index + 1), 1, 1);
    }
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_COUNT,
                          nd_tariff_validate_seasons(seasons,
                                                     ND_TARIFF_MAX_SEASONS + 1,
                                                     1));
}

static void test_seasons_reject_bad_day_tables(void)
{
    nd_tariff_interval_t seasons[] = { season(1, 1, 0), season(5, 1, 2) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_validate_seasons(seasons, 2, 2));
    seasons[0].value = 3;
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_validate_seasons(seasons, 2, 2));
    seasons[0].value = 1;
    seasons[1].value = ND_TARIFF_MAX_DAY_TABLES + 1;
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_validate_seasons(seasons, 2,
                                                     ND_TARIFF_MAX_DAY_TABLES));
}

static void test_seasons_reject_bad_dates(void)
{
    nd_tariff_interval_t seasons[] = { season(0, 1, 1), season(5, 1, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_DATE,
                          nd_tariff_validate_seasons(seasons, 2, 1));
    seasons[0] = season(13, 1, 1);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_DATE,
                          nd_tariff_validate_seasons(seasons, 2, 1));
    seasons[0] = season(2, 29, 1);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_DATE,
                          nd_tariff_validate_seasons(seasons, 2, 1));
    seasons[0] = season(1, 0, 1);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_DATE,
                          nd_tariff_validate_seasons(seasons, 2, 1));
}

static void test_seasons_allow_one_wrap_and_reject_two(void)
{
    nd_tariff_interval_t one_wrap[] = { season(10, 1, 1), season(2, 1, 1) };
    nd_tariff_interval_t two_wraps[] = { season(10, 1, 1), season(2, 1, 1),
                                         season(11, 1, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_VALID,
                          nd_tariff_validate_seasons(one_wrap, 2, 1));
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_ORDER,
                          nd_tariff_validate_seasons(two_wraps, 3, 1));
}

static void test_seasons_reject_equal_starts_with_different_tables(void)
{
    nd_tariff_interval_t equal_different[] = { season(1, 1, 1),
                                               season(1, 1, 2) };
    nd_tariff_interval_t equal_same[] = { season(1, 1, 1), season(1, 1, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_ORDER,
                          nd_tariff_validate_seasons(equal_different, 2, 2));
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_VALID,
                          nd_tariff_validate_seasons(equal_same, 2, 1));
}

static void test_periods_accept_fifteen_minute_gaps(void)
{
    nd_tariff_interval_t periods[] = { period(0, 0, 1), period(0, 15, 2) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_VALID,
                          nd_tariff_validate_periods(periods, 2, 2));
}

static void test_periods_reject_a_fourteen_minute_gap(void)
{
    nd_tariff_interval_t periods[] = { period(0, 0, 1), period(0, 14, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_SPACING,
                          nd_tariff_validate_periods(periods, 2, 1));
}

static void test_periods_reject_bad_fees_and_times(void)
{
    nd_tariff_interval_t periods[] = { period(0, 0, 0), period(0, 15, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_validate_periods(periods, 2, 1));
    periods[0].value = 2;
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_validate_periods(periods, 2, 1));
    periods[0] = period(24, 0, 1);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_TIME,
                          nd_tariff_validate_periods(periods, 2, 1));
    periods[0] = period(0, 60, 1);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_TIME,
                          nd_tariff_validate_periods(periods, 2, 1));
}

static void test_periods_check_the_wrap_gap(void)
{
    nd_tariff_interval_t tight[] = { period(23, 50, 1), period(0, 0, 1) };
    nd_tariff_interval_t spaced[] = { period(23, 45, 1), period(0, 0, 1) };

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_SPACING,
                          nd_tariff_validate_periods(tight, 2, 1));
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_VALID,
                          nd_tariff_validate_periods(spaced, 2, 1));
}

static void test_table_validate_accepts_a_consistent_table(void)
{
    nd_tariff_table_t table;

    nd_tariff_table_init(&table);
    table.day_table_count = 2;
    table.fee_count = 2;
    table.season_count = 2;
    table.seasons[0] = season(1, 1, 1);
    table.seasons[1] = season(5, 1, 2);
    table.period_count[0] = 1;
    table.period_count[1] = 2;
    table.periods[0][0] = period(0, 0, 1);
    table.periods[1][0] = period(0, 0, 1);
    table.periods[1][1] = period(0, 15, 2);
    table.rest_day_table = 1;
    table.holiday_count = 1;
    table.holidays[0].year = 2021;
    table.holidays[0].month = 5;
    table.holidays[0].day = 1;
    table.holidays[0].day_table = 2;

    TEST_ASSERT_EQUAL_INT(ND_TARIFF_VALID, nd_tariff_table_validate(&table));
}

static void test_table_validate_rejects_inconsistent_parts(void)
{
    nd_tariff_table_t table;

    nd_tariff_table_init(&table);
    table.day_table_count = 1;
    table.fee_count = 1;
    table.period_count[0] = 1;
    table.periods[0][0] = period(0, 0, 2);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_table_validate(&table));

    nd_tariff_table_init(&table);
    table.day_table_count = 1;
    table.season_count = 1;
    table.seasons[0] = season(1, 1, 2);
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_VALUE,
                          nd_tariff_table_validate(&table));

    nd_tariff_table_init(&table);
    table.day_table_count = 1;
    table.period_count[0] = ND_TARIFF_MAX_PERIODS + 1;
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_COUNT,
                          nd_tariff_table_validate(&table));

    nd_tariff_table_init(&table);
    table.holiday_count = 1;
    table.holidays[0].year = 2021;
    table.holidays[0].month = 2;
    table.holidays[0].day = 30;
    table.holidays[0].day_table = 1;
    TEST_ASSERT_EQUAL_INT(ND_TARIFF_ERR_DATE,
                          nd_tariff_table_validate(&table));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_seasons_accept_a_year_table);
    RUN_TEST(test_seasons_reject_an_empty_table);
    RUN_TEST(test_seasons_reject_too_many_entries);
    RUN_TEST(test_seasons_reject_bad_day_tables);
    RUN_TEST(test_seasons_reject_bad_dates);
    RUN_TEST(test_seasons_allow_one_wrap_and_reject_two);
    RUN_TEST(test_seasons_reject_equal_starts_with_different_tables);
    RUN_TEST(test_periods_accept_fifteen_minute_gaps);
    RUN_TEST(test_periods_reject_a_fourteen_minute_gap);
    RUN_TEST(test_periods_reject_bad_fees_and_times);
    RUN_TEST(test_periods_check_the_wrap_gap);
    RUN_TEST(test_table_validate_accepts_a_consistent_table);
    RUN_TEST(test_table_validate_rejects_inconsistent_parts);
    return UNITY_END();
}
