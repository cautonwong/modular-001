#include "unity.h"

#include "components/services/time_format/time_format.h"

void setUp(void) {}
void tearDown(void) {}

static void test_date_format_checks_day_and_month(void)
{
    uint8_t valid[] = {31, 12, 24};
    uint8_t bad_day[] = {32, 1, 24};
    uint8_t bad_month[] = {1, 13, 24};
    TEST_ASSERT_EQUAL_UINT8(0, Date_Format_Check(valid));
    TEST_ASSERT_EQUAL_UINT8(1, Date_Format_Check(bad_day));
    TEST_ASSERT_EQUAL_UINT8(1, Date_Format_Check(bad_month));

    uint8_t leap[] = {29, 2, 24};
    uint8_t common[] = {29, 2, 23};
    TEST_ASSERT_EQUAL_UINT8(0, Date_Format_Check(leap));
    TEST_ASSERT_EQUAL_UINT8(1, Date_Format_Check(common));
}

static void test_time_format_checks_ranges(void)
{
    uint8_t valid[] = {23, 59, 59};
    uint8_t bad_hour[] = {24, 0, 0};
    uint8_t bad_minute[] = {0, 60, 0};
    uint8_t bad_second[] = {0, 0, 60};
    TEST_ASSERT_EQUAL_UINT8(0, Time_Format_Check(valid));
    TEST_ASSERT_EQUAL_UINT8(1, Time_Format_Check(bad_hour));
    TEST_ASSERT_EQUAL_UINT8(1, Time_Format_Check(bad_minute));
    TEST_ASSERT_EQUAL_UINT8(1, Time_Format_Check(bad_second));
}

static void test_month_lengths(void)
{
    uint8_t feb_leap[] = {2, 24};
    uint8_t feb_common[] = {2, 23};
    uint8_t april[] = {4, 24};
    TEST_ASSERT_EQUAL_UINT8(29, GetMonMaxDay(feb_leap));
    TEST_ASSERT_EQUAL_UINT8(28, GetMonMaxDay(feb_common));
    TEST_ASSERT_EQUAL_UINT8(30, GetMonMaxDay(april));
    TEST_ASSERT_EQUAL_UINT8(31, MonTab[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_date_format_checks_day_and_month);
    RUN_TEST(test_time_format_checks_ranges);
    RUN_TEST(test_month_lengths);
    return UNITY_END();
}
