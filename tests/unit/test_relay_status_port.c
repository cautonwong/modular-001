#include "unity.h"
#include "domain/load_control/relay_status.h"

static int is_off;
static int calls;

static int fake_is_off(void *ctx)
{
    (void)ctx;
    calls++;
    return is_off;
}

void setUp(void)
{
    is_off = 0;
    calls = 0;
    nd_relay_status_bind(NULL);
}

void tearDown(void)
{
}

static void test_unbound_relay_status_is_closed(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_relay_status_is_off());
    TEST_ASSERT_EQUAL_INT(0, calls);
}

static void test_bound_relay_status_dispatches(void)
{
    const nd_relay_status_port_t port = { NULL, fake_is_off };

    nd_relay_status_bind(&port);
    is_off = 1;
    TEST_ASSERT_EQUAL_INT(1, nd_relay_status_is_off());
    TEST_ASSERT_EQUAL_INT(1, calls);

    is_off = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_relay_status_is_off());
    TEST_ASSERT_EQUAL_INT(2, calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_relay_status_is_closed);
    RUN_TEST(test_bound_relay_status_dispatches);
    return UNITY_END();
}
