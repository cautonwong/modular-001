#include "unity.h"

#include <string.h>

#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#define FLASH_SIZE 8192u
#define SECTOR_SIZE 4096u

static uint8_t flash[FLASH_SIZE];
static uint8_t sector_buffer[SECTOR_SIZE];
static nd_sector_store_t sector_store;
static nd_profile_store_t profile;
static int erase_count;

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
    erase_count++;
    memset(flash + offset, 0xFF, SECTOR_SIZE);
    return 0;
}

static int flash_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    memcpy(flash + offset, buffer, length);
    return 0;
}

void setUp(void)
{
    memset(flash, 0xFF, sizeof(flash));
    erase_count = 0;
    nd_sector_store_init(&sector_store, NULL, flash_read, flash_erase,
                         flash_write, FLASH_SIZE, SECTOR_SIZE,
                         sector_buffer);
    TEST_ASSERT_EQUAL_INT(
        0, nd_profile_store_init(&profile, nd_sector_store_port(&sector_store),
                                 0, 60, 128));
    nd_profile_store_set_sector_slots(&profile, 32);
}

void tearDown(void) {}

static void test_wrap_erases_once_per_slot_not_per_byte(void)
{
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 13, .hour = 12,
    };
    for (uint32_t index = 0; index < 70; index++) {
        uint8_t record = (uint8_t)index;
        time.minute = (uint8_t)(index % 60);
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(
                                     &profile, &time, 0, &record, 1));
    }
    TEST_ASSERT_EQUAL_UINT16(60, nd_profile_store_entries(&profile));
    TEST_ASSERT_LESS_OR_EQUAL_INT(12, erase_count);
    TEST_ASSERT_GREATER_THAN_INT(0, erase_count);

    nd_profile_store_t reopened;
    TEST_ASSERT_EQUAL_INT(0,
                          nd_profile_store_init(
                              &reopened, nd_sector_store_port(&sector_store), 0,
                              60, 128));
    TEST_ASSERT_EQUAL_UINT16(60, nd_profile_store_entries(&reopened));
    uint8_t out[2] = {0};
    size_t length = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 0, out,
                                                   sizeof(out), &length));
    TEST_ASSERT_EQUAL_UINT8(10, out[0]);
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&reopened, 59, out,
                                                   sizeof(out), &length));
    TEST_ASSERT_EQUAL_UINT8(69, out[0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wrap_erases_once_per_slot_not_per_byte);
    return UNITY_END();
}
