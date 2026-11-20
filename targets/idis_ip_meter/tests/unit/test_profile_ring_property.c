#include "unity.h"

#include <stdio.h>
#include <string.h>

#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#define SECTOR_SIZE 4096u
#define SLOT_SIZE 128u
#define SECTOR_SLOTS (SECTOR_SIZE / SLOT_SIZE)
/* The 1152-record PQ ring plus its spare sector, sized in slots. */
#define FLASH_SLOTS 1184u
#define FLASH_SIZE (FLASH_SLOTS * SLOT_SIZE)

static uint8_t flash[FLASH_SIZE];
static uint8_t sector_buffer[SECTOR_SIZE];
static nd_sector_store_t sector_store;
static nd_profile_store_t profile;

/* The append pre-erase must never touch the live window. The test owns the
 * expected window so the fake erase can prove it. */
static uint16_t live_head;
static uint16_t live_entries;
static uint16_t phys;
static int erase_count;
static int erase_violations;

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
    uint32_t first = offset / SLOT_SIZE;
    erase_count++;
    for (uint32_t slot = first; slot < first + SECTOR_SLOTS; slot++) {
        for (uint16_t live = 0; live < live_entries; live++) {
            uint16_t window = (uint16_t)((live_head + phys - live_entries
                                          + live)
                                         % phys);
            if (window == slot) {
                erase_violations++;
            }
        }
    }
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

void setUp(void) {}

void tearDown(void) {}

static uint16_t expected_entries(uint16_t capacity, uint32_t appends)
{
    return appends < capacity ? (uint16_t)appends : capacity;
}

static void check_contents(uint16_t capacity, uint32_t appends)
{
    uint16_t entries = nd_profile_store_entries(&profile);
    TEST_ASSERT_EQUAL_UINT16(expected_entries(capacity, appends), entries);
    for (uint16_t index = 0; index < entries; index++) {
        uint8_t out = 0;
        size_t length = 0;
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_read(&profile, index, &out,
                                                       sizeof(out), &length));
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(appends - entries + 1u + index), out);
    }
}

static void check_large_contents(uint16_t capacity, uint32_t appends)
{
    uint16_t entries = nd_profile_store_entries(&profile);
    TEST_ASSERT_EQUAL_UINT16(expected_entries(capacity, appends), entries);
    for (uint16_t index = 0; index < entries; index++) {
        /* Two bytes so a 256-slot misalignment cannot alias to a pass. */
        uint8_t out[2] = {0};
        size_t length = 0;
        int status = nd_profile_store_read(&profile, index, out, sizeof(out),
                                           &length);
        uint16_t expected = (uint16_t)(appends - entries + index);
        uint16_t actual = (uint16_t)(out[0] | ((uint16_t)out[1] << 8));
        if (status != 0 || length != sizeof(out) || actual != expected) {
            char message[160];
            snprintf(message, sizeof(message),
                     "index %u: expected %u, got %u; read status %d, "
                     "length %u, erase_count %d, appends %u",
                     (unsigned)index, (unsigned)expected, (unsigned)actual,
                     status, (unsigned)length, erase_count,
                     (unsigned)appends);
            TEST_FAIL_MESSAGE(message);
        }
    }
}

static void test_erase_target_never_holds_a_live_record(void)
{
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 13, .hour = 12,
    };
    for (uint16_t capacity = 1; capacity <= 80; capacity++) {
        memset(flash, 0xFF, sizeof(flash));
        erase_count = 0;
        erase_violations = 0;
        nd_sector_store_init(&sector_store, NULL, flash_read, flash_erase,
                             flash_write, sizeof(flash), SECTOR_SIZE,
                             sector_buffer, 0);
        TEST_ASSERT_EQUAL_INT(
            0, nd_profile_store_init(&profile,
                                     nd_sector_store_port(&sector_store), 0,
                                     capacity, SLOT_SIZE, SECTOR_SIZE, NULL));
        phys = (uint16_t)(((capacity + SECTOR_SLOTS - 1u) / SECTOR_SLOTS)
                              * SECTOR_SLOTS
                          + SECTOR_SLOTS);

        uint32_t appends = 3u * capacity + 40u;
        for (uint32_t count = 1; count <= appends; count++) {
            live_head = profile.head;
            live_entries = profile.entries;
            uint8_t value = (uint8_t)count;
            TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(
                                         &profile, &time, 0, &value, 1));
            TEST_ASSERT_EQUAL_INT(0, erase_violations);
            check_contents(capacity, count);
        }
        TEST_ASSERT_EQUAL_INT((appends + SECTOR_SLOTS - 1u) / SECTOR_SLOTS,
                              erase_count);
    }
}

static void test_large_profile_thousand_appends(void)
{
    enum { CAPACITY = 1152, APPENDS = CAPACITY + 1000 };
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 19, .hour = 10,
    };
    memset(flash, 0xFF, sizeof(flash));
    erase_count = 0;
    erase_violations = 0;
    nd_sector_store_init(&sector_store, NULL, flash_read, flash_erase,
                         flash_write, sizeof(flash), SECTOR_SIZE,
                         sector_buffer, 0);
    TEST_ASSERT_EQUAL_INT(
        0, nd_profile_store_init(&profile, nd_sector_store_port(&sector_store),
                                 0, CAPACITY, SLOT_SIZE, SECTOR_SIZE, NULL));
    phys = (uint16_t)(((CAPACITY + SECTOR_SLOTS - 1u) / SECTOR_SLOTS)
                          * SECTOR_SLOTS
                      + SECTOR_SLOTS);

    for (uint32_t count = 1; count <= APPENDS; count++) {
        live_head = profile.head;
        live_entries = profile.entries;
        uint16_t number = (uint16_t)(count - 1u);
        uint8_t value[2] = {(uint8_t)number, (uint8_t)(number >> 8)};
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&profile, &time, 0,
                                                         value,
                                                         sizeof(value)));
        TEST_ASSERT_EQUAL_INT(0, erase_violations);
        if (count % SECTOR_SLOTS == 0u) {
            check_large_contents(CAPACITY, count);
        }
    }
    check_large_contents(CAPACITY, APPENDS);
    TEST_ASSERT_EQUAL_INT((APPENDS + SECTOR_SLOTS - 1u) / SECTOR_SLOTS,
                          erase_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_erase_target_never_holds_a_live_record);
    RUN_TEST(test_large_profile_thousand_appends);
    return UNITY_END();
}
