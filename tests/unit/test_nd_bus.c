#include "unity.h"

#include "components/core/bus/nd_bus.h"

#include <string.h>

static int key_calls;
static int power_calls;
static nd_event_t last_event;
static int call_order[8];
static int call_count;

static void key_handler(void *ctx, const nd_event_t *event)
{
    int *calls = ctx;

    (*calls)++;
    last_event = *event;
    if (call_count < 8) {
        call_order[call_count++] = 1;
    }
}

static void power_handler(void *ctx, const nd_event_t *event)
{
    int *calls = ctx;

    (*calls)++;
    last_event = *event;
    if (call_count < 8) {
        call_order[call_count++] = 2;
    }
}

static void late_handler(void *ctx, const nd_event_t *event)
{
    int *calls = ctx;

    (void)event;
    (*calls)++;
}

void setUp(void)
{
    key_calls = 0;
    power_calls = 0;
    call_count = 0;
    memset(&last_event, 0, sizeof(last_event));
    nd_bus_init();
}

void tearDown(void)
{
}

static void test_publish_without_subscribers_returns_zero(void)
{
    nd_event_t event = { ND_EVENT_TICK_SECOND, 10, { 0 } };

    TEST_ASSERT_EQUAL_INT(0, nd_bus_publish(&event));
}

static void test_subscribe_dispatches_only_matching_topic(void)
{
    nd_event_t event = { ND_EVENT_KEY, 1234, { 0 } };

    TEST_ASSERT_EQUAL_INT(0, nd_bus_subscribe(ND_EVENT_KEY, key_handler,
                                              &key_calls));
    TEST_ASSERT_EQUAL_INT(0, nd_bus_subscribe(ND_EVENT_POWER_CHANGE,
                                              power_handler, &power_calls));

    event.data.key.key = 3;
    event.data.key.state = 1;
    TEST_ASSERT_EQUAL_INT(1, nd_bus_publish(&event));
    TEST_ASSERT_EQUAL_INT(1, key_calls);
    TEST_ASSERT_EQUAL_INT(0, power_calls);
    TEST_ASSERT_EQUAL_INT(ND_EVENT_KEY, last_event.topic);
    TEST_ASSERT_EQUAL_UINT32(1234, last_event.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(3, last_event.data.key.key);
    TEST_ASSERT_EQUAL_INT(1, last_event.data.key.state);
}

static void test_subscribers_run_in_registration_order(void)
{
    nd_event_t event = { ND_EVENT_TICK_SECOND, 1, { 0 } };

    nd_bus_subscribe(ND_EVENT_TICK_SECOND, key_handler, &key_calls);
    nd_bus_subscribe(ND_EVENT_TICK_SECOND, power_handler, &power_calls);

    TEST_ASSERT_EQUAL_INT(2, nd_bus_publish(&event));
    TEST_ASSERT_EQUAL_INT(2, call_count);
    TEST_ASSERT_EQUAL_INT(1, call_order[0]);
    TEST_ASSERT_EQUAL_INT(2, call_order[1]);
}

static void test_dl698_frame_payload_is_carried(void)
{
    nd_event_t event = { ND_EVENT_DL698_FRAME, 88, { 0 } };

    event.data.frame.channel = 2;
    event.data.frame.length = 42;
    nd_bus_subscribe(ND_EVENT_DL698_FRAME, key_handler, &key_calls);
    TEST_ASSERT_EQUAL_INT(1, nd_bus_publish(&event));
    TEST_ASSERT_EQUAL_INT(ND_EVENT_DL698_FRAME, last_event.topic);
    TEST_ASSERT_EQUAL_UINT32(88, last_event.timestamp_ms);
    TEST_ASSERT_EQUAL_INT(2, last_event.data.frame.channel);
    TEST_ASSERT_EQUAL_INT(42, last_event.data.frame.length);
}

static void test_subscribe_rejects_invalid_arguments(void)
{
    int i;

    TEST_ASSERT_EQUAL_INT(-1, nd_bus_subscribe(ND_EVENT_KEY, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_bus_subscribe(ND_EVENT_TOPIC_COUNT,
                                               key_handler, &key_calls));

    for (i = 0; i < ND_BUS_MAX_SUBSCRIBERS; i++) {
        TEST_ASSERT_EQUAL_INT(0, nd_bus_subscribe(ND_EVENT_KEY,
                                                  key_handler, &key_calls));
    }
    TEST_ASSERT_EQUAL_INT(-1, nd_bus_subscribe(ND_EVENT_KEY, key_handler,
                                               &key_calls));
}

static void test_publish_rejects_invalid_event(void)
{
    nd_event_t event = { ND_EVENT_TOPIC_COUNT, 0, { 0 } };

    TEST_ASSERT_EQUAL_INT(-1, nd_bus_publish(NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_bus_publish(&event));
}

static void test_reset_clears_subscriptions(void)
{
    nd_event_t event = { ND_EVENT_ALARM, 5, { 0 } };

    nd_bus_subscribe(ND_EVENT_ALARM, key_handler, &key_calls);
    nd_bus_reset();
    TEST_ASSERT_EQUAL_INT(0, nd_bus_publish(&event));
    TEST_ASSERT_EQUAL_INT(0, key_calls);
}

static void late_subscribe_handler(void *ctx, const nd_event_t *event)
{
    (void)event;
    (void)ctx;
    nd_bus_subscribe(ND_EVENT_ALARM, late_handler, &power_calls);
}

static void test_subscribe_during_publish_waits_for_next_event(void)
{
    nd_event_t event = { ND_EVENT_ALARM, 7, { 0 } };

    nd_bus_subscribe(ND_EVENT_ALARM, late_subscribe_handler, NULL);
    TEST_ASSERT_EQUAL_INT(1, nd_bus_publish(&event));
    TEST_ASSERT_EQUAL_INT(2, nd_bus_publish(&event));
    TEST_ASSERT_EQUAL_INT(1, power_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_publish_without_subscribers_returns_zero);
    RUN_TEST(test_subscribe_dispatches_only_matching_topic);
    RUN_TEST(test_subscribers_run_in_registration_order);
    RUN_TEST(test_dl698_frame_payload_is_carried);
    RUN_TEST(test_subscribe_rejects_invalid_arguments);
    RUN_TEST(test_publish_rejects_invalid_event);
    RUN_TEST(test_reset_clears_subscriptions);
    RUN_TEST(test_subscribe_during_publish_waits_for_next_event);
    return UNITY_END();
}
