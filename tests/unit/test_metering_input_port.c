#include "unity.h"
#include "domain/metering/metering_input.h"

static int calls_a;
static int calls_b;
static nd_metering_input_t last_input;

static int fake_read_a(void *ctx, nd_metering_input_t input, int32_t *value)
{
    (void)ctx;
    calls_a++;
    last_input = input;
    *value = input == ND_METERING_INPUT_TEMPERATURE ? 2500 : 33000;
    return 1;
}

static int fake_read_b(void *ctx, nd_metering_input_t input, int32_t *value)
{
    (void)ctx;
    (void)input;
    calls_b++;
    *value = 0;
    return 0;
}

void setUp(void)
{
    calls_a = 0;
    calls_b = 0;
    last_input = ND_METERING_INPUT_TEMPERATURE;
}

void tearDown(void)
{
}

static void test_null_port_missing_callback_and_null_value_are_rejected(void)
{
    const nd_metering_input_port_t no_callback = { NULL, NULL };
    const nd_metering_input_port_t port = { NULL, fake_read_a };
    int32_t value = 123;

    TEST_ASSERT_EQUAL_INT(0, nd_metering_input_read(
        NULL, ND_METERING_INPUT_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT(0, nd_metering_input_read(
        &no_callback, ND_METERING_INPUT_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT(0, nd_metering_input_read(
        &port, ND_METERING_INPUT_BATTERY, NULL));
    TEST_ASSERT_EQUAL_INT(123, value);
    TEST_ASSERT_EQUAL_INT(0, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);
}

static void test_dispatch_both_channels(void)
{
    const nd_metering_input_port_t port = { NULL, fake_read_a };
    int32_t value = 0;

    TEST_ASSERT_EQUAL_INT(1, nd_metering_input_read(
        &port, ND_METERING_INPUT_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT(2500, value);
    TEST_ASSERT_EQUAL_INT(ND_METERING_INPUT_TEMPERATURE, last_input);

    TEST_ASSERT_EQUAL_INT(1, nd_metering_input_read(
        &port, ND_METERING_INPUT_BATTERY, &value));
    TEST_ASSERT_EQUAL_INT(33000, value);
    TEST_ASSERT_EQUAL_INT(ND_METERING_INPUT_BATTERY, last_input);
    TEST_ASSERT_EQUAL_INT(2, calls_a);
}

static void test_per_port_independence(void)
{
    const nd_metering_input_port_t port_a = { NULL, fake_read_a };
    const nd_metering_input_port_t port_b = { NULL, fake_read_b };
    int32_t value = 7;

    TEST_ASSERT_EQUAL_INT(0, nd_metering_input_read(
        &port_b, ND_METERING_INPUT_BATTERY, &value));
    TEST_ASSERT_EQUAL_INT(0, value);
    TEST_ASSERT_EQUAL_INT(0, calls_a);
    TEST_ASSERT_EQUAL_INT(1, calls_b);

    TEST_ASSERT_EQUAL_INT(1, nd_metering_input_read(
        &port_a, ND_METERING_INPUT_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT(2500, value);
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(1, calls_b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_port_missing_callback_and_null_value_are_rejected);
    RUN_TEST(test_dispatch_both_channels);
    RUN_TEST(test_per_port_independence);
    return UNITY_END();
}
