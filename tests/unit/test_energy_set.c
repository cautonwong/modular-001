#include "unity.h"

#include "domain/metering/energy_set.h"

static nd_energy_set_t set;

void setUp(void)
{
    nd_energy_set_init(&set, 2000);
}

void tearDown(void) {}

static void test_active_import_total_and_tariff(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1, 4000));
    TEST_ASSERT_EQUAL_INT64(200, nd_energy_set_read(
                                     &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1,
                                     ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_INT64(200, nd_energy_set_read(
                                     &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 0,
                                     ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read(
                                       &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1,
                                       ND_ENERGY_RES_0_0001_KWH));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read_raw(
                                       &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                       1));
    TEST_ASSERT_EQUAL_INT64(0, nd_energy_set_read(
                                    &set, ND_ENERGY_CHANNEL_ACTIVE_EXPORT, 1,
                                    ND_ENERGY_RES_0_01_KWH));
}

static void test_reactive_quadrants_are_stored_separately(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &set, ND_ENERGY_CHANNEL_REACTIVE_Q1, 1, 4000));
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &set, ND_ENERGY_CHANNEL_REACTIVE_Q2, 1, 2000));
    TEST_ASSERT_EQUAL_INT64(200, nd_energy_set_read(
                                     &set, ND_ENERGY_CHANNEL_REACTIVE_Q1, 1,
                                     ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_INT64(100, nd_energy_set_read(
                                     &set, ND_ENERGY_CHANNEL_REACTIVE_Q2, 1,
                                     ND_ENERGY_RES_0_01_KWH));
}

static void test_apparent_import(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &set, ND_ENERGY_CHANNEL_APPARENT_IMPORT, 1, 1000));
    TEST_ASSERT_EQUAL_INT64(50, nd_energy_set_read(
                                    &set, ND_ENERGY_CHANNEL_APPARENT_IMPORT, 1,
                                    ND_ENERGY_RES_0_01_KWH));
}

static void test_invalid_kind_and_null(void)
{
    TEST_ASSERT_EQUAL_INT(-1, nd_energy_set_add_pulses(
                                  NULL, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_energy_set_add_pulses(
                                  &set, (nd_energy_channel_id_t)99, 1, 1));
    TEST_ASSERT_EQUAL_INT64(0, nd_energy_set_read(
                                   &set, (nd_energy_channel_id_t)99, 1,
                                   ND_ENERGY_RES_0_01_KWH));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_active_import_total_and_tariff);
    RUN_TEST(test_reactive_quadrants_are_stored_separately);
    RUN_TEST(test_apparent_import);
    RUN_TEST(test_invalid_kind_and_null);
    return UNITY_END();
}
