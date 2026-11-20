/* Purpose: a corrupt or saturated calibration must not become pulses. The
 * power-to-pulse conversion rejects samples above the meter's top load and
 * caps the accepted rate at the configured meter constant. */
#include "unity.h"

#include <stdint.h>

#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"

void setUp(void) {}

void tearDown(void) {}

static void test_absurd_power_consumes_nothing(void)
{
    uint64_t fraction = 0;

    TEST_ASSERT_EQUAL_UINT32(0, nd_idis_energy_pulses(&fraction, INT32_MAX,
                                                      2000u));
    TEST_ASSERT_EQUAL_UINT64(0, fraction);

    fraction = 999999u;
    TEST_ASSERT_EQUAL_UINT32(0, nd_idis_energy_pulses(&fraction, INT32_MIN,
                                                      2000u));
    TEST_ASSERT_EQUAL_UINT64(999999u, fraction);
}

static void test_top_load_stays_at_the_meter_constant_rate(void)
{
    uint64_t fraction = 0;

    /* 41 kW at 2000 pulses/kWh is 22.77 pulses/s. */
    TEST_ASSERT_EQUAL_UINT32(22, nd_idis_energy_pulses(&fraction, 41000,
                                                       2000u));
    TEST_ASSERT_EQUAL_UINT64(777777u, fraction);

    /* A carried fraction may round the same second up to 23, never higher. */
    fraction = 999999u;
    TEST_ASSERT_EQUAL_UINT32(23, nd_idis_energy_pulses(&fraction, 41000,
                                                       2000u));

    /* 41 kW at 3200 pulses/kWh is 36.44 pulses/s. */
    fraction = 0;
    TEST_ASSERT_EQUAL_UINT32(36, nd_idis_energy_pulses(&fraction, 41000,
                                                       3200u));
}

static void test_normal_power_accumulates_fractions(void)
{
    uint64_t fraction = 0;

    TEST_ASSERT_EQUAL_UINT32(1, nd_idis_energy_pulses(&fraction, 1800, 2000u));
    TEST_ASSERT_EQUAL_UINT64(0, fraction);

    TEST_ASSERT_EQUAL_UINT32(0, nd_idis_energy_pulses(&fraction, 1799, 2000u));
    TEST_ASSERT_EQUAL_UINT64(999444u, fraction);

    TEST_ASSERT_EQUAL_UINT32(2, nd_idis_energy_pulses(&fraction, 3600, 2000u));
    TEST_ASSERT_EQUAL_UINT64(999444u, fraction);

    TEST_ASSERT_EQUAL_UINT32(0, nd_idis_energy_pulses(NULL, 1800, 2000u));
    TEST_ASSERT_EQUAL_UINT32(0, nd_idis_energy_pulses(&fraction, 1800, 0u));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_absurd_power_consumes_nothing);
    RUN_TEST(test_top_load_stays_at_the_meter_constant_rate);
    RUN_TEST(test_normal_power_accumulates_fractions);
    return UNITY_END();
}
