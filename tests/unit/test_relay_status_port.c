#include "unity.h"
#include "domain/load_control/relay_status.h"

static int calls_a;
static int calls_b;

static int fake_is_off_a(void *ctx)
{
    (void)ctx;
    calls_a++;
    return 1;
}

static int fake_is_off_b(void *ctx)
{
    (void)ctx;
    calls_b++;
    return 0;
}

void setUp(void)
{
    calls_a = 0;
    calls_b = 0;
}

void tearDown(void)
{
}

static void test_null_port_and_missing_callback_are_not_off(void)
{
    const nd_relay_status_port_t no_callback = { NULL, NULL };

    TEST_ASSERT_EQUAL_INT(0, nd_relay_status_is_off(NULL));
    TEST_ASSERT_EQUAL_INT(0, nd_relay_status_is_off(&no_callback));
    TEST_ASSERT_EQUAL_INT(0, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);
}

static void test_dispatch_and_per_port_independence(void)
{
    const nd_relay_status_port_t port_a = { NULL, fake_is_off_a };
    const nd_relay_status_port_t port_b = { NULL, fake_is_off_b };

    TEST_ASSERT_EQUAL_INT(1, nd_relay_status_is_off(&port_a));
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);

    TEST_ASSERT_EQUAL_INT(0, nd_relay_status_is_off(&port_b));
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(1, calls_b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_port_and_missing_callback_are_not_off);
    RUN_TEST(test_dispatch_and_per_port_independence);
    return UNITY_END();
}
