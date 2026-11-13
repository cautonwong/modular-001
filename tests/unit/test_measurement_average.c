#include "unity.h"

#include "domain/metering/measurement_average.h"

static nd_measurement_average_t average;

void setUp(void)
{
    nd_measurement_average_init(&average);
}

void tearDown(void) {}

static nd_measurement_value_t sample(int64_t value, uint8_t status)
{
    return (nd_measurement_value_t){
        .value = value,
        .scaler = -1,
        .unit = ND_MEASUREMENT_UNIT_VOLT,
        .phase = ND_MEASURE_PHASE_L1,
        .status = status,
    };
}

static void test_average_keeps_scaler_unit_and_ors_status(void)
{
    nd_measurement_value_t first = sample(2300, 0);
    nd_measurement_value_t second = sample(2310, 1);
    TEST_ASSERT_EQUAL_INT(0, nd_measurement_average_add(&average, &first));
    TEST_ASSERT_EQUAL_INT(0, nd_measurement_average_add(&average, &second));
    nd_measurement_value_t out;
    TEST_ASSERT_EQUAL_INT(0, nd_measurement_average_value(&average, &out));
    TEST_ASSERT_EQUAL_INT64(2305, out.value);
    TEST_ASSERT_EQUAL_INT8(-1, out.scaler);
    TEST_ASSERT_EQUAL_UINT8(ND_MEASUREMENT_UNIT_VOLT, out.unit);
    TEST_ASSERT_EQUAL_UINT8(1, out.status);
}

static void test_average_rejects_mismatched_samples(void)
{
    nd_measurement_value_t first = sample(2300, 0);
    nd_measurement_value_t second = sample(2300, 0);
    second.phase = ND_MEASURE_PHASE_L2;
    TEST_ASSERT_EQUAL_INT(0, nd_measurement_average_add(&average, &first));
    TEST_ASSERT_EQUAL_INT(-1, nd_measurement_average_add(&average, &second));
    TEST_ASSERT_EQUAL_INT(-1, nd_measurement_average_value(NULL, &second));
    TEST_ASSERT_EQUAL_INT(0, nd_measurement_average_reset(&average));
    TEST_ASSERT_EQUAL_INT(-1, nd_measurement_average_value(&average, &second));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_average_keeps_scaler_unit_and_ors_status);
    RUN_TEST(test_average_rejects_mismatched_samples);
    return UNITY_END();
}
