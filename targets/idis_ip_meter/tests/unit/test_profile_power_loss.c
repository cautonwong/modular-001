#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#define SECTOR_SIZE 4096u
#define SLOT_SIZE 128u
#define FLASH_SIZE 12288u

static uint8_t flash[FLASH_SIZE];
static uint8_t sector_buffer[SECTOR_SIZE];
static nd_sector_store_t sector_store;

static int ops;
static int fail_at = -1;

static int op_fails(void)
{
    int fail = ops == fail_at;
    ops++;
    return fail;
}

static int flash_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    (void)ctx;
    memcpy(buffer, flash + offset, length);
    return 0;
}

static int flash_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    if (op_fails()) {
        memset(flash + offset, 0xFF, SECTOR_SIZE / 2u);
        return -1;
    }
    memset(flash + offset, 0xFF, SECTOR_SIZE);
    return 0;
}

/* A failed write leaves half the bytes behind, like a torn program. */
static int flash_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    if (op_fails()) {
        memcpy(flash + offset, buffer, length / 2u);
        return -1;
    }
    memcpy(flash + offset, buffer, length);
    return 0;
}

static void open_store(nd_profile_store_t *profile, uint16_t capacity)
{
    nd_sector_store_init(&sector_store, NULL, flash_read, flash_erase,
                         flash_write, sizeof(flash), SECTOR_SIZE,
                         sector_buffer, 0);
    TEST_ASSERT_EQUAL_INT(
        0, nd_profile_store_init(profile,
                                 nd_sector_store_port(&sector_store), 0,
                                 capacity, SLOT_SIZE, SECTOR_SIZE, NULL));
}

static void check_records(const nd_profile_store_t *profile, uint16_t ack,
                          uint16_t entries)
{
    TEST_ASSERT_EQUAL_UINT16(entries, nd_profile_store_entries(profile));
    for (uint16_t index = 0; index < entries; index++) {
        uint8_t out = 0;
        size_t length = 0;
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(profile, index, &out,
                                                       sizeof(out), &length));
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(ack - entries + 1u + index), out);
    }
}

void setUp(void) {}

void tearDown(void) {}

static void test_fail_after_n_operations_loses_no_acknowledged_record(void)
{
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 13, .hour = 12,
    };
    const int appends = 100;
    const int clean_ops = appends + (appends + 31) / 32;
    for (int fail = 0; fail <= clean_ops; fail++) {
        memset(flash, 0xFF, sizeof(flash));
        ops = 0;
        fail_at = fail;
        nd_profile_store_t profile;
        open_store(&profile, 60);

        uint16_t ack = 0;
        for (int count = 1; count <= appends; count++) {
            uint8_t value = (uint8_t)count;
            if (nd_profile_store_append(&profile, &time, 0, &value, 1) != 0) {
                break;
            }
            ack++;
        }
        uint16_t entries = ack < 60u ? ack : 60u;
        TEST_ASSERT_EQUAL_UINT16(entries, nd_profile_store_entries(&profile));

        fail_at = -1;
        nd_profile_store_t reopened;
        open_store(&reopened, 60);
        check_records(&reopened, ack, entries);
    }
}

static void test_torn_reset_keeps_previous_records(void)
{
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 13, .hour = 12,
    };
    memset(flash, 0xFF, sizeof(flash));
    ops = 0;
    fail_at = -1;
    nd_profile_store_t profile;
    open_store(&profile, 60);
    for (uint8_t count = 1; count <= 3; count++) {
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                         &count, 1));
    }

    fail_at = ops;
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_reset(&profile));
    fail_at = -1;

    nd_profile_store_t reopened;
    open_store(&reopened, 60);
    check_records(&reopened, 3, 3);

    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_reset(&reopened));
    open_store(&profile, 60);
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&profile));
}

static uint16_t crc16_update(uint16_t crc, const uint8_t *data, size_t length)
{
    for (size_t index = 0; index < length; index++) {
        crc ^= (uint16_t)data[index] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void put_record(uint8_t *slot, uint16_t sequence, uint8_t payload)
{
    memset(slot, 0xFF, 64);
    slot[0] = 0x52;
    slot[1] = 0x45;
    slot[2] = 0x43;
    slot[3] = 0x31;
    slot[4] = 4;
    slot[13] = 0;
    slot[14] = 1;
    slot[15] = (uint8_t)(sequence >> 8);
    slot[16] = (uint8_t)sequence;
    slot[20] = payload;
    uint16_t crc = crc16_update(crc16_update(0xFFFFu, slot, 18), &slot[20], 1);
    slot[18] = (uint8_t)(crc >> 8);
    slot[19] = (uint8_t)crc;
}

static void check_payload(const nd_profile_store_t *profile, uint16_t index,
                          uint8_t expected)
{
    uint8_t out = 0;
    size_t length = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(profile, index, &out,
                                                   sizeof(out), &length));
    TEST_ASSERT_EQUAL_UINT8(expected, out);
}

static void test_sequence_wrap_recovers_adjacent_records(void)
{
    static uint8_t memory[1024];
    static nd_byte_store_t byte_store;
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 13, .hour = 12,
    };
    memset(memory, 0xFF, sizeof(memory));
    nd_byte_store_init(&byte_store, memory, sizeof(memory));
    put_record(memory, 65534u, 0xAA);
    put_record(memory + 64, 65535u, 0xBB);

    nd_profile_store_t profile;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &byte_store.port,
                                                   0, 3, 64, 0, NULL));
    TEST_ASSERT_EQUAL_UINT16(2, nd_profile_store_entries(&profile));
    check_payload(&profile, 0, 0xAA);
    check_payload(&profile, 1, 0xBB);

    uint8_t record = 0xCC;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &byte_store.port,
                                                   0, 3, 64, 0, NULL));
    TEST_ASSERT_EQUAL_UINT16(3, nd_profile_store_entries(&profile));
    check_payload(&profile, 0, 0xAA);
    check_payload(&profile, 1, 0xBB);
    check_payload(&profile, 2, 0xCC);

    record = 0xDD;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &byte_store.port,
                                                   0, 3, 64, 0, NULL));
    TEST_ASSERT_EQUAL_UINT16(3, nd_profile_store_entries(&profile));
    check_payload(&profile, 0, 0xBB);
    check_payload(&profile, 1, 0xCC);
    check_payload(&profile, 2, 0xDD);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fail_after_n_operations_loses_no_acknowledged_record);
    RUN_TEST(test_torn_reset_keeps_previous_records);
    RUN_TEST(test_sequence_wrap_recovers_adjacent_records);
    return UNITY_END();
}
