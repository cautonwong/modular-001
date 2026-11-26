#include "unity.h"
#include "targets/stategrid_meter/app/display/backlight.h"

static int last_on;
static int calls_a;
static int calls_b;

static void fake_set_a(void *ctx, int on)
{
    (void)ctx;
    calls_a++;
    last_on = on;
}

static void fake_set_b(void *ctx, int on)
{
    (void)ctx;
    (void)on;
    calls_b++;
}

void setUp(void)
{
    last_on = -1;
    calls_a = 0;
    calls_b = 0;
}

void tearDown(void)
{
}

static void test_null_port_and_missing_callback_are_noops(void)
{
    const nd_backlight_port_t no_callback = { NULL, NULL };

    nd_backlight_set(NULL, 1);
    nd_backlight_set(&no_callback, 1);
    TEST_ASSERT_EQUAL_INT(0, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);
    TEST_ASSERT_EQUAL_INT(-1, last_on);
}

static void test_dispatch_and_per_port_independence(void)
{
    const nd_backlight_port_t port_a = { NULL, fake_set_a };
    const nd_backlight_port_t port_b = { NULL, fake_set_b };

    nd_backlight_set(&port_a, 1);
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);
    TEST_ASSERT_EQUAL_INT(1, last_on);

    nd_backlight_set(&port_b, 0);
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(1, calls_b);

    nd_backlight_set(&port_a, 0);
    TEST_ASSERT_EQUAL_INT(2, calls_a);
    TEST_ASSERT_EQUAL_INT(0, last_on);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_port_and_missing_callback_are_noops);
    RUN_TEST(test_dispatch_and_per_port_independence);
    return UNITY_END();
}
