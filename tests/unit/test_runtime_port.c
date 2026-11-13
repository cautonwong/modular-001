#include "unity.h"
#include "domain/common/runtime.h"

static uint32_t last_us;
static int delay_calls;
static int watchdog_calls;

static void fake_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    delay_calls++;
    last_us = us;
}

static void fake_watchdog(void *ctx)
{
    (void)ctx;
    watchdog_calls++;
}

void setUp(void)
{
    last_us = 0;
    delay_calls = 0;
    watchdog_calls = 0;
    nd_runtime_bind(NULL);
}

void tearDown(void)
{
}

static void test_unbound_runtime_is_noop(void)
{
    nd_runtime_delay_us(1000);
    nd_runtime_watchdog_clear();
    TEST_ASSERT_EQUAL_INT(0, delay_calls);
    TEST_ASSERT_EQUAL_INT(0, watchdog_calls);
}

static void test_bound_runtime_dispatches(void)
{
    const nd_runtime_port_t port = { NULL, fake_delay, fake_watchdog };

    nd_runtime_bind(&port);
    nd_runtime_delay_us(5000);
    nd_runtime_watchdog_clear();
    TEST_ASSERT_EQUAL_INT(1, delay_calls);
    TEST_ASSERT_EQUAL_UINT32(5000, last_us);
    TEST_ASSERT_EQUAL_INT(1, watchdog_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_runtime_is_noop);
    RUN_TEST(test_bound_runtime_dispatches);
    return UNITY_END();
}
