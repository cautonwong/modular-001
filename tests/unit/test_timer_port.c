#include "unity.h"
#include "domain/common/timer.h"

static uint8_t last_id;
static uint32_t last_count;
static int calls_a;
static int calls_b;

static void fake_start_a(void *ctx, uint8_t timer_id, uint32_t count)
{
    (void)ctx;
    calls_a++;
    last_id = timer_id;
    last_count = count;
}

static void fake_start_b(void *ctx, uint8_t timer_id, uint32_t count)
{
    (void)ctx;
    (void)timer_id;
    (void)count;
    calls_b++;
}

void setUp(void)
{
    last_id = 0;
    last_count = 0;
    calls_a = 0;
    calls_b = 0;
}

void tearDown(void)
{
}

static void test_null_port_and_missing_callback_are_noops(void)
{
    const nd_timer_port_t no_callback = { NULL, NULL };

    nd_timer_start_one(NULL, ND_TIMER_MULTIFUNCTION_PORT, 10);
    nd_timer_start_one(&no_callback, ND_TIMER_MULTIFUNCTION_PORT, 10);
    TEST_ASSERT_EQUAL_INT(0, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);
}

static void test_dispatch_and_per_port_independence(void)
{
    const nd_timer_port_t port_a = { NULL, fake_start_a };
    const nd_timer_port_t port_b = { NULL, fake_start_b };

    nd_timer_start_one(&port_a, ND_TIMER_MULTIFUNCTION_PORT, 8);
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(0, calls_b);
    TEST_ASSERT_EQUAL_UINT8(ND_TIMER_MULTIFUNCTION_PORT, last_id);
    TEST_ASSERT_EQUAL_UINT32(8, last_count);

    nd_timer_start_one(&port_b, 2, 3);
    TEST_ASSERT_EQUAL_INT(1, calls_a);
    TEST_ASSERT_EQUAL_INT(1, calls_b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_null_port_and_missing_callback_are_noops);
    RUN_TEST(test_dispatch_and_per_port_independence);
    return UNITY_END();
}
