#include "unity.h"

#include <string.h>

#include "infra/memory/parameter_store.h"
#include "targets/stategrid_meter/app/tariff_lookup.h"

static uint8_t parameters[5];
static uint8_t weekend[1];
static uint8_t weekend_fee[1];
static uint8_t season0[3];
static uint8_t season1[3];
static uint8_t holiday0[6];
static uint8_t day0[6];
static uint8_t day1[6];
static nd_param_record_t records[8];
static nd_param_table_t store;

void setUp(void)
{
    parameters[0] = 2; /* seasons */
    parameters[1] = 2; /* day tables */
    parameters[2] = 2; /* periods per day table */
    parameters[3] = 2; /* fees */
    parameters[4] = 1; /* holidays */

    weekend[0] = 0xFF;
    weekend_fee[0] = 1;

    season0[0] = 1; season0[1] = 1; season0[2] = 1;
    season1[0] = 7; season1[1] = 1; season1[2] = 2;

    holiday0[0] = 0x07; holiday0[1] = 0xE8; /* 2024 */
    holiday0[2] = 6; holiday0[3] = 5; holiday0[4] = 0; holiday0[5] = 1;

    day0[0] = 0; day0[1] = 0; day0[2] = 0x01;
    day0[3] = 8; day0[4] = 0; day0[5] = 0x02;
    memcpy(day1, day0, sizeof(day0));

    records[0] = (nd_param_record_t){916, parameters, sizeof(parameters)};
    records[1] = (nd_param_record_t){922, weekend, sizeof(weekend)};
    records[2] = (nd_param_record_t){923, weekend_fee, sizeof(weekend_fee)};
    records[3] = (nd_param_record_t){924, season0, sizeof(season0)};
    records[4] = (nd_param_record_t){925, season1, sizeof(season1)};
    records[5] = (nd_param_record_t){1008, holiday0, sizeof(holiday0)};
    records[6] = (nd_param_record_t){952, day0, sizeof(day0)};
    records[7] = (nd_param_record_t){953, day1, sizeof(day1)};
    nd_param_table_init(&store, records, 8);
}

void tearDown(void) {}

static nd_datetime_t at(uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute)
{
    nd_datetime_t value = {0, minute, hour, day, month, year};
    return value;
}

static void test_day_table_follows_holiday_and_season(void)
{
    nd_datetime_t holiday = at(2024, 6, 5, 12, 0);
    nd_datetime_t before_july = at(2024, 3, 1, 12, 0);
    nd_datetime_t after_july = at(2024, 8, 1, 12, 0);

    TEST_ASSERT_EQUAL_INT(0, tariff_lookup_load(nd_param_table_port(&store)));
    TEST_ASSERT_EQUAL_UINT8(1, tariff_lookup_day_table(&holiday));
    TEST_ASSERT_EQUAL_UINT8(1, tariff_lookup_day_table(&before_july));
    TEST_ASSERT_EQUAL_UINT8(2, tariff_lookup_day_table(&after_july));
}

static void test_fee_tracks_the_active_period(void)
{
    nd_datetime_t before = at(2024, 3, 1, 7, 59);
    nd_datetime_t boundary = at(2024, 3, 1, 8, 0);
    nd_datetime_t inside = at(2024, 3, 1, 8, 1);
    nd_datetime_t midnight = at(2024, 3, 2, 0, 0);
    uint8_t changed = 0;

    TEST_ASSERT_EQUAL_INT(0, tariff_lookup_load(nd_param_table_port(&store)));

    TEST_ASSERT_EQUAL_UINT8(1, tariff_lookup_fee(1, &before, &changed));
    TEST_ASSERT_EQUAL_UINT8(0, changed);
    TEST_ASSERT_EQUAL_UINT8(2, tariff_lookup_fee(1, &boundary, &changed));
    TEST_ASSERT_EQUAL_UINT8(1, changed);
    TEST_ASSERT_EQUAL_UINT8(2, tariff_lookup_fee(1, &inside, &changed));
    TEST_ASSERT_EQUAL_UINT8(0, changed);
    TEST_ASSERT_EQUAL_UINT8(1, tariff_lookup_fee(1, &midnight, &changed));
    TEST_ASSERT_EQUAL_UINT8(1, changed);
}

static void test_fee_signals_a_period_starting_now(void)
{
    nd_datetime_t midnight = at(2024, 3, 1, 0, 0);
    uint8_t changed = 0;

    TEST_ASSERT_EQUAL_INT(0, tariff_lookup_load(nd_param_table_port(&store)));
    TEST_ASSERT_EQUAL_UINT8(1, tariff_lookup_fee(1, &midnight, &changed));
    TEST_ASSERT_EQUAL_UINT8(1, changed);
}

static void test_load_rejects_a_missing_parameter_block(void)
{
    nd_param_record_t empty[1];
    nd_param_table_t empty_store;

    nd_param_table_init(&empty_store, empty, 0);
    TEST_ASSERT_EQUAL_INT(-1, tariff_lookup_load(nd_param_table_port(&empty_store)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_day_table_follows_holiday_and_season);
    RUN_TEST(test_fee_tracks_the_active_period);
    RUN_TEST(test_fee_signals_a_period_starting_now);
    RUN_TEST(test_load_rejects_a_missing_parameter_block);
    return UNITY_END();
}
