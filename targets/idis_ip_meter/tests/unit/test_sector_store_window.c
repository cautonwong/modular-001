#include "unity.h"

#include <stdio.h>
#include <string.h>

#include "infra/memory/sector_store.h"

#define SLOT_BYTES 128u
#define SECTOR_BYTES 4096u
#define SLOT_COUNT 1152u
#define CAPACITY (SLOT_COUNT * SLOT_BYTES)
#define SECTORS (CAPACITY / SECTOR_BYTES)

static uint8_t flash[CAPACITY];
static uint8_t sector_buffer[SECTOR_BYTES];

static unsigned int backing_reads;
static size_t backing_length;

static int memory_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    backing_reads++;
    backing_length = length;
    memcpy(buffer, flash + offset, length);
    return 0;
}

static int memory_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    memset(flash + offset, 0xFF, SECTOR_BYTES);
    return 0;
}

static int memory_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    memcpy(flash + offset, buffer, length);
    return 0;
}

typedef struct {
    nd_store_t port;
    const nd_store_t *inner;
    unsigned int reads;
} counting_store_t;

static int counting_read(void *ctx, uint32_t offset, uint8_t *buffer,
                         size_t length)
{
    counting_store_t *store = ctx;
    store->reads++;
    return store->inner->read(store->inner->ctx, offset, buffer, length);
}

static int counting_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                          size_t length)
{
    counting_store_t *store = ctx;
    return store->inner->write(store->inner->ctx, offset, buffer, length);
}

static int counting_erase(void *ctx, uint32_t offset, size_t length)
{
    counting_store_t *store = ctx;
    return store->inner->erase(store->inner->ctx, offset, length);
}

static nd_sector_store_t sector_store;
static counting_store_t store;

static void init_store(uint8_t enable_window)
{
    memset(&store, 0, sizeof(store));
    nd_sector_store_init(&sector_store, NULL, memory_read, memory_erase,
                         memory_write, CAPACITY, SECTOR_BYTES, sector_buffer,
                         enable_window);
    store.inner = nd_sector_store_port(&sector_store);
    store.port = (nd_store_t){
        .ctx = &store,
        .read = counting_read,
        .write = counting_write,
        .erase = counting_erase,
    };
}

void setUp(void)
{
    for (uint32_t index = 0; index < CAPACITY; index++) {
        flash[index] = (uint8_t)(index * 31u + 7u);
    }
    backing_reads = 0;
    backing_length = 0;
}

void tearDown(void) {}

static void test_full_scan_fills_once_per_sector(void)
{
    uint8_t slot[SLOT_BYTES];
    init_store(1);

    for (uint32_t index = 0; index < SLOT_COUNT; index++) {
        uint32_t offset = index * SLOT_BYTES;
        TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, offset, slot,
                                               sizeof(slot)));
        TEST_ASSERT_EQUAL_UINT8(flash[offset], slot[0]);
        TEST_ASSERT_EQUAL_UINT8(flash[offset + SLOT_BYTES - 1u],
                                slot[SLOT_BYTES - 1u]);
    }

    TEST_ASSERT_EQUAL_UINT(SLOT_COUNT, store.reads);
    TEST_ASSERT_EQUAL_UINT(SECTORS, backing_reads);
    TEST_ASSERT_EQUAL_size_t(SECTOR_BYTES, backing_length);
    printf("full scan: %u slot reads, %u sector reads of %u bytes\n",
           store.reads, backing_reads, (unsigned int)backing_length);
}

static void test_one_record_read_is_one_fill(void)
{
    uint8_t slot[8];
    uint32_t offset = 700u * SLOT_BYTES;
    init_store(1);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, offset, slot,
                                           sizeof(slot)));

    TEST_ASSERT_EQUAL_UINT(1, store.reads);
    TEST_ASSERT_EQUAL_UINT(1, backing_reads);
    TEST_ASSERT_EQUAL_size_t(SECTOR_BYTES, backing_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(flash + offset, slot, sizeof(slot));
}

static void test_write_invalidates_window(void)
{
    uint8_t slot[SLOT_BYTES];
    uint8_t patch[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint32_t offset = 3u * SLOT_BYTES;
    init_store(1);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, offset, slot,
                                           sizeof(slot)));
    TEST_ASSERT_EQUAL_UINT(1, backing_reads);

    TEST_ASSERT_EQUAL_INT(0, nd_store_write(&store.port, offset, patch,
                                            sizeof(patch)));
    TEST_ASSERT_EQUAL_UINT(2, backing_reads);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, offset, slot,
                                           sizeof(slot)));
    TEST_ASSERT_EQUAL_UINT(3, backing_reads);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(patch, slot, sizeof(patch));
}

static void test_cross_sector_read_bypasses(void)
{
    uint8_t buffer[SLOT_BYTES];
    uint8_t large[SECTOR_BYTES + 1u];
    uint32_t crossing = SECTOR_BYTES - 64u;
    init_store(1);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, 0, buffer, 8));
    TEST_ASSERT_EQUAL_UINT(1, backing_reads);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, crossing, buffer,
                                           sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT(2, backing_reads);
    TEST_ASSERT_EQUAL_size_t(sizeof(buffer), backing_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(flash + crossing, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, SLOT_BYTES, buffer, 8));
    TEST_ASSERT_EQUAL_UINT(2, backing_reads);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, 0, large,
                                           sizeof(large)));
    TEST_ASSERT_EQUAL_UINT(3, backing_reads);
    TEST_ASSERT_EQUAL_size_t(sizeof(large), backing_length);
}

static void test_disabled_window_reads_through(void)
{
    uint8_t buffer[8];
    init_store(0);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, 0, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(&store.port, 8, buffer, sizeof(buffer)));

    TEST_ASSERT_EQUAL_UINT(2, backing_reads);
    TEST_ASSERT_EQUAL_size_t(sizeof(buffer), backing_length);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_full_scan_fills_once_per_sector);
    RUN_TEST(test_one_record_read_is_one_fill);
    RUN_TEST(test_write_invalidates_window);
    RUN_TEST(test_cross_sector_read_bypasses);
    RUN_TEST(test_disabled_window_reads_through);
    return UNITY_END();
}
