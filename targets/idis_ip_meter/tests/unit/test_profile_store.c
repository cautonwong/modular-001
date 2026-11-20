#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

static uint8_t memory[1024];
static nd_byte_store_t store;
static nd_profile_store_t profile;

static const nd_profile_store_opts_t legacy_opts = {
    .legacy_base_offset = 0,
};

/* Goldens are the on-media record documented by nd_profile_store.c: a 20 B
 * big-endian header plus payload. CRC-16 is CCITT-FALSE over the first 18
 * header bytes and the payload. A v3 record is the pre-v4 layout that init
 * must hold instead of adopting. */
static const uint8_t GOLDEN_V4_RECORD[23] = {
    0x52, 0x45, 0x43, 0x31, 0x04, 0x55, 0x07, 0xEA, 0x0B, 0x0D, 0x0C, 0x22,
    0x38, 0x00, 0x03, 0x00, 0x01, 0x00, 0x5A, 0xF4, 0x10, 0x20, 0x30,
};
static const uint8_t GOLDEN_V3_RECORD[21] = {
    0x52, 0x45, 0x43, 0x31, 0x03, 0x00, 0x07, 0xEA, 0x0B, 0x0D, 0x0C,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0xC4, 0x46, 0x11,
};
static const uint8_t GOLDEN_MARK_RECORD[20] = {
    0x54, 0x4D, 0x42, 0x31, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x3A, 0xF6,
};

typedef struct {
    nd_store_t port;
    nd_store_t inner;
    uint32_t fail_from;
    int fail_all;
} fault_store_t;

static int fault_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    fault_store_t *fault = ctx;
    if (fault->fail_all || offset >= fault->fail_from) {
        return -1;
    }
    return fault->inner.read(fault->inner.ctx, offset, buffer, length);
}

static int fault_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    fault_store_t *fault = ctx;
    return fault->inner.write(fault->inner.ctx, offset, buffer, length);
}

static int fault_erase(void *ctx, uint32_t offset, size_t length)
{
    fault_store_t *fault = ctx;
    return fault->inner.erase(fault->inner.ctx, offset, length);
}

void setUp(void)
{
    memset(memory, 0, sizeof(memory));
    nd_byte_store_init(&store, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &store.port, 0, 3,
                                                   64, 0, NULL));
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
                                                   64, 0, NULL));
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
    memory[20] ^= 0x01;
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                    &length));
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
                                                   64, 0, NULL));
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
                                                   64, 0, NULL));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT8(new_record, out[0]);
}

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
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

static uint16_t crc16(const uint8_t *data, size_t length)
{
    return crc16_update(0xFFFFu, data, length);
}

static void put_legacy_record(uint8_t *slot, uint16_t sequence)
{
    memset(slot, 0, 64);
    put_u32(slot, 0x52454331u);
    slot[4] = 3;
    put_u16(&slot[13], 1);
    put_u16(&slot[15], sequence);
    slot[20] = 0x11;
    put_u16(&slot[18], crc16_update(crc16(slot, 18), &slot[20], 1));
}

static void test_legacy_metadata_fails_closed_until_reset(void)
{
    uint8_t meta[32] = {0};
    put_u32(meta, 0x49445031u);
    meta[4] = 3;
    put_u16(&meta[5], 64);
    put_u16(&meta[7], 3);
    put_u16(&meta[15], 1);
    put_u16(&meta[30], crc16(meta, 30));
    memcpy(memory, meta, sizeof(meta));

    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(ND_PROFILE_STORE_LEGACY_IMAGE,
                          nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                64, 0, &legacy_opts));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&reopened));
    uint8_t blocked = 0x55;
    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_append(&reopened, &time, 0,
                                                      &blocked, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_reset(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0,
                                                   3, 64, 0, &legacy_opts));
    uint8_t record = 0x77;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&reopened, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0,
                                                   3, 64, 0, &legacy_opts));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&reopened));
}

static void test_legacy_record_fails_closed_until_reset(void)
{
    put_legacy_record(memory + 64, 1);

    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(ND_PROFILE_STORE_LEGACY_IMAGE,
                          nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                64, 0, &legacy_opts));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_reset(&reopened));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&reopened, &store.port, 0,
                                                   3, 64, 0, &legacy_opts));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&reopened));
}

static void test_read_error_during_recovery_fails_init(void)
{
    fault_store_t fault = {
        .inner = *nd_byte_store_port(&store),
        .fail_from = 0,
        .fail_all = 1,
    };
    fault.port = (nd_store_t){.ctx = &fault, .read = fault_read,
                              .write = fault_write, .erase = fault_erase};
    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_init(&reopened, &fault.port, 0,
                                                    3, 64, 0, NULL));
}

static void test_read_error_mid_scan_fails_init(void)
{
    uint8_t record = 0x22;
    nd_datetime_t time = time_at(1);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &record, 1));
    fault_store_t fault = {
        .inner = *nd_byte_store_port(&store),
        .fail_from = 64,
        .fail_all = 0,
    };
    fault.port = (nd_store_t){.ctx = &fault, .read = fault_read,
                              .write = fault_write, .erase = fault_erase};
    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_init(&reopened, &fault.port, 0,
                                                    3, 64, 0, NULL));
}

static void test_v4_record_matches_the_documented_golden(void)
{
    uint8_t record[] = {0x10, 0x20, 0x30};
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 13, .hour = 12, .minute = 34,
        .second = 56,
    };

    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0x55,
                                                     record, sizeof(record)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_V4_RECORD, memory,
                                  sizeof(GOLDEN_V4_RECORD));
}

static void test_literal_v4_golden_loads(void)
{
    uint8_t out[4] = {0};
    size_t length = 0;

    memset(memory, 0xFF, sizeof(memory));
    memcpy(memory, GOLDEN_V4_RECORD, sizeof(GOLDEN_V4_RECORD));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &store.port, 0, 3,
                                                   64, 0, NULL));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&profile));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_size_t(3, length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&GOLDEN_V4_RECORD[20], out, 3);
}

static void test_v3_golden_is_held_as_a_legacy_image(void)
{
    nd_profile_store_t reopened;
    uint8_t record = 0x66;
    nd_datetime_t time = time_at(1);

    memset(memory, 0xFF, sizeof(memory));
    /* Pre-v4 records start after the two 32 B metadata copies at the base. */
    memcpy(memory + 64, GOLDEN_V3_RECORD, sizeof(GOLDEN_V3_RECORD));
    TEST_ASSERT_EQUAL_INT(ND_PROFILE_STORE_LEGACY_IMAGE,
                          nd_profile_store_init(&reopened, &store.port, 0, 3,
                                                64, 0, &legacy_opts));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&reopened));
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_append(&reopened, &time, 0,
                                                      &record, 1));
}

static void test_reset_mark_matches_the_documented_golden(void)
{
    uint8_t record = 0xAA;
    nd_datetime_t time = time_at(1);

    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_reset(&profile));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_MARK_RECORD, memory + 64,
                                  sizeof(GOLDEN_MARK_RECORD));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_append_read_and_reopen);
    RUN_TEST(test_ring_wraps_and_keeps_newest_records);
    RUN_TEST(test_reset_clears_entries);
    RUN_TEST(test_corrupt_record_is_rejected);
    RUN_TEST(test_recovery_after_unsynced_append);
    RUN_TEST(test_reset_does_not_resurrect_old_records);
    RUN_TEST(test_legacy_metadata_fails_closed_until_reset);
    RUN_TEST(test_legacy_record_fails_closed_until_reset);
    RUN_TEST(test_read_error_during_recovery_fails_init);
    RUN_TEST(test_read_error_mid_scan_fails_init);
    RUN_TEST(test_v4_record_matches_the_documented_golden);
    RUN_TEST(test_literal_v4_golden_loads);
    RUN_TEST(test_v3_golden_is_held_as_a_legacy_image);
    RUN_TEST(test_reset_mark_matches_the_documented_golden);
    return UNITY_END();
}
