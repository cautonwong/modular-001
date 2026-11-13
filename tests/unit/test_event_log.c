#include "unity.h"
#include <string.h>

#include "domain/events/event_log.h"

static nd_event_log_t event_log;

void setUp(void)
{
    nd_event_log_init(&event_log);
}

void tearDown(void) {}

static nd_event_record_t make_event(nd_event_kind_t kind, uint8_t second)
{
    nd_event_record_t record = {.kind = kind, .time = {.second = second}};
    return record;
}

static void test_append_latest_and_counters(void)
{
    nd_event_record_t first = make_event(ND_EVENT_POWER_OFF, 1);
    nd_event_record_t second = make_event(ND_EVENT_COVER_OPEN, 2);
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(&event_log, &first));
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(&event_log, &second));
    TEST_ASSERT_EQUAL_UINT16(2, nd_event_log_count(&event_log));
    TEST_ASSERT_EQUAL_UINT8(1, nd_event_log_at(&event_log, 0)->time.second);
    TEST_ASSERT_EQUAL_UINT8(2, nd_event_log_latest(&event_log)->time.second);
    TEST_ASSERT_EQUAL_UINT32(1, nd_event_log_counter(&event_log, ND_EVENT_POWER_OFF));
    TEST_ASSERT_EQUAL_UINT32(1, nd_event_log_counter(&event_log, ND_EVENT_COVER_OPEN));
    TEST_ASSERT_EQUAL_UINT32(0, nd_event_log_counter(&event_log, ND_EVENT_GENERIC));
}

static void test_wrap_keeps_newest_entries(void)
{
    for (uint8_t index = 0; index < ND_EVENT_LOG_MAX + 2; index++) {
        nd_event_record_t record = make_event(ND_EVENT_GENERIC, index);
        TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(&event_log, &record));
    }
    TEST_ASSERT_EQUAL_UINT16(ND_EVENT_LOG_MAX, nd_event_log_count(&event_log));
    TEST_ASSERT_EQUAL_UINT8(2, nd_event_log_at(&event_log, 0)->time.second);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_LOG_MAX + 1,
                            nd_event_log_latest(&event_log)->time.second);
    TEST_ASSERT_EQUAL_UINT32(ND_EVENT_LOG_MAX + 2,
                             nd_event_log_counter(&event_log, ND_EVENT_GENERIC));
}

static void test_reset_and_invalid_input(void)
{
    nd_event_record_t record = make_event(ND_EVENT_GENERIC, 0);
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_append(NULL, &record));
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_append(&event_log, NULL));
    record.kind = ND_EVENT_KIND_COUNT;
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_append(&event_log, &record));
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_reset(&event_log));
    TEST_ASSERT_EQUAL_UINT16(0, nd_event_log_count(&event_log));
    TEST_ASSERT_NULL(nd_event_log_latest(&event_log));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_append_latest_and_counters);
    RUN_TEST(test_wrap_keeps_newest_entries);
    RUN_TEST(test_reset_and_invalid_input);
    return UNITY_END();
}
