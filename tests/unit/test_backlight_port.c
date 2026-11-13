#include "unity.h"
#include "domain/display/backlight.h"

static int last_on;
static int calls;

static void fake_set(void *ctx, int on)
{
    (void)ctx;
    calls++;
    last_on = on;
}

void setUp(void)
{
    last_on = -1;
    calls = 0;
    nd_backlight_bind(NULL);
}

void tearDown(void)
{
}

static void test_unbound_backlight_is_noop(void)
{
    nd_backlight_set(1);
    TEST_ASSERT_EQUAL_INT(0, calls);
}

static void test_bound_backlight_dispatches(void)
{
    const nd_backlight_port_t port = { NULL, fake_set };

    nd_backlight_bind(&port);
    nd_backlight_set(1);
    TEST_ASSERT_EQUAL_INT(1, calls);
    TEST_ASSERT_EQUAL_INT(1, last_on);

    nd_backlight_set(0);
    TEST_ASSERT_EQUAL_INT(2, calls);
    TEST_ASSERT_EQUAL_INT(0, last_on);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_backlight_is_noop);
    RUN_TEST(test_bound_backlight_dispatches);
    return UNITY_END();
}
