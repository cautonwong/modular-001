#include "unity.h"

#include "application/ports/platform.h"

#include <stdbool.h>

static int watchdog_calls;
static int event_calls;
static bool event_enabled;

static void fake_watchdog(void *ctx)
{
    (void)ctx;
    watchdog_calls++;
}

static void fake_event(void *ctx, bool enable)
{
    (void)ctx;
    event_calls++;
    event_enabled = enable;
}

static const nd_platform_port_t fake_port = { NULL, fake_watchdog, fake_event };

void setUp(void)
{
    watchdog_calls = 0;
    event_calls = 0;
    event_enabled = false;
    nd_platform_bind(NULL);
}

void tearDown(void)
{
}

static void test_unbound_port_is_safe(void)
{
    nd_platform_watchdog_kick();
    nd_platform_event_output_set(true);
    TEST_ASSERT_EQUAL_INT(0, watchdog_calls);
    TEST_ASSERT_EQUAL_INT(0, event_calls);
}

static void test_bound_port_dispatches_both_calls(void)
{
    nd_platform_bind(&fake_port);

    nd_platform_watchdog_kick();
    nd_platform_event_output_set(true);
    nd_platform_event_output_set(false);

    TEST_ASSERT_EQUAL_INT(1, watchdog_calls);
    TEST_ASSERT_EQUAL_INT(2, event_calls);
    TEST_ASSERT_FALSE(event_enabled);
}

static void test_rebinding_replaces_the_port(void)
{
    static const nd_platform_port_t second = { NULL, fake_watchdog, fake_event };

    nd_platform_bind(&fake_port);
    nd_platform_bind(&second);
    nd_platform_watchdog_kick();
    nd_platform_event_output_set(true);

    TEST_ASSERT_EQUAL_INT(1, watchdog_calls);
    TEST_ASSERT_EQUAL_INT(1, event_calls);
    TEST_ASSERT_TRUE(event_enabled);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_port_is_safe);
    RUN_TEST(test_bound_port_dispatches_both_calls);
    RUN_TEST(test_rebinding_replaces_the_port);
    return UNITY_END();
}
