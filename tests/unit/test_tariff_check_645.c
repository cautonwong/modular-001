#include "unity.h"

#include "domain/tariff/tariff.h"

#include "application/adapters/dl645/645id_table.h"
#include "application/adapters/dl645/fr_645_env.h"
#include "application/ports/resource_access.h"
#include "targets/stategrid_meter/app/port.h"

unsigned char SeasonTab_Format_Check(void *instance, unsigned char *Addr,
                                     unsigned char *Len);
unsigned char TouTab_Format_Check(void *instance, unsigned char *Addr,
                                  unsigned char *Len);

static unsigned char eeprom[2048];
static nd_resource_port_t port;

static int16_t fake_get_index(void *ctx, uint32_t index, uint8_t *buf,
                              uint16_t size)
{
    uint16_t offset = (uint16_t)(index & 0xFFFFu);
    (void)ctx;
    for (uint16_t i = 0; i < size; i++) {
        buf[i] = eeprom[(offset + i) % sizeof(eeprom)];
    }
    return 0;
}

const nd_resource_port_t *nd_fr645_resource_access(void *instance)
{
    (void)instance;
    return &port;
}

void setUp(void)
{
    for (unsigned int i = 0; i < sizeof(eeprom); i++) {
        eeprom[i] = 0;
    }
    eeprom[SeasonDay_Para + 1] = 2;
    eeprom[SeasonDay_Para + 3] = 2;
    port.get_index = fake_get_index;
}

void tearDown(void) {}

static void test_season_table_is_accepted_and_padded(void)
{
    unsigned char data[64] = { 0 };
    unsigned char len = 6;

    data[0] = 1;
    data[1] = 0x01;
    data[2] = 0x01;
    data[3] = 2;
    data[4] = 0x01;
    data[5] = 0x05;

    TEST_ASSERT_EQUAL_UINT8(0, SeasonTab_Format_Check(NULL, data, &len));
    TEST_ASSERT_EQUAL_UINT8(3 * ND_TARIFF_MAX_SEASONS, len);
    TEST_ASSERT_EQUAL_UINT8(1, data[0]);
    TEST_ASSERT_EQUAL_UINT8(2, data[3]);
    TEST_ASSERT_EQUAL_UINT8(2, data[6]);
    TEST_ASSERT_EQUAL_UINT8(0x01, data[7]);
    TEST_ASSERT_EQUAL_UINT8(0x05, data[8]);
    TEST_ASSERT_EQUAL_UINT8(2, data[39]);
    TEST_ASSERT_EQUAL_UINT8(0x05, data[41]);
}

static void test_season_table_above_the_configured_day_tables_is_rejected(void)
{
    unsigned char data[8] = { 3, 0x01, 0x01 };
    unsigned char len = 3;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            SeasonTab_Format_Check(NULL, data, &len));
}

static void test_season_table_rejects_an_invalid_date(void)
{
    unsigned char data[8] = { 1, 0x29, 0x02 };
    unsigned char len = 3;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            SeasonTab_Format_Check(NULL, data, &len));
}

static void test_season_table_rejects_an_empty_write(void)
{
    unsigned char data[8] = { 0 };
    unsigned char len = 0;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            SeasonTab_Format_Check(NULL, data, &len));
}

static void test_period_table_is_accepted_and_padded(void)
{
    unsigned char data[64] = { 0 };
    unsigned char len = 6;

    data[0] = 1;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 2;
    data[4] = 0x15;
    data[5] = 0x00;

    TEST_ASSERT_EQUAL_UINT8(0, TouTab_Format_Check(NULL, data, &len));
    TEST_ASSERT_EQUAL_UINT8(3 * ND_TARIFF_MAX_PERIODS, len);
    TEST_ASSERT_EQUAL_UINT8(2, data[6]);
    TEST_ASSERT_EQUAL_UINT8(0x15, data[7]);
    TEST_ASSERT_EQUAL_UINT8(2, data[39]);
    TEST_ASSERT_EQUAL_UINT8(0x15, data[40]);
    TEST_ASSERT_EQUAL_UINT8(0, data[41]);
}

static void test_period_table_above_the_configured_fees_is_rejected(void)
{
    unsigned char data[8] = { 3, 0x00, 0x00 };
    unsigned char len = 3;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_FEEOV,
                            TouTab_Format_Check(NULL, data, &len));
}

static void test_period_table_rejects_a_zero_fee(void)
{
    unsigned char data[8] = { 0, 0x00, 0x00 };
    unsigned char len = 3;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            TouTab_Format_Check(NULL, data, &len));
}

static void test_period_table_rejects_short_spacing(void)
{
    unsigned char data[16] = { 1, 0x00, 0x00, 1, 0x05, 0x00 };
    unsigned char len = 6;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            TouTab_Format_Check(NULL, data, &len));
}

static void test_period_table_rejects_a_short_wrap_gap(void)
{
    unsigned char data[16] = { 1, 0x50, 0x23, 2, 0x00, 0x00 };
    unsigned char len = 6;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            TouTab_Format_Check(NULL, data, &len));
}

static void test_period_table_accepts_a_fifteen_minute_wrap_gap(void)
{
    unsigned char data[64] = { 1, 0x45, 0x23, 2, 0x00, 0x00 };
    unsigned char len = 6;

    TEST_ASSERT_EQUAL_UINT8(0, TouTab_Format_Check(NULL, data, &len));
}

static void test_period_table_rejects_an_invalid_hour(void)
{
    unsigned char data[8] = { 1, 0x00, 0x24 };
    unsigned char len = 3;

    TEST_ASSERT_EQUAL_UINT8(RS_ERR_OTHERS,
                            TouTab_Format_Check(NULL, data, &len));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_season_table_is_accepted_and_padded);
    RUN_TEST(test_season_table_above_the_configured_day_tables_is_rejected);
    RUN_TEST(test_season_table_rejects_an_invalid_date);
    RUN_TEST(test_season_table_rejects_an_empty_write);
    RUN_TEST(test_period_table_is_accepted_and_padded);
    RUN_TEST(test_period_table_above_the_configured_fees_is_rejected);
    RUN_TEST(test_period_table_rejects_a_zero_fee);
    RUN_TEST(test_period_table_rejects_short_spacing);
    RUN_TEST(test_period_table_rejects_a_short_wrap_gap);
    RUN_TEST(test_period_table_accepts_a_fifteen_minute_wrap_gap);
    RUN_TEST(test_period_table_rejects_an_invalid_hour);
    return UNITY_END();
}
