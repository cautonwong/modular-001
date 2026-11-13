#include "unity.h"
#include "domain/metering/metering_input.h"

static int calls;
static nd_metering_input_t last_input;
static int result;

static int fake_read(void *ctx, nd_metering_input_t input, int32_t *value)
{
    (void)ctx;
    calls++;
    last_input = input;
    if (result)
    {
        *value = input == ND_METERING_INPUT_TEMPERATURE ? 2500 : 33000;
    }
    return result;
}

void setUp(void)
{
    calls = 0;
    last_input = ND_METERING_INPUT_TEMPERATURE;
    result = 1;
    nd_metering_input_bind(NULL);
}

void tearDown(void)
{
}

static void test_unbound_input_returns_failure(void)
{
    int32_t value = 0;

    TEST_ASSERT_EQUAL_INT(0, nd_metering_input_read(ND_METERING_INPUT_TEMPERATURE,
                                                    &value));
    TEST_ASSERT_EQUAL_INT(0, calls);
}

static void test_bound_input_dispatches_both_channels(void)
{
    const nd_metering_input_port_t port = { NULL, fake_read };
    int32_t value = 0;

    nd_metering_input_bind(&port);
    TEST_ASSERT_EQUAL_INT(1, nd_metering_input_read(ND_METERING_INPUT_TEMPERATURE,
                                                    &value));
    TEST_ASSERT_EQUAL_INT(2500, value);
    TEST_ASSERT_EQUAL_INT(ND_METERING_INPUT_TEMPERATURE, last_input);

    TEST_ASSERT_EQUAL_INT(1, nd_metering_input_read(ND_METERING_INPUT_BATTERY,
                                                    &value));
    TEST_ASSERT_EQUAL_INT(33000, value);
    TEST_ASSERT_EQUAL_INT(ND_METERING_INPUT_BATTERY, last_input);
    TEST_ASSERT_EQUAL_INT(2, calls);
}

static void test_null_value_is_rejected(void)
{
    const nd_metering_input_port_t port = { NULL, fake_read };

    nd_metering_input_bind(&port);
    TEST_ASSERT_EQUAL_INT(0, nd_metering_input_read(ND_METERING_INPUT_BATTERY,
                                                    NULL));
    TEST_ASSERT_EQUAL_INT(0, calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_input_returns_failure);
    RUN_TEST(test_bound_input_dispatches_both_channels);
    RUN_TEST(test_null_value_is_rejected);
    return UNITY_END();
}
