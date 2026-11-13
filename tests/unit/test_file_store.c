#include "unity.h"

#include <stdio.h>
#include <unistd.h>

#include "infra/memory/file_store.h"

static char path[64];
static nd_file_store_t store;

void setUp(void)
{
    snprintf(path, sizeof(path), "/tmp/nd_file_store_%d.bin", (int)getpid());
    remove(path);
}

void tearDown(void)
{
    nd_file_store_close(&store);
    remove(path);
}

static void test_write_reopen_read(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_file_store_open(&store, path, 64));
    const uint8_t data[] = {1, 2, 3, 4};
    TEST_ASSERT_EQUAL_INT(0, nd_store_write(nd_file_store_port(&store), 8, data,
                                            sizeof(data)));
    TEST_ASSERT_EQUAL_INT(0, nd_file_store_close(&store));

    TEST_ASSERT_EQUAL_INT(0, nd_file_store_open(&store, path, 64));
    uint8_t readback[4] = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(nd_file_store_port(&store), 8,
                                           readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, readback, sizeof(data));
}

static void test_bounds_are_rejected(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_file_store_open(&store, path, 16));
    uint8_t byte = 0;
    TEST_ASSERT_EQUAL_INT(-1, nd_store_read(nd_file_store_port(&store), 16,
                                            &byte, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_store_write(nd_file_store_port(&store), 15,
                                             &byte, 2));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_write_reopen_read);
    RUN_TEST(test_bounds_are_rejected);
    return UNITY_END();
}
