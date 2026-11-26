#include "unity.h"

#include "application/ports/platform.h"

#include <stdbool.h>

static int watchdog_calls;
static int other_watchdog_calls;
static int event_calls;
static int reported_calls;
static bool event_enabled;

static void fake_watchdog(void *ctx)
{
    (void)ctx;
    watchdog_calls++;
}

static void other_watchdog(void *ctx)
{
    (void)ctx;
    other_watchdog_calls++;
}

static void fake_event(void *ctx, bool enable)
{
    (void)ctx;
    event_calls++;
    event_enabled = enable;
}

static bool fake_error(void *ctx)
{
    (void)ctx;
    return reported_calls == 0;
}

static void fake_reported(void *ctx)
{
    (void)ctx;
    reported_calls++;
}

static const nd_platform_port_t fake_port = {
    NULL, fake_watchdog, fake_event, fake_error, fake_reported
};

void setUp(void)
{
    watchdog_calls = 0;
    other_watchdog_calls = 0;
    event_calls = 0;
    reported_calls = 0;
    event_enabled = false;
}

void tearDown(void)
{
}

static void test_null_port_is_safe(void)
{
    nd_platform_watchdog_kick(NULL);
    nd_platform_event_output_set(NULL, true);
    TEST_ASSERT_FALSE(nd_platform_device_error(NULL));
    nd_platform_device_error_reported(NULL);

    TEST_ASSERT_EQUAL_INT(0, watchdog_calls);
    TEST_ASSERT_EQUAL_INT(0, event_calls);
    TEST_ASSERT_EQUAL_INT(0, reported_calls);
}

static void test_port_dispatches_every_action(void)
{
    nd_platform_watchdog_kick(&fake_port);
    nd_platform_event_output_set(&fake_port, true);
    nd_platform_event_output_set(&fake_port, false);
    TEST_ASSERT_TRUE(nd_platform_device_error(&fake_port));
    nd_platform_device_error_reported(&fake_port);
    TEST_ASSERT_FALSE(nd_platform_device_error(&fake_port));

    TEST_ASSERT_EQUAL_INT(1, watchdog_calls);
    TEST_ASSERT_EQUAL_INT(2, event_calls);
    TEST_ASSERT_EQUAL_INT(1, reported_calls);
    TEST_ASSERT_FALSE(event_enabled);
}

static void test_each_call_dispatches_its_own_port(void)
{
    static const nd_platform_port_t other_port = { NULL, other_watchdog };

    nd_platform_watchdog_kick(&other_port);
    nd_platform_watchdog_kick(&fake_port);

    TEST_ASSERT_EQUAL_INT(1, other_watchdog_calls);
    TEST_ASSERT_EQUAL_INT(1, watchdog_calls);
}

static void test_missing_callbacks_are_safe(void)
{
    static const nd_platform_port_t partial_port = { NULL, fake_watchdog };

    nd_platform_event_output_set(&partial_port, true);
    TEST_ASSERT_FALSE(nd_platform_device_error(&partial_port));
    nd_platform_device_error_reported(&partial_port);

    TEST_ASSERT_EQUAL_INT(0, event_calls);
    TEST_ASSERT_EQUAL_INT(0, reported_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_port_is_safe);
    RUN_TEST(test_port_dispatches_every_action);
    RUN_TEST(test_each_call_dispatches_its_own_port);
    RUN_TEST(test_missing_callbacks_are_safe);
    return UNITY_END();
}
