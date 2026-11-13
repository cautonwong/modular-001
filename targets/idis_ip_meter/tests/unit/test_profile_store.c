#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

static uint8_t memory[1024];
static nd_byte_store_t store;
static nd_profile_store_t profile;

void setUp(void)
{
    memset(memory, 0, sizeof(memory));
    nd_byte_store_init(&store, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &store.port, 0, 3,
                                                   64));
}

void tearDown(void) {}

static nd_datetime_t time_at(uint8_t minute)
{
    return (nd_datetime_t){.second = 0, .minute = minute, .hour = 8,
                           .day = 11, .month = 11, .year = 2026};
}

static void test_append_read_and_reopen(void)
{
    uint8_t record[] = {0x10, 0x20, 0x30};
    uint8_t out[8] = {0};
    size_t length = 0;

    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0x55,
                                                     record, sizeof(record)));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&profile));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(record), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(record, out, sizeof(record));

    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                   64));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(record, out, sizeof(record));
}

static void test_ring_wraps_and_keeps_newest_records(void)
{
    uint8_t out[8] = {0};
    size_t length = 0;

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t record = i;
        nd_datetime_t time = time_at(i);
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, i,
                                                         &record, 1));
    }
    TEST_ASSERT_EQUAL_UINT16(3, nd_profile_store_entries(&profile));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8(1, out[0]);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&profile, 2, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8(3, out[0]);
}

static void test_reset_clears_entries(void)
{
    uint8_t record = 0xAA;
    size_t length = 0;
    uint8_t out[4];

    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_reset(&profile));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&profile));
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                    &length));
}

static void test_corrupt_record_is_rejected(void)
{
    uint8_t record[] = {1, 2, 3};
    uint8_t out[4];
    size_t length = 0;

    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     record, sizeof(record)));
    memory[64 + 20] ^= 0x01;
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                    &length));
}

static void test_backup_metadata_recovers_after_primary_corruption(void)
{
    uint8_t record[] = {0x44, 0x55};
    uint8_t out[4] = {0};
    size_t length = 0;
    nd_datetime_t time = time_at(2);

    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     record, sizeof(record)));
    memory[0] ^= 0xFF;

    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                   64));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(record, out, sizeof(record));
}


static void test_recovery_after_unsynced_append(void)
{
    uint8_t first = 0x11;
    uint8_t second = 0x22;
    uint8_t out[4] = {0};
    size_t length = 0;
    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &first, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &second, 1));
    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                   64));
    TEST_ASSERT_EQUAL_UINT16(2, nd_profile_store_entries(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8(first, out[0]);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 1, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8(second, out[0]);
}

static void test_reset_does_not_resurrect_old_records(void)
{
    uint8_t old_record = 0x33;
    uint8_t new_record = 0x44;
    uint8_t out[4] = {0};
    size_t length = 0;
    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &old_record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_reset(&profile));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &new_record, 1));
    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                   64));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8(new_record, out[0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_append_read_and_reopen);
    RUN_TEST(test_ring_wraps_and_keeps_newest_records);
    RUN_TEST(test_reset_clears_entries);
    RUN_TEST(test_corrupt_record_is_rejected);
    RUN_TEST(test_backup_metadata_recovers_after_primary_corruption);
    RUN_TEST(test_recovery_after_unsynced_append);
    RUN_TEST(test_reset_does_not_resurrect_old_records);
    return UNITY_END();
}
