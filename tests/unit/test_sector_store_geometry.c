#include "unity.h"

#include <string.h>

#include "infra/memory/sector_store.h"

#define CAPACITY 64u
#define SECTOR_BYTES 16u

static uint8_t memory[CAPACITY];
static uint8_t sector_buffer[SECTOR_BYTES];
static nd_sector_store_t store;

void setUp(void) {}

void tearDown(void) {}

static int device_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    memcpy(buffer, memory + offset, length);
    return 0;
}

static int device_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    memset(memory + offset, 0xFF, SECTOR_BYTES);
    return 0;
}

static int device_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    for (size_t index = 0; index < length; index++) {
        memory[offset + index] &= buffer[index];
    }
    return 0;
}

static void init_store(uint32_t capacity, uint32_t sector_size,
                       uint8_t *buffer)
{
    memset(memory, 0xFF, sizeof(memory));
    nd_sector_store_init(&store, NULL, device_read, device_erase,
                         device_write, capacity, sector_size, buffer, 0u);
}

static void assert_store_rejects(void)
{
    const uint8_t source[4] = {1, 2, 3, 4};
    uint8_t dest[4] = {0};
    uint8_t before[CAPACITY];

    memcpy(before, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(
        -1, nd_store_read(nd_sector_store_port(&store), 0u, dest, 1u));
    TEST_ASSERT_EQUAL_INT(
        -1, nd_store_write(nd_sector_store_port(&store), 0u, source, 1u));
    TEST_ASSERT_EQUAL_INT(
        -1, nd_store_erase(nd_sector_store_port(&store), 0u, SECTOR_BYTES));
    TEST_ASSERT_EQUAL_MEMORY(before, memory, sizeof(memory));
}

static void test_accepted_geometry_roundtrip(void)
{
    const uint8_t pattern[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t readback[4] = {0};

    init_store(CAPACITY, SECTOR_BYTES, sector_buffer);
    TEST_ASSERT_EQUAL_INT(
        0, nd_store_write(nd_sector_store_port(&store), 20u, pattern, 4u));
    TEST_ASSERT_EQUAL_INT(
        0, nd_store_read(nd_sector_store_port(&store), 20u, readback, 4u));
    TEST_ASSERT_EQUAL_MEMORY(pattern, readback, 4);
    TEST_ASSERT_EQUAL_INT(
        0, nd_store_erase(nd_sector_store_port(&store), 0u, SECTOR_BYTES));
    TEST_ASSERT_EQUAL_UINT8(0xFF, memory[0]);
    TEST_ASSERT_EQUAL_UINT8(0xDE, memory[20]);
}

static void test_rejects_zero_sector_size(void)
{
    init_store(CAPACITY, 0u, sector_buffer);
    assert_store_rejects();
}

static void test_rejects_non_power_of_two_sector_size(void)
{
    init_store(CAPACITY, 12u, sector_buffer);
    assert_store_rejects();
}

static void test_rejects_partial_last_sector(void)
{
    init_store(CAPACITY - 1u, SECTOR_BYTES, sector_buffer);
    assert_store_rejects();
}

static void test_rejects_missing_sector_buffer(void)
{
    init_store(CAPACITY, SECTOR_BYTES, NULL);
    assert_store_rejects();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_accepted_geometry_roundtrip);
    RUN_TEST(test_rejects_zero_sector_size);
    RUN_TEST(test_rejects_non_power_of_two_sector_size);
    RUN_TEST(test_rejects_partial_last_sector);
    RUN_TEST(test_rejects_missing_sector_buffer);
    return UNITY_END();
}
