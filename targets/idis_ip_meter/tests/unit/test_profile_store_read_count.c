#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#define SLOT_BYTES 128u

typedef struct {
    nd_store_t port;
    nd_store_t inner;
    unsigned int reads;
    size_t last_length;
} counting_store_t;

static int counting_read(void *ctx, uint32_t offset, uint8_t *buffer,
                         size_t length)
{
    counting_store_t *store = ctx;
    store->reads++;
    store->last_length = length;
    return store->inner.read(store->inner.ctx, offset, buffer, length);
}

static int counting_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                          size_t length)
{
    counting_store_t *store = ctx;
    return store->inner.write(store->inner.ctx, offset, buffer, length);
}

static int counting_erase(void *ctx, uint32_t offset, size_t length)
{
    counting_store_t *store = ctx;
    return store->inner.erase(store->inner.ctx, offset, length);
}

static uint8_t memory[3u * SLOT_BYTES];
static nd_byte_store_t byte_store;
static counting_store_t store;
static nd_profile_store_t profile;

void setUp(void)
{
    memset(memory, 0, sizeof(memory));
    nd_byte_store_init(&byte_store, memory, sizeof(memory));
    store.inner = *nd_byte_store_port(&byte_store);
    store.port = (nd_store_t){.ctx = &store,
                              .read = counting_read,
                              .write = counting_write,
                              .erase = counting_erase};
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(&profile, &store.port, 0, 3,
                                                   SLOT_BYTES, 0, NULL));
    store.reads = 0;
    store.last_length = 0;
}

void tearDown(void) {}

static void append_one(const uint8_t *record, size_t length)
{
    nd_datetime_t time = {.second = 0, .minute = 1, .hour = 8, .day = 11,
                          .month = 11, .year = 2026};
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                     record, length));
}

static void test_read_uses_one_whole_slot_store_read(void)
{
    uint8_t record[] = {0x10, 0x20, 0x30, 0x40};
    uint8_t out[8] = {0};
    size_t length = 0;

    append_one(record, sizeof(record));
    store.reads = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                   &length));
    TEST_ASSERT_EQUAL_UINT(1, store.reads);
    TEST_ASSERT_EQUAL_size_t(SLOT_BYTES, store.last_length);
    TEST_ASSERT_EQUAL_size_t(sizeof(record), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(record, out, sizeof(record));
}

static void test_corrupt_crc_fails_closed_after_one_read(void)
{
    uint8_t record[] = {1, 2, 3};
    uint8_t out[4];
    size_t length = 0;

    append_one(record, sizeof(record));
    memory[20] ^= 0x01;
    store.reads = 0;
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_read(&profile, 0, out, sizeof(out),
                                                    &length));
    TEST_ASSERT_EQUAL_UINT(1, store.reads);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_read_uses_one_whole_slot_store_read);
    RUN_TEST(test_corrupt_crc_fails_closed_after_one_read);
    return UNITY_END();
}
