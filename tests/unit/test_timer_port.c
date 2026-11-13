#include "unity.h"
#include "domain/common/timer.h"

static uint8_t last_id;
static uint32_t last_count;
static int calls;

static void fake_start_one(void *ctx, uint8_t timer_id, uint32_t count)
{
    (void)ctx;
    calls++;
    last_id = timer_id;
    last_count = count;
}

void setUp(void)
{
    last_id = 0;
    last_count = 0;
    calls = 0;
    nd_timer_bind(NULL);
}

void tearDown(void)
{
}

static void test_unbound_timer_is_noop(void)
{
    nd_timer_start_one(ND_TIMER_MULTIFUNCTION_PORT, 10);
    TEST_ASSERT_EQUAL_INT(0, calls);
}

static void test_bound_timer_dispatches(void)
{
    const nd_timer_port_t port = { NULL, fake_start_one };

    nd_timer_bind(&port);
    nd_timer_start_one(ND_TIMER_MULTIFUNCTION_PORT, 8);
    TEST_ASSERT_EQUAL_INT(1, calls);
    TEST_ASSERT_EQUAL_UINT8(ND_TIMER_MULTIFUNCTION_PORT, last_id);
    TEST_ASSERT_EQUAL_UINT32(8, last_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_timer_is_noop);
    RUN_TEST(test_bound_timer_dispatches);
    return UNITY_END();
}
