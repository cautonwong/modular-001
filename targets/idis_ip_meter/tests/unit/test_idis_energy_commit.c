/* Purpose: the journal is durable per accepted pulse, so a periodic 300 s save
 * must never erase it. Commit only when the journal cannot hold the book:
 * FULL, a register clear, an append-error retry, or a power-off with appends
 * since the last commit. A counting block measures the erases of the 16 KiB,
 * four-sector journal. */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/memory/byte_store.h"
#include "infra/storage/block.h"
#include "infra/storage/pulse_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"

#define JOURNAL_BYTES 16384u
#define SECTOR_BYTES 4096u
#define BASE_BYTES 1024u
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define LEGACY_BYTES 2048u
#define SAVES_PER_DAY (24u * 3600u / 300u)
#define JOURNAL_METADATA_BYTES 544u
#define LANE_BITS (((JOURNAL_BYTES - JOURNAL_METADATA_BYTES) / 2u) * 8u)
#define PULSE_RAW_UNITS 5 /* meter constant 2000: one pulse is 5e-4 kWh */

typedef struct {
    nd_block_ops_t ops;
    uint8_t *data;
    uint32_t programs;
    uint32_t erase_calls;
    uint32_t erase_bytes;
} count_block_t;

static uint8_t memory[MEMORY_BYTES];
static uint8_t legacy_memory[LEGACY_BYTES];
static count_block_t block;
static nd_byte_store_t legacy;
static nd_pulse_store_config_t config;
static nd_pulse_store_storage_t storage;
static nd_pulse_store_t *store;
static nd_energy_set_t book;

static int count_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    count_block_t *counts = ctx;

    if (offset > counts->ops.size || length > counts->ops.size - offset) {
        return -1;
    }
    memcpy(buffer, counts->data + offset, length);
    return 0;
}

static int count_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    count_block_t *counts = ctx;

    if (offset > counts->ops.size || length > counts->ops.size - offset) {
        return -1;
    }
    memcpy(counts->data + offset, buffer, length);
    counts->programs++;
    return 0;
}

static int count_erase(void *ctx, uint32_t offset, uint32_t length)
{
    count_block_t *counts = ctx;

    if (offset > counts->ops.size || length > counts->ops.size - offset) {
        return -1;
    }
    memset(counts->data + offset, 0xFF, length);
    counts->erase_calls++;
    counts->erase_bytes += length;
    return 0;
}

static void boot(void)
{
    config = (nd_pulse_store_config_t){
        .block = &block.ops,
        .journal_offset = 0,
        .journal_bytes = JOURNAL_BYTES,
        .base_offset = {JOURNAL_BYTES, JOURNAL_BYTES + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = 1u,
    };
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
    store = nd_pulse_store_port(&storage);
}

static void reboot(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
    store = nd_pulse_store_port(&storage);
}

static int64_t read_import(const nd_energy_set_t *set)
{
    return nd_energy_set_read_raw(set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1);
}

static int64_t reload_import(void)
{
    nd_energy_set_t restored;

    nd_energy_set_init(&restored, 2000u);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &restored, store,
                                 nd_byte_store_port(&legacy), 0));
    return read_import(&restored);
}

void setUp(void)
{
    memset(memory, 0xFF, sizeof(memory));
    memset(legacy_memory, 0x00, sizeof(legacy_memory));
    block = (count_block_t){
        .ops = {
            .ctx = &block,
            .read = count_read,
            .program = count_program,
            .erase = count_erase,
            .size = MEMORY_BYTES,
            .erase_unit = SECTOR_BYTES,
        },
        .data = memory,
    };
    nd_byte_store_init(&legacy, legacy_memory, sizeof(legacy_memory));
    boot();
    nd_energy_set_init(&book, 2000u);
    /* First boot commits generation 1; count only the periodic saves. */
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &book, store, nd_byte_store_port(&legacy), 0));
    block.programs = 0;
    block.erase_calls = 0;
    block.erase_bytes = 0;
}

void tearDown(void) {}

static int save_periodic(void)
{
    return nd_idis_energy_save(&book, store, nd_byte_store_port(&legacy), 0);
}

static int commit_book(void)
{
    return nd_idis_energy_commit(&book, store, nd_byte_store_port(&legacy), 0);
}

static void test_twenty_four_hours_of_saves_without_appends_do_not_erase(void)
{
    for (uint32_t save = 0; save < SAVES_PER_DAY; save++) {
        TEST_ASSERT_EQUAL_INT(0, save_periodic());
    }
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_calls);
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_bytes);
    TEST_ASSERT_EQUAL_UINT32(0, block.programs);
}

static void test_a_day_of_pulses_and_saves_never_erases(void)
{
    for (uint32_t window = 0; window < SAVES_PER_DAY; window++) {
        TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                     &book, store, ND_ENERGY_FORWARD, 1u));
        TEST_ASSERT_EQUAL_INT(0, save_periodic());
    }
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_calls);
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_bytes);

    reboot();
    TEST_ASSERT_EQUAL_INT64((int64_t)SAVES_PER_DAY * PULSE_RAW_UNITS,
                            reload_import());
}

static void test_journal_wrap_commits_exactly_once(void)
{
    for (uint32_t pulse = 0; pulse < LANE_BITS; pulse++) {
        TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                     &book, store, ND_ENERGY_FORWARD, 1u));
    }
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_calls);

    /* The next pulse finds the lane full: one commit, then the retry. */
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                 &book, store, ND_ENERGY_FORWARD, 1u));
    TEST_ASSERT_EQUAL_UINT32(1, block.erase_calls);
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_BYTES, block.erase_bytes);
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_BYTES / SECTOR_BYTES,
                             block.erase_bytes / SECTOR_BYTES);

    reboot();
    TEST_ASSERT_EQUAL_INT64((int64_t)(LANE_BITS + 1u) * PULSE_RAW_UNITS,
                            reload_import());
}

static void test_power_off_with_pending_appends_commits_once(void)
{
    for (uint32_t pulse = 0; pulse < 2u; pulse++) {
        TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                     &book, store, ND_ENERGY_FORWARD, 1u));
        TEST_ASSERT_EQUAL_INT(0, save_periodic());
    }
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_calls);

    TEST_ASSERT_EQUAL_INT(0, commit_book());
    TEST_ASSERT_EQUAL_UINT32(1, block.erase_calls);
    TEST_ASSERT_EQUAL_UINT32(JOURNAL_BYTES, block.erase_bytes);

    reboot();
    TEST_ASSERT_EQUAL_INT64(2 * PULSE_RAW_UNITS, reload_import());
}

static void test_register_clear_commits_even_when_the_book_equals_the_base(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                 &book, store, ND_ENERGY_FORWARD, 1u));
    TEST_ASSERT_EQUAL_INT(0, save_periodic());
    TEST_ASSERT_EQUAL_UINT32(0, block.erase_calls);

    /* The base is zero, so the post-reset book equals it while the journal
     * still holds the pulse. The commit must reset the journal anyway. */
    nd_energy_set_reset(&book, ND_ENERGY_CHANNEL_ACTIVE_IMPORT);
    TEST_ASSERT_EQUAL_INT(0, commit_book());
    TEST_ASSERT_EQUAL_UINT32(1, block.erase_calls);

    reboot();
    TEST_ASSERT_EQUAL_INT64(0, reload_import());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_twenty_four_hours_of_saves_without_appends_do_not_erase);
    RUN_TEST(test_a_day_of_pulses_and_saves_never_erases);
    RUN_TEST(test_journal_wrap_commits_exactly_once);
    RUN_TEST(test_power_off_with_pending_appends_commits_once);
    RUN_TEST(test_register_clear_commits_even_when_the_book_equals_the_base);
    return UNITY_END();
}
