#include "unity.h"

#include <string.h>

#include "infra/memory/parameter_store.h"
#include "targets/stategrid_meter/persistence/legacy_tariff_parser.h"

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

static nd_datetime_t date(uint16_t year, uint8_t month, uint8_t day)
{
    nd_datetime_t value = {0, 0, 12, day, month, year};
    return value;
}

static void test_load_reads_the_parameter_block(void)
{
    nd_tariff_table_t table;

    TEST_ASSERT_EQUAL_INT(0, legacy_tariff_load(nd_param_table_port(&store), &table));

    TEST_ASSERT_EQUAL_UINT8(2, table.season_count);
    TEST_ASSERT_EQUAL_UINT8(2, table.day_table_count);
    TEST_ASSERT_EQUAL_UINT8(2, table.fee_count);
    TEST_ASSERT_EQUAL_UINT8(1, table.holiday_count);
    TEST_ASSERT_EQUAL_UINT8(2, table.period_count[0]);
    TEST_ASSERT_EQUAL_UINT8(2, table.periods[0][1].value);
}

static void test_holiday_and_season_selection_after_load(void)
{
    nd_tariff_table_t table;
    nd_datetime_t holiday = date(2024, 6, 5);
    nd_datetime_t before_july = date(2024, 3, 1);
    nd_datetime_t after_july = date(2024, 8, 1);

    TEST_ASSERT_EQUAL_INT(0, legacy_tariff_load(nd_param_table_port(&store), &table));

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_day_table(&table, &holiday));
    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_day_table(&table, &before_july));
    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_select_day_table(&table, &after_july));
}

static void test_fee_selection_after_load(void)
{
    nd_tariff_table_t table;

    TEST_ASSERT_EQUAL_INT(0, legacy_tariff_load(nd_param_table_port(&store), &table));

    TEST_ASSERT_EQUAL_UINT8(1, nd_tariff_select_fee(&table, 1, 7, 59));
    TEST_ASSERT_EQUAL_UINT8(2, nd_tariff_select_fee(&table, 1, 8, 0));
}

static void test_missing_parameter_block_fails(void)
{
    nd_param_record_t empty[1];
    nd_param_table_t empty_store;
    nd_tariff_table_t table;

    nd_param_table_init(&empty_store, empty, 0);

    TEST_ASSERT_EQUAL_INT(-1, legacy_tariff_load(nd_param_table_port(&empty_store), &table));
}

static void test_truncated_season_table_stops_at_last_valid_entry(void)
{
    nd_tariff_table_t table;
    records[4].length = 0; /* second season unreadable */

    TEST_ASSERT_EQUAL_INT(0, legacy_tariff_load(nd_param_table_port(&store), &table));
    TEST_ASSERT_EQUAL_UINT8(1, table.season_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_load_reads_the_parameter_block);
    RUN_TEST(test_holiday_and_season_selection_after_load);
    RUN_TEST(test_fee_selection_after_load);
    RUN_TEST(test_missing_parameter_block_fails);
    RUN_TEST(test_truncated_season_table_stops_at_last_valid_entry);
    return UNITY_END();
}
