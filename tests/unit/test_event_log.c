#include "unity.h"
#include <string.h>

#include "domain/events/event_log.h"

static nd_event_log_storage_t event_log_storage;
static nd_event_log_t *event_log;

void setUp(void)
{
    event_log = nd_event_log_of(&event_log_storage);
    nd_event_log_init(event_log);
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
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(event_log, &first));
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(event_log, &second));
    TEST_ASSERT_EQUAL_UINT16(2, nd_event_log_count(event_log));
    TEST_ASSERT_EQUAL_UINT8(1, nd_event_log_at(event_log, 0)->time.second);
    TEST_ASSERT_EQUAL_UINT8(2, nd_event_log_latest(event_log)->time.second);
    TEST_ASSERT_EQUAL_UINT32(1, nd_event_log_counter(event_log, ND_EVENT_POWER_OFF));
    TEST_ASSERT_EQUAL_UINT32(1, nd_event_log_counter(event_log, ND_EVENT_COVER_OPEN));
    TEST_ASSERT_EQUAL_UINT32(0, nd_event_log_counter(event_log, ND_EVENT_GENERIC));
}

static void test_wrap_keeps_newest_entries(void)
{
    for (uint8_t index = 0; index < ND_EVENT_LOG_MAX + 2; index++) {
        nd_event_record_t record = make_event(ND_EVENT_GENERIC, index);
        TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(event_log, &record));
    }
    TEST_ASSERT_EQUAL_UINT16(ND_EVENT_LOG_MAX, nd_event_log_count(event_log));
    TEST_ASSERT_EQUAL_UINT8(2, nd_event_log_at(event_log, 0)->time.second);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_LOG_MAX + 1,
                            nd_event_log_latest(event_log)->time.second);
    TEST_ASSERT_EQUAL_UINT32(ND_EVENT_LOG_MAX + 2,
                             nd_event_log_counter(event_log, ND_EVENT_GENERIC));
}

static void test_reset_and_invalid_input(void)
{
    nd_event_record_t record = make_event(ND_EVENT_GENERIC, 0);
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_append(NULL, &record));
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_append(event_log, NULL));
    record.kind = ND_EVENT_KIND_COUNT;
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_append(event_log, &record));
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_reset(event_log));
    TEST_ASSERT_EQUAL_UINT16(0, nd_event_log_count(event_log));
    TEST_ASSERT_NULL(nd_event_log_latest(event_log));
}

static void test_export_import_round_trip(void)
{
    for (uint8_t index = 0; index < ND_EVENT_LOG_MAX + 3; index++) {
        nd_event_record_t record =
            make_event((nd_event_kind_t)(1 + index % 3), index);
        record.value = index;
        TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(event_log, &record));
    }

    nd_event_record_t entries[ND_EVENT_LOG_MAX];
    nd_event_log_export(event_log, entries);

    nd_event_log_storage_t restored_storage;
    nd_event_log_t *restored = nd_event_log_of(&restored_storage);
    nd_event_log_init(restored);
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_import(
        restored, entries, nd_event_log_count(event_log),
        nd_event_log_head(event_log)));
    TEST_ASSERT_EQUAL_UINT16(nd_event_log_count(event_log),
                             nd_event_log_count(restored));
    TEST_ASSERT_EQUAL_UINT16(nd_event_log_head(event_log),
                             nd_event_log_head(restored));
    for (uint16_t index = 0; index < ND_EVENT_LOG_MAX; index++) {
        const nd_event_record_t *expected = nd_event_log_at(event_log, index);
        const nd_event_record_t *actual = nd_event_log_at(restored, index);
        TEST_ASSERT_EQUAL_INT(expected->kind, actual->kind);
        TEST_ASSERT_EQUAL_INT32(expected->value, actual->value);
        TEST_ASSERT_EQUAL_UINT8(expected->time.second, actual->time.second);
    }
}

static void test_import_clamps_persisted_count_and_head(void)
{
    nd_event_record_t entries[ND_EVENT_LOG_MAX] = {0};
    nd_event_log_storage_t restored_storage;
    nd_event_log_t *restored = nd_event_log_of(&restored_storage);
    nd_event_log_init(restored);
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_import(restored, entries, 200,
                                                 ND_EVENT_LOG_MAX));
    TEST_ASSERT_EQUAL_UINT16(ND_EVENT_LOG_MAX,
                             nd_event_log_count(restored));
    TEST_ASSERT_EQUAL_UINT16(0, nd_event_log_head(restored));
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_import(NULL, entries, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_event_log_import(restored, NULL, 0, 0));
}

static void test_export_matches_persisted_layout(void)
{
    nd_event_record_t first;
    memset(&first, 0, sizeof(first));
    first.kind = ND_EVENT_POWER_OFF;
    first.time.second = 1;
    first.time.minute = 2;
    first.time.hour = 3;
    first.time.day = 4;
    first.time.month = 5;
    first.time.year = 2026;
    first.source = 6;
    first.severity = 7;
    first.duration_seconds = 8;
    first.value = 9;

    nd_event_record_t second;
    memset(&second, 0, sizeof(second));
    second.kind = ND_EVENT_COVER_OPEN;
    second.time.second = 10;
    second.time.minute = 11;
    second.time.hour = 12;
    second.time.day = 13;
    second.time.month = 6;
    second.time.year = 2027;
    second.source = 8;
    second.severity = 9;
    second.duration_seconds = 3600;
    second.value = -45;

    TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(event_log, &first));
    TEST_ASSERT_EQUAL_INT(0, nd_event_log_append(event_log, &second));

    nd_event_record_t entries[ND_EVENT_LOG_MAX];
    nd_event_log_export(event_log, entries);
    TEST_ASSERT_EQUAL_UINT16(2, nd_event_log_count(event_log));
    TEST_ASSERT_EQUAL_UINT16(2, nd_event_log_head(event_log));

    static const uint8_t golden[] = {
        0x01, 0x00, 0x00, 0x00,          /* kind: ND_EVENT_POWER_OFF */
        0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0xEA, 0x07, /* time */
        0x06, 0x07, 0x00, 0x00,          /* source, severity, padding */
        0x08, 0x00, 0x00, 0x00,          /* duration_seconds */
        0x09, 0x00, 0x00, 0x00,          /* value */
        0x03, 0x00, 0x00, 0x00,          /* kind: ND_EVENT_COVER_OPEN */
        0x0A, 0x0B, 0x0C, 0x0D, 0x06, 0x00, 0xEB, 0x07, /* time */
        0x08, 0x09, 0x00, 0x00,          /* source, severity, padding */
        0x10, 0x0E, 0x00, 0x00,          /* duration_seconds: 3600 */
        0xD3, 0xFF, 0xFF, 0xFF,          /* value: -45 */
    };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(golden, entries, sizeof(golden));
    for (size_t index = sizeof(golden); index < sizeof(entries); index++) {
        TEST_ASSERT_EQUAL_HEX8(0, ((const uint8_t *)entries)[index]);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_append_latest_and_counters);
    RUN_TEST(test_wrap_keeps_newest_entries);
    RUN_TEST(test_reset_and_invalid_input);
    RUN_TEST(test_export_import_round_trip);
    RUN_TEST(test_import_clamps_persisted_count_and_head);
    RUN_TEST(test_export_matches_persisted_layout);
    return UNITY_END();
}
