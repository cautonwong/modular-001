#include "unity.h"

#include "domain/common/calendar.h"

void setUp(void) {}
void tearDown(void) {}

static void test_bcd_encodes_decimal_digits(void)
{
    TEST_ASSERT_EQUAL_UINT8(0x00, nd_calendar_bcd(0));
    TEST_ASSERT_EQUAL_UINT8(0x09, nd_calendar_bcd(9));
    TEST_ASSERT_EQUAL_UINT8(0x10, nd_calendar_bcd(10));
    TEST_ASSERT_EQUAL_UINT8(0x59, nd_calendar_bcd(59));
    TEST_ASSERT_EQUAL_UINT8(0x99, nd_calendar_bcd(99));
    TEST_ASSERT_EQUAL_UINT8(0x00, nd_calendar_bcd(100));
}

static void test_days_in_month_honours_leap_years(void)
{
    TEST_ASSERT_EQUAL_UINT8(31, nd_calendar_days_in_month(1, 2024));
    TEST_ASSERT_EQUAL_UINT8(29, nd_calendar_days_in_month(2, 2024));
    TEST_ASSERT_EQUAL_UINT8(28, nd_calendar_days_in_month(2, 2023));
    TEST_ASSERT_EQUAL_UINT8(30, nd_calendar_days_in_month(4, 2024));
    TEST_ASSERT_EQUAL_UINT8(0, nd_calendar_days_in_month(13, 2024));
}

static void test_date_validation(void)
{
    nd_datetime_t date = {0, 0, 0, 31, 1, 2024};
    TEST_ASSERT_TRUE(nd_calendar_date_valid(&date));

    date.day = 30;
    date.month = 2;
    TEST_ASSERT_FALSE(nd_calendar_date_valid(&date));

    date.day = 31;
    date.month = 4;
    TEST_ASSERT_FALSE(nd_calendar_date_valid(&date));

    date.day = 0;
    date.month = 1;
    TEST_ASSERT_FALSE(nd_calendar_date_valid(&date));

    date.day = 1;
    date.month = 13;
    TEST_ASSERT_FALSE(nd_calendar_date_valid(&date));
}

static void test_time_validation(void)
{
    nd_datetime_t time = {59, 59, 23, 1, 1, 2024};
    TEST_ASSERT_TRUE(nd_calendar_time_valid(&time));

    time.second = 60;
    TEST_ASSERT_FALSE(nd_calendar_time_valid(&time));
}

static void test_total_day_matches_legacy_formula(void)
{
    TEST_ASSERT_EQUAL_UINT32(1, nd_calendar_total_day(1, 1, 2000));
    TEST_ASSERT_EQUAL_UINT32(61, nd_calendar_total_day(1, 3, 2000));
    TEST_ASSERT_EQUAL_UINT32(367, nd_calendar_total_day(1, 1, 2001));
    TEST_ASSERT_EQUAL_UINT32(426, nd_calendar_total_day(1, 3, 2001));
    TEST_ASSERT_EQUAL_UINT32(1522, nd_calendar_total_day(1, 3, 2004));
    TEST_ASSERT_EQUAL_UINT32(8893, nd_calendar_total_day(6, 5, 2024));
    /* Legacy two-digit year formula repeats every 100 years. */
    TEST_ASSERT_EQUAL_UINT32(1, nd_calendar_total_day(1, 1, 2100));
}

static void test_total_minutes_combines_day_hour_and_minute(void)
{
    nd_datetime_t datetime = {0, 30, 2, 1, 1, 2001};

    uint32_t expected = nd_calendar_total_day(1, 1, 2001) * 24 * 60 + 2 * 60 + 30;
    TEST_ASSERT_EQUAL_UINT32(expected, nd_calendar_total_minutes(&datetime));
}

static void test_rtc_hex_is_ssmmhhddmmyyyy(void)
{
    nd_datetime_t datetime = {9, 8, 7, 6, 5, 2024};
    uint8_t out[ND_RTC_HEX_LEN];

    nd_calendar_to_rtc_hex(&datetime, out);

    TEST_ASSERT_EQUAL_UINT8(9, out[0]);
    TEST_ASSERT_EQUAL_UINT8(8, out[1]);
    TEST_ASSERT_EQUAL_UINT8(7, out[2]);
    TEST_ASSERT_EQUAL_UINT8(6, out[3]);
    TEST_ASSERT_EQUAL_UINT8(5, out[4]);
    TEST_ASSERT_EQUAL_UINT8(0xE8, out[5]);
    TEST_ASSERT_EQUAL_UINT8(0x07, out[6]);
}

static void test_rtc_bcd_uses_two_digit_year(void)
{
    nd_datetime_t datetime = {9, 8, 7, 6, 5, 2024};
    uint8_t out[ND_RTC_BCD_LEN];

    nd_calendar_to_rtc_bcd(&datetime, out);

    TEST_ASSERT_EQUAL_UINT8(0x09, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0x08, out[1]);
    TEST_ASSERT_EQUAL_UINT8(0x07, out[2]);
    TEST_ASSERT_EQUAL_UINT8(0x06, out[3]);
    TEST_ASSERT_EQUAL_UINT8(0x05, out[4]);
    TEST_ASSERT_EQUAL_UINT8(0x24, out[5]);
}

static void test_from_total_day_round_trips(void)
{
    static const uint32_t days[] = {1, 60, 61, 366, 367, 1521, 1522, 8893, 8949};
    nd_datetime_t datetime;

    for (uint8_t i = 0; i < sizeof(days) / sizeof(days[0]); i++) {
        nd_calendar_from_total_day(days[i], &datetime);
        TEST_ASSERT_EQUAL_UINT32(days[i], nd_calendar_total_day(datetime.day, datetime.month,
                                                                datetime.year));
    }

    nd_calendar_from_total_day(8893, &datetime); /* 2024-05-06 */
    TEST_ASSERT_EQUAL_UINT8(6, datetime.day);
    TEST_ASSERT_EQUAL_UINT8(5, datetime.month);
    TEST_ASSERT_EQUAL_UINT16(2024, datetime.year);

    nd_calendar_from_total_day(60, &datetime); /* 2000-02-29 */
    TEST_ASSERT_EQUAL_UINT8(29, datetime.day);
    TEST_ASSERT_EQUAL_UINT8(2, datetime.month);
}

static void test_weekday_matches_the_legacy_formula(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_calendar_weekday(1, 1, 2001)); /* Monday */
    TEST_ASSERT_EQUAL_UINT8(0, nd_calendar_weekday(7, 1, 2001)); /* Sunday */
    TEST_ASSERT_EQUAL_UINT8(6, nd_calendar_weekday(1, 1, 2000)); /* Saturday */
}


static void test_add_seconds_handles_day_boundary(void)
{
    nd_datetime_t datetime = {0, 15, 10, 11, 11, 2026};
    nd_datetime_t out;
    TEST_ASSERT_EQUAL_INT(0, nd_calendar_add_seconds(&datetime, -900, &out));
    TEST_ASSERT_EQUAL_UINT8(10, out.hour);
    TEST_ASSERT_EQUAL_UINT8(0, out.minute);
    TEST_ASSERT_EQUAL_INT(0, nd_calendar_add_seconds(&datetime, 3600, &out));
    TEST_ASSERT_EQUAL_UINT8(11, out.hour);
    TEST_ASSERT_EQUAL_UINT8(15, out.minute);
    TEST_ASSERT_EQUAL_INT(0, nd_calendar_add_seconds(&datetime, 86400, &out));
    TEST_ASSERT_EQUAL_UINT8(12, out.day);
    TEST_ASSERT_EQUAL_UINT8(10, out.hour);
}

static void test_from_elapsed_keeps_time_legal_across_days(void)
{
    nd_datetime_t out;
    uint32_t base = nd_calendar_total_day(1, 1, 2026);
    nd_calendar_from_elapsed(base, 0, &out);
    TEST_ASSERT_EQUAL_UINT16(2026, out.year);
    TEST_ASSERT_EQUAL_UINT8(1, out.month);
    TEST_ASSERT_EQUAL_UINT8(1, out.day);
    TEST_ASSERT_EQUAL_UINT8(0, out.hour);
    nd_calendar_from_elapsed(base, 3661, &out);
    TEST_ASSERT_EQUAL_UINT8(1, out.hour);
    TEST_ASSERT_EQUAL_UINT8(1, out.minute);
    TEST_ASSERT_EQUAL_UINT8(1, out.second);
    nd_calendar_from_elapsed(base, 86400u + 5u, &out);
    TEST_ASSERT_EQUAL_UINT8(2, out.day);
    TEST_ASSERT_EQUAL_UINT8(0, out.hour);
    TEST_ASSERT_EQUAL_UINT8(5, out.second);
    nd_calendar_from_elapsed(base, 365u * 86400u + 3600u, &out);
    TEST_ASSERT_EQUAL_UINT16(2027, out.year);
    TEST_ASSERT_EQUAL_UINT8(1, out.month);
    TEST_ASSERT_EQUAL_UINT8(1, out.day);
    TEST_ASSERT_EQUAL_UINT8(1, out.hour);
    nd_calendar_from_elapsed(base, 4294967295u, &out);
    TEST_ASSERT_TRUE(nd_calendar_date_valid(&out));
    TEST_ASSERT_TRUE(nd_calendar_time_valid(&out));
}

static void test_every_day_round_trips_for_a_century(void)
{
    uint32_t previous = 0;

    for (uint16_t year = 2000; year <= 2099; year++) {
        for (uint8_t month = 1; month <= 12; month++) {
            uint8_t days = nd_calendar_days_in_month(month, year);
            for (uint8_t day = 1; day <= days; day++) {
                nd_datetime_t out;
                uint32_t total = nd_calendar_total_day(day, month, year);

                TEST_ASSERT_EQUAL_UINT32(previous + 1, total);
                previous = total;
                nd_calendar_from_total_day(total, &out);
                TEST_ASSERT_EQUAL_UINT16(year, out.year);
                TEST_ASSERT_EQUAL_UINT8(month, out.month);
                TEST_ASSERT_EQUAL_UINT8(day, out.day);
            }
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bcd_encodes_decimal_digits);
    RUN_TEST(test_days_in_month_honours_leap_years);
    RUN_TEST(test_date_validation);
    RUN_TEST(test_time_validation);
    RUN_TEST(test_total_day_matches_legacy_formula);
    RUN_TEST(test_total_minutes_combines_day_hour_and_minute);
    RUN_TEST(test_from_total_day_round_trips);
    RUN_TEST(test_rtc_hex_is_ssmmhhddmmyyyy);
    RUN_TEST(test_rtc_bcd_uses_two_digit_year);
    RUN_TEST(test_weekday_matches_the_legacy_formula);
    RUN_TEST(test_add_seconds_handles_day_boundary);
    RUN_TEST(test_from_elapsed_keeps_time_legal_across_days);
    RUN_TEST(test_every_day_round_trips_for_a_century);
    return UNITY_END();
}
