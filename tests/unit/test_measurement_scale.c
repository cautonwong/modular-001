#include "unity.h"

#include <math.h>

#include "components/core/types/measurement_scale.h"

void setUp(void) {}
void tearDown(void) {}

static void test_in_range_values_keep_the_plain_cast(void)
{
    TEST_ASSERT_EQUAL_INT64(0, measurement_scale_clamp(0.0f));
    TEST_ASSERT_EQUAL_INT64(1234, measurement_scale_clamp(1234.75f));
    TEST_ASSERT_EQUAL_INT64(-1234, measurement_scale_clamp(-1234.75f));
    TEST_ASSERT_EQUAL_INT64(2147483520, measurement_scale_clamp(2147483520.0f));
    TEST_ASSERT_EQUAL_INT64(INT32_MIN, measurement_scale_clamp(-2147483648.0f));
}

static void test_out_of_range_values_saturate(void)
{
    TEST_ASSERT_EQUAL_INT64(INT32_MAX, measurement_scale_clamp(2147483648.0f));
    TEST_ASSERT_EQUAL_INT64(INT32_MAX, measurement_scale_clamp(1.0e30f));
    TEST_ASSERT_EQUAL_INT64(INT32_MIN, measurement_scale_clamp(-2147483904.0f));
    TEST_ASSERT_EQUAL_INT64(INT32_MIN, measurement_scale_clamp(-1.0e30f));
}

static void test_non_finite_values_saturate(void)
{
    TEST_ASSERT_EQUAL_INT64(INT32_MIN, measurement_scale_clamp(NAN));
    TEST_ASSERT_EQUAL_INT64(INT32_MAX, measurement_scale_clamp(INFINITY));
    TEST_ASSERT_EQUAL_INT64(INT32_MIN, measurement_scale_clamp(-INFINITY));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_in_range_values_keep_the_plain_cast);
    RUN_TEST(test_out_of_range_values_saturate);
    RUN_TEST(test_non_finite_values_saturate);
    return UNITY_END();
}
