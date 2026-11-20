/*
 * End-to-end sector window test through a profile GET.
 *
 * `targets/host/idis_host_main.c` binds the IDIS flash to a sector store with
 * the read-through window enabled. This test composes the same stores over a
 * byte backing, fills 128 records (four 4 KiB sectors), and reads the whole
 * profile with the window off and on. The two responses must be byte
 * identical, and the backing reads must drop from one per slot to one per
 * touched sector.
 */
#include "unity.h"

#include <stdio.h>
#include <string.h>

#include "infra/memory/byte_store.h"
#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_capture.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define SLOT_BYTES 128u
#define SECTOR_BYTES 4096u
#define SLOT_COUNT 128u
#define SECTOR_SLOTS (SECTOR_BYTES / SLOT_BYTES)
#define SECTOR_COUNT (SLOT_COUNT / SECTOR_SLOTS)
/* The ring rounds up to whole sectors and keeps one spare sector. */
#define FLASH_SLOTS (((SLOT_COUNT + SECTOR_SLOTS - 1u) / SECTOR_SLOTS) \
                         * SECTOR_SLOTS \
                     + SECTOR_SLOTS)
#define FLASH_BYTES (FLASH_SLOTS * SLOT_BYTES)

static uint8_t flash[FLASH_BYTES];
static uint8_t sector_off_buffer[SECTOR_BYTES];
static uint8_t sector_on_buffer[SECTOR_BYTES];
static nd_byte_store_t byte_store;

typedef struct {
    nd_store_t port;
    nd_store_t inner;
    unsigned int reads;
} counting_store_t;

static counting_store_t counting;

static int counting_read(void *ctx, uint32_t offset, uint8_t *buffer,
                         size_t length)
{
    counting_store_t *store = ctx;
    store->reads++;
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

static int sector_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    return nd_store_read(ctx, offset, buffer, length);
}

static int sector_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    return nd_store_write(ctx, offset, buffer, length);
}

static int sector_erase(void *ctx, uint32_t offset)
{
    return nd_store_erase(ctx, offset, SECTOR_BYTES);
}

static nd_sector_store_t sector_off;
static nd_sector_store_t sector_on;

static nd_dlms_datetime_t fake_clock_now;
static nd_idis_objects_t objects_off;
static nd_idis_objects_t objects_on;

static int fake_clock_read(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = fake_clock_now;
    return 0;
}

static void init_store(nd_sector_store_t *store, uint8_t *buffer,
                       uint8_t enable_window)
{
    nd_sector_store_init(store, &counting.port, sector_read, sector_erase,
                         sector_write, FLASH_BYTES, SECTOR_BYTES, buffer,
                         enable_window);
}

static void init_profile(nd_idis_objects_t *objects,
                         nd_sector_store_t *store)
{
    static const nd_clock_port_t clock_port = {
        .ctx = NULL,
        .read = fake_clock_read,
    };
    const nd_capture_object_t captures[] = {
        {ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0},
    };
    memset(objects, 0, sizeof(*objects));
    objects->clock.port = &clock_port;
    nd_idis_profile_config_t config = {
        .ln = ND_OBIS_LOAD_PROFILE_1,
        .store = nd_sector_store_port(store),
        .base_offset = 0,
        .capacity = SLOT_COUNT,
        .slot_size = SLOT_BYTES,
        .clock = &clock_port,
        .capture_count = 1,
        .capture_period = 60,
    };
    config.captures[0] = captures[0];
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects->profiles, &config, 1, 0, NULL));
    nd_cosem_port_t port = {
        .ctx = objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects->profiles, &port);
}

static void append_slots(void)
{
    nd_profile_store_t *store = &objects_off.profiles.items[0].store;
    nd_datetime_t time = {
        .year = 2026, .month = 11, .day = 19, .hour = 10,
    };
    for (uint32_t slot = 0; slot < SLOT_COUNT; slot++) {
        uint8_t record[] = {ND_AXDR_UNSIGNED, (uint8_t)slot};
        time.second = (uint8_t)(slot % 60u);
        TEST_ASSERT_EQUAL_INT(0,
                              nd_profile_store_append(store, &time, 0,
                                                      record,
                                                      sizeof(record)));
    }
    TEST_ASSERT_EQUAL_UINT16(SLOT_COUNT, nd_profile_store_entries(store));
}

static size_t read_profile(nd_idis_objects_t *objects, uint8_t *buffer,
                           size_t capacity)
{
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, capacity);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(objects, 16,
                                            ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_LOAD_PROFILE_1, 2, NULL,
                                            &out));
    return out.len;
}

void setUp(void)
{
    memset(flash, 0xFF, sizeof(flash));
    fake_clock_now = (nd_dlms_datetime_t){
        .year = 2026, .month = 11, .day = 19, .hour = 10, .minute = 0,
        .second = 0,
    };
    nd_byte_store_init(&byte_store, flash, sizeof(flash));
    counting.inner = *nd_byte_store_port(&byte_store);
    counting.port = (nd_store_t){
        .ctx = &counting,
        .read = counting_read,
        .write = counting_write,
        .erase = counting_erase,
    };
    counting.reads = 0;

    init_store(&sector_off, sector_off_buffer, 0);
    init_profile(&objects_off, &sector_off);
    append_slots();

    init_store(&sector_on, sector_on_buffer, 1);
    init_profile(&objects_on, &sector_on);
    /* Profile init leaves the window holding the metadata sector; re-init the
     * store so the GET starts cold and pays every touched sector. */
    init_store(&sector_on, sector_on_buffer, 1);
    counting.reads = 0;
}

void tearDown(void) {}

static void test_window_get_matches_and_reads_one_per_sector(void)
{
    static uint8_t response_off[SECTOR_BYTES];
    static uint8_t response_on[SECTOR_BYTES];

    counting.reads = 0;
    size_t length_off = read_profile(&objects_off, response_off,
                                     sizeof(response_off));
    unsigned int reads_off = counting.reads;

    counting.reads = 0;
    size_t length_on = read_profile(&objects_on, response_on,
                                    sizeof(response_on));
    unsigned int reads_on = counting.reads;

    printf("profile GET: %u reads with window off, %u with window on\n",
           reads_off, reads_on);
    TEST_ASSERT_EQUAL_size_t(length_off, length_on);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(response_off, response_on, length_off);
    TEST_ASSERT_EQUAL_UINT(SLOT_COUNT, reads_off);
    TEST_ASSERT_EQUAL_UINT(SECTOR_COUNT, reads_on);
    TEST_ASSERT_TRUE(reads_on < reads_off);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_window_get_matches_and_reads_one_per_sector);
    return UNITY_END();
}
