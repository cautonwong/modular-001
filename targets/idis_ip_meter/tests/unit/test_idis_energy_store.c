#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/memory/byte_store.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/pulse_snapshot.h"
#include "infra/storage/pulse_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"

#define JOURNAL_BYTES 4096u
#define FULL_JOURNAL_BYTES 546u /* one byte of bits per direction */
#define BASE_BYTES 1024u
#define LEGACY_BYTES 2048u

static uint8_t pulse_memory[JOURNAL_BYTES + 2u * BASE_BYTES];
static uint8_t legacy_memory[LEGACY_BYTES];
static nd_block_ram_t pulse_block;
static nd_byte_store_t legacy;
static nd_pulse_store_storage_t storage;

void setUp(void)
{
    memset(pulse_memory, 0xFF, sizeof(pulse_memory));
    memset(legacy_memory, 0x00, sizeof(legacy_memory));
    nd_block_ram_init(&pulse_block, pulse_memory, sizeof(pulse_memory));
    nd_byte_store_init(&legacy, legacy_memory, sizeof(legacy_memory));
}

void tearDown(void) {}

static nd_pulse_store_config_t journal_config(uint32_t bytes)
{
    nd_pulse_store_config_t config = {
        .block = nd_block_ram_port(&pulse_block),
        .journal_offset = 0,
        .journal_bytes = bytes,
        .base_offset = {bytes, bytes + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = 1u,
    };

    return config;
}

static nd_pulse_store_t *start_journal(uint32_t bytes)
{
    nd_pulse_store_config_t config = journal_config(bytes);

    return nd_pulse_store_init(&storage, &config) == 0
               ? nd_pulse_store_port(&storage)
               : NULL;
}

static void seed_set(nd_energy_set_t *set)
{
    nd_energy_set_init(set, 2000u);
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1,
                                 4000u));
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 set, ND_ENERGY_CHANNEL_REACTIVE_Q2, 2,
                                 2000u));
}

static void test_base_round_trip(void)
{
    nd_pulse_store_t *store = start_journal(JOURNAL_BYTES);
    TEST_ASSERT_NOT_NULL(store);
    nd_energy_set_t set;
    seed_set(&set);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_commit(
                                 &set, store, nd_byte_store_port(&legacy), 0));

    nd_energy_set_t restored;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &restored, store, nd_byte_store_port(&legacy),
                                 0));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read_raw(
                                       &restored,
                                       ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1));
    TEST_ASSERT_EQUAL_INT64(10000, nd_energy_set_read_raw(
                                       &restored,
                                       ND_ENERGY_CHANNEL_REACTIVE_Q2, 2));
    TEST_ASSERT_EQUAL_UINT32(2000, restored.meter_const);
}

static void test_legacy_record_becomes_generation_one(void)
{
    nd_energy_set_t legacy_set;
    seed_set(&legacy_set);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_save_legacy(
                                 &legacy_set, nd_byte_store_port(&legacy), 0));

    nd_pulse_store_t *store = start_journal(JOURNAL_BYTES);
    TEST_ASSERT_NOT_NULL(store);
    nd_energy_set_t loaded = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &loaded, store, nd_byte_store_port(&legacy),
                                 0));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read_raw(
                                       &loaded,
                                       ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1));

    uint8_t blob[sizeof(nd_energy_set_t)];
    size_t length = 0;
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_base(store, blob, sizeof(blob),
                                              &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(nd_energy_set_t), length);

    nd_energy_set_t kept;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load_legacy(
                                 &kept, nd_byte_store_port(&legacy), 0));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read_raw(
                                       &kept, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                       1));
}

static void test_pulses_replay_after_restart(void)
{
    nd_pulse_store_t *store = start_journal(JOURNAL_BYTES);
    TEST_ASSERT_NOT_NULL(store);
    nd_energy_set_t set;
    nd_energy_set_init(&set, 2000u);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &set, store, nd_byte_store_port(&legacy), 0));
    for (int pulse = 0; pulse < 5; pulse++) {
        TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                     &set, store, ND_ENERGY_FORWARD, 2u));
    }
    TEST_ASSERT_EQUAL_INT64(25, nd_energy_set_read_raw(
                                    &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 2));

    nd_energy_set_t restored;
    nd_energy_set_init(&restored, 2000u);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &restored, store,
                                 nd_byte_store_port(&legacy), 0));
    TEST_ASSERT_EQUAL_INT64(25, nd_energy_set_read_raw(
                                    &restored, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                    2));
    TEST_ASSERT_EQUAL_INT64(25, nd_energy_set_read_raw(
                                    &restored, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                    ND_TARIFF_TOTAL));
}

static void test_full_commits_then_retries_the_pulse(void)
{
    nd_pulse_store_t *store = start_journal(FULL_JOURNAL_BYTES);
    TEST_ASSERT_NOT_NULL(store);
    nd_energy_set_t set;
    nd_energy_set_init(&set, 2000u);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &set, store, nd_byte_store_port(&legacy), 0));
    for (int pulse = 0; pulse < 9; pulse++) {
        TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_pulse(
                                     &set, store, ND_ENERGY_FORWARD, 1u));
    }

    nd_energy_set_t restored;
    nd_energy_set_init(&restored, 2000u);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &restored, store,
                                 nd_byte_store_port(&legacy), 0));
    /* Eight pulses reached the generation 1 base; the ninth was retried
     * into the reset journal. No pulse is lost or counted twice. */
    TEST_ASSERT_EQUAL_INT64(45, nd_energy_set_read_raw(
                                    &restored, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                    1));
}

static void test_legacy_without_crc_is_not_silently_accepted(void)
{
    nd_energy_set_t legacy_set;
    seed_set(&legacy_set);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_save_legacy(
                                 &legacy_set, nd_byte_store_port(&legacy), 0));

    static uint8_t snapshot_memory[LEGACY_BYTES];
    static nd_block_ram_t snapshot_block;
    static nd_pulse_snapshot_storage_t snapshot;
    memset(snapshot_memory, 0x00, sizeof(snapshot_memory));
    memcpy(snapshot_memory, legacy_memory,
           sizeof(uint32_t) + sizeof(nd_energy_set_t));
    nd_block_ram_init(&snapshot_block, snapshot_memory, sizeof(snapshot_memory));
    nd_pulse_snapshot_config_t config = {
        .block = nd_block_ram_port(&snapshot_block),
        .offset = 0,
        .blob_bytes = sizeof(nd_energy_set_t),
    };
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&snapshot, &config));
    nd_pulse_store_t *store = nd_pulse_snapshot_port(&snapshot);

    uint8_t blob[sizeof(nd_energy_set_t)];
    size_t length = 0;
    TEST_ASSERT_EQUAL_INT(ND_PULSE_ERROR,
                          nd_pulse_store_base(store, blob, sizeof(blob),
                                              &length));
    nd_energy_set_t rejected;
    nd_energy_set_init(&rejected, 2000u);
    TEST_ASSERT_EQUAL_INT(-1,
                          nd_idis_energy_load(&rejected, store,
                                              nd_byte_store_port(&legacy), 0));

    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_adopt(
                                 &rejected, store, nd_byte_store_port(&legacy),
                                 0));
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_base(store, blob, sizeof(blob),
                                              &length));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &rejected, store,
                                 nd_byte_store_port(&legacy), 0));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read_raw(
                                       &rejected,
                                       ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_base_round_trip);
    RUN_TEST(test_legacy_record_becomes_generation_one);
    RUN_TEST(test_pulses_replay_after_restart);
    RUN_TEST(test_full_commits_then_retries_the_pulse);
    RUN_TEST(test_legacy_without_crc_is_not_silently_accepted);
    return UNITY_END();
}
