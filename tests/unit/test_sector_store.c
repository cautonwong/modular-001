#include "unity.h"

#include <string.h>

#include "infra/memory/sector_store.h"

#define SECTOR_SIZE 16u
#define CAPACITY 64u

static uint8_t memory[CAPACITY];
static uint8_t workspace[SECTOR_SIZE];
static nd_sector_store_t store;
static int erase_count;
static int fail_erase;
static int fail_write;

static int fake_read(void *ctx, uint32_t offset, uint8_t *buffer,
                     size_t length)
{
    (void)ctx;
    memcpy(buffer, memory + offset, length);
    return 0;
}

static int fake_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    if (fail_erase) {
        return -1;
    }
    erase_count++;
    memset(memory + offset, 0xFF, SECTOR_SIZE);
    return 0;
}

static int fake_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                      size_t length)
{
    (void)ctx;
    if (fail_write) {
        return -1;
    }
    memcpy(memory + offset, buffer, length);
    return 0;
}

void setUp(void)
{
    memset(memory, 0xFF, sizeof(memory));
    erase_count = 0;
    fail_erase = 0;
    fail_write = 0;
    nd_sector_store_init(&store, NULL, fake_read, fake_erase, fake_write,
                         CAPACITY, SECTOR_SIZE, workspace);
}

void tearDown(void) {}

static void test_roundtrip_across_sectors(void)
{
    uint8_t pattern[20];
    uint8_t readback[20] = {0};
    for (uint8_t index = 0; index < sizeof(pattern); index++) {
        pattern[index] = (uint8_t)(index + 1u);
    }

    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_sector_store_port(&store), 12,
                                            pattern, sizeof(pattern)));
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(nd_sector_store_port(&store), 12,
                                           readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(pattern, readback, sizeof(pattern));
    TEST_ASSERT_EQUAL_INT(0, erase_count);
}

static void test_overwrite_erases_and_same_value_does_not(void)
{
    uint8_t value[4] = {1, 2, 3, 4};
    uint8_t changed[4] = {5, 6, 7, 8};
    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_sector_store_port(&store), 2,
                                            value, sizeof(value)));
    TEST_ASSERT_EQUAL_INT(0, erase_count);
    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_sector_store_port(&store), 2,
                                            changed, sizeof(changed)));
    TEST_ASSERT_EQUAL_INT(1, erase_count);
    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_sector_store_port(&store), 2,
                                            changed, sizeof(changed)));
    TEST_ASSERT_EQUAL_INT(1, erase_count);
}

static void test_bounds_fail_without_writing(void)
{
    uint8_t value[4] = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL_INT(-1, nd_store_write(nd_sector_store_port(&store), 61,
                                             value, sizeof(value)));
    TEST_ASSERT_EQUAL_INT(-1, nd_store_read(nd_sector_store_port(&store), 61,
                                            value, sizeof(value)));
    TEST_ASSERT_EQUAL_INT(0, erase_count);
}

static void test_backend_failure_propagates(void)
{
    uint8_t value[4] = {1, 2, 3, 4};
    uint8_t changed[4] = {5, 6, 7, 8};
    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_sector_store_port(&store), 0,
                                            value, sizeof(value)));
    fail_erase = 1;
    TEST_ASSERT_EQUAL_INT(-1, nd_store_write(nd_sector_store_port(&store), 0,
                                             changed, sizeof(changed)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip_across_sectors);
    RUN_TEST(test_overwrite_erases_and_same_value_does_not);
    RUN_TEST(test_bounds_fail_without_writing);
    RUN_TEST(test_backend_failure_propagates);
    return UNITY_END();
}
