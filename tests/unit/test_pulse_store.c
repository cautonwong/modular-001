/* Purpose: the journal's consumer-visible contract before any crash testing.
 * Cases: first boot reports EMPTY and refuses appends, lane split by direction
 * and tariff, tariff segments replayed in order, a partial byte resumed across
 * reboots, FULL until a commit, program granularity trading durability, and
 * argument validation. */
#include "unity.h"

#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/pulse_store.h"
#include "support/pulse_harness.h"

#define JOURNAL_BYTES 16384u
#define BASE_BYTES 1024u
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define SMALL_JOURNAL_BYTES 552u
#define SMALL_MEMORY_BYTES (SMALL_JOURNAL_BYTES + 2u * BASE_BYTES)
#define BASE_SLOT_ONE (JOURNAL_BYTES + BASE_BYTES)

/* Goldens are the layout documented in docs/design/energy-pulse-store.md
 * sections 4.1 and 4.2: little-endian header, blob, CRC-32 (reflected).
 * Version 1's first commit writes base slot 1 and journal generation 1. */
static const uint8_t GOLDEN_BLOB[8] = {0x00, 0x01, 0x02, 0x03,
                                       0x04, 0x05, 0x06, 0x07};
static const uint8_t GOLDEN_BASE[24] = {
    0x42, 0x50, 0x44, 0x4E, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0F, 0xD4, 0xF2, 0xE8,
};
/* The same record with version 0, CRC-32 recomputed over its own 20 bytes
 * (0x469A459E), so magic, length and CRC all pass and only the version gate
 * can reject it. */
static const uint8_t GOLDEN_BASE_V0[24] = {
    0x42, 0x50, 0x44, 0x4E, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x9E, 0x45, 0x9A, 0x46,
};
static const uint8_t GOLDEN_JOURNAL_HEADER[16] = {
    0x4A, 0x50, 0x44, 0x4E, 0x01, 0x00, 0x10, 0x00,
    0x01, 0x00, 0x00, 0x00, 0xF5, 0x5E, 0xFE, 0x2F,
};
static const uint8_t GOLDEN_MARKER[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0A, 0xA1, 0x69, 0xC3,
};
#define GOLDEN_JOURNAL_METADATA_BYTES 544u

static uint8_t memory[MEMORY_BYTES];
static nd_block_ram_t block;
static nd_pulse_store_storage_t storage;

static nd_pulse_store_config_t config_over(nd_block_ram_t *over,
                                           uint32_t journal_bytes,
                                           uint8_t granularity)
{
    nd_pulse_store_config_t config = {
        .block = nd_block_ram_port(over),
        .journal_offset = 0,
        .journal_bytes = journal_bytes,
        .base_offset = {journal_bytes, journal_bytes + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = granularity,
    };
    return config;
}

static void start(uint8_t *data, uint32_t bytes, uint32_t journal_bytes,
                  nd_block_ram_t *over, nd_pulse_store_storage_t *store,
                  uint8_t granularity)
{
    memset(data, 0xFF, bytes);
    nd_block_ram_init(over, data, bytes);
    nd_pulse_store_config_t config =
        config_over(over, journal_bytes, granularity);

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(store, &config));
}

static void reopen(uint32_t journal_bytes, uint8_t granularity)
{
    nd_pulse_store_config_t config =
        config_over(&block, journal_bytes, granularity);

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
}

static void seal(void)
{
    nd_energy_set_t set;

    pulse_set_init(&set);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(nd_pulse_store_port(&storage),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
}

static void append_lane(nd_energy_direction_t direction, uint8_t tariff)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_append(nd_pulse_store_port(&storage),
                              nd_energy_pulse_lane(direction, tariff)));
}

void setUp(void)
{
    start(memory, MEMORY_BYTES, JOURNAL_BYTES, &block, &storage,
          ND_PULSE_PROGRAM_GRANULARITY);
}

void tearDown(void) {}

static void test_first_boot_is_empty_and_needs_a_commit(void)
{
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;
    nd_energy_set_t set;

    TEST_ASSERT_EQUAL_INT(ND_PULSE_EMPTY,
                          nd_pulse_store_base(nd_pulse_store_port(&storage),
                                              buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(ND_PULSE_ERROR,
                          nd_pulse_store_append(nd_pulse_store_port(&storage),
                                                nd_energy_pulse_lane(
                                                    ND_ENERGY_FORWARD, 1)));
    seal();
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_base(nd_pulse_store_port(&storage),
                                              buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(nd_energy_set_t), length);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(0, pulse_total(&set));
}

static void test_lane_split_by_direction_and_tariff(void)
{
    nd_energy_set_t set;

    seal();
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_REVERSE, 1);
    append_lane(ND_ENERGY_REVERSE, 1);
    append_lane(ND_ENERGY_REVERSE, 1);
    append_lane(ND_ENERGY_FORWARD, 2);
    append_lane(ND_ENERGY_FORWARD, 2);
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(2, pulse_slot_count(&set, ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT32(3, pulse_slot_count(&set, ND_ENERGY_REVERSE, 1));
    TEST_ASSERT_EQUAL_UINT32(2, pulse_slot_count(&set, ND_ENERGY_FORWARD, 2));
    TEST_ASSERT_EQUAL_UINT32(0, pulse_slot_count(&set, ND_ENERGY_REVERSE, 2));
    TEST_ASSERT_EQUAL_UINT32(7, pulse_total(&set));
}

static void test_tariff_changes_replay_in_order(void)
{
    nd_energy_set_t set;

    seal();
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 2);
    append_lane(ND_ENERGY_FORWARD, 2);
    append_lane(ND_ENERGY_FORWARD, 2);
    append_lane(ND_ENERGY_FORWARD, 1);
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(3, pulse_slot_count(&set, ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT32(3, pulse_slot_count(&set, ND_ENERGY_FORWARD, 2));
    TEST_ASSERT_EQUAL_UINT32(6, pulse_total(&set));
}

static void test_partial_byte_resumes_across_reboots(void)
{
    nd_energy_set_t set;

    seal();
    for (uint32_t index = 0; index < 5u; index++) {
        append_lane(ND_ENERGY_FORWARD, 1);
    }
    /* Five bits cleared, the upper three still 1. */
    TEST_ASSERT_EQUAL_HEX8(0xE0u, memory[544]);
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    for (uint32_t index = 0; index < 3u; index++) {
        append_lane(ND_ENERGY_FORWARD, 1);
    }
    TEST_ASSERT_EQUAL_HEX8(0x00u, memory[544]);
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    for (uint32_t index = 0; index < 4u; index++) {
        append_lane(ND_ENERGY_FORWARD, 1);
    }
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(12, pulse_slot_count(&set, ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT32(12, pulse_total(&set));
}

static void test_full_returns_full_until_a_commit(void)
{
    static uint8_t small_memory[SMALL_MEMORY_BYTES];
    static nd_block_ram_t small_block;
    static nd_pulse_store_storage_t small_storage;
    nd_energy_set_t set;
    nd_pulse_store_t *store;

    start(small_memory, SMALL_MEMORY_BYTES, SMALL_JOURNAL_BYTES, &small_block,
          &small_storage, ND_PULSE_PROGRAM_GRANULARITY);
    store = nd_pulse_store_port(&small_storage);
    pulse_set_init(&set);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(store, (const uint8_t *)&set,
                                                sizeof(set)));
    for (uint32_t index = 0; index < 32u; index++) {
        TEST_ASSERT_EQUAL_INT(
            ND_PULSE_OK,
            nd_pulse_store_append(store,
                                  nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
        pulse_add(&set, ND_ENERGY_FORWARD, 1);
    }
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_FULL,
        nd_pulse_store_append(store, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(store, (const uint8_t *)&set,
                                                sizeof(set)));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_append(store, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
    pulse_add(&set, ND_ENERGY_FORWARD, 1);
    {
        nd_pulse_store_config_t config =
            config_over(&small_block, SMALL_JOURNAL_BYTES,
                        ND_PULSE_PROGRAM_GRANULARITY);
        TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&small_storage, &config));
    }
    TEST_ASSERT_EQUAL_INT(0,
                          pulse_recover(nd_pulse_store_port(&small_storage),
                                        &set));
    TEST_ASSERT_EQUAL_UINT32(33, pulse_total(&set));
}

static void test_granularity_trades_durability(void)
{
    static nd_pulse_store_storage_t eight;
    nd_energy_set_t set;

    start(memory, MEMORY_BYTES, JOURNAL_BYTES, &block, &eight, 8u);
    pulse_set_init(&set);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(nd_pulse_store_port(&eight),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    for (uint32_t index = 0; index < 7u; index++) {
        TEST_ASSERT_EQUAL_INT(
            ND_PULSE_OK,
            nd_pulse_store_append(nd_pulse_store_port(&eight),
                                  nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
    }
    reopen(JOURNAL_BYTES, 8u);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(0, pulse_total(&set)); /* at most 7 buffered bits lost */
    for (uint32_t index = 0; index < 8u; index++) {
        append_lane(ND_ENERGY_FORWARD, 1);
    }
    reopen(JOURNAL_BYTES, 8u);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(8, pulse_total(&set));
}

static void test_granularity_commit_absorbs_pending_bits(void)
{
    static nd_pulse_store_storage_t eight;
    nd_energy_set_t set;

    start(memory, MEMORY_BYTES, JOURNAL_BYTES, &block, &eight, 8u);
    pulse_set_init(&set);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(nd_pulse_store_port(&eight),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    for (uint32_t index = 0; index < 3u; index++) {
        TEST_ASSERT_EQUAL_INT(
            ND_PULSE_OK,
            nd_pulse_store_append(nd_pulse_store_port(&eight),
                                  nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
        pulse_add(&set, ND_ENERGY_FORWARD, 1);
    }
    /* The blob includes the buffered pulses, so the commit may drop them. */
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(nd_pulse_store_port(&eight),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    reopen(JOURNAL_BYTES, 8u);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(3, pulse_total(&set));
}

static void test_base_reports_required_capacity(void)
{
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    seal();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_FULL,
        nd_pulse_store_base(nd_pulse_store_port(&storage), buffer,
                            sizeof(buffer) - 1u, &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(nd_energy_set_t), length);
}

static void test_lane_encoding_folds_like_the_book(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 0));
    TEST_ASSERT_EQUAL_UINT8(0, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT8(1, nd_energy_pulse_lane(ND_ENERGY_REVERSE, 1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 13));
    TEST_ASSERT_EQUAL_UINT8(22, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 12));
    TEST_ASSERT_EQUAL_UINT8(23, nd_energy_pulse_lane(ND_ENERGY_REVERSE, 12));
    for (uint8_t lane = 0; lane < ND_PULSE_LANES; lane++) {
        nd_energy_direction_t direction =
            nd_energy_pulse_lane_direction(lane);
        uint8_t tariff = nd_energy_pulse_lane_tariff(lane);

        TEST_ASSERT_EQUAL_UINT8(lane,
                                nd_energy_pulse_lane(direction, tariff));
        TEST_ASSERT_TRUE(tariff >= 1u && tariff <= ND_PULSE_TARIFFS);
    }
}

static void test_base_record_matches_the_documented_golden(void)
{
    uint8_t out[sizeof(GOLDEN_BLOB)];
    size_t length = 0;

    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_commit(nd_pulse_store_port(&storage), GOLDEN_BLOB,
                              sizeof(GOLDEN_BLOB)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_BASE, memory + BASE_SLOT_ONE,
                                  sizeof(GOLDEN_BASE));
    for (uint32_t index = sizeof(GOLDEN_BASE); index < BASE_BYTES; index++) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, memory[BASE_SLOT_ONE + index]);
    }

    /* The literal alone must decode after the device forgets how it wrote. */
    memset(memory, 0xFF, sizeof(memory));
    memcpy(memory + BASE_SLOT_ONE, GOLDEN_BASE, sizeof(GOLDEN_BASE));
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_base(nd_pulse_store_port(&storage), out, sizeof(out),
                            &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(GOLDEN_BLOB), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_BLOB, out, sizeof(GOLDEN_BLOB));
}

static void test_base_record_with_an_old_version_is_a_fault(void)
{
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    memcpy(memory + BASE_SLOT_ONE, GOLDEN_BASE_V0, sizeof(GOLDEN_BASE_V0));
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(nd_pulse_store_port(&storage), buffer,
                            sizeof(buffer), &length));
}

static void test_journal_metadata_matches_the_documented_golden(void)
{
    seal();
    append_lane(ND_ENERGY_FORWARD, 1);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_JOURNAL_HEADER, memory,
                                  sizeof(GOLDEN_JOURNAL_HEADER));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_JOURNAL_HEADER, memory + 16,
                                  sizeof(GOLDEN_JOURNAL_HEADER));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_MARKER, memory + 32,
                                  sizeof(GOLDEN_MARKER));
    for (uint32_t index = 48; index < GOLDEN_JOURNAL_METADATA_BYTES; index++) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, memory[index]);
    }
    TEST_ASSERT_EQUAL_HEX8(0xFE, memory[GOLDEN_JOURNAL_METADATA_BYTES]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, memory[GOLDEN_JOURNAL_METADATA_BYTES + 1]);
}

static void test_literal_journal_metadata_replays_one_pulse(void)
{
    nd_energy_set_t set;

    seal();
    memcpy(memory, GOLDEN_JOURNAL_HEADER, sizeof(GOLDEN_JOURNAL_HEADER));
    memcpy(memory + 16, GOLDEN_JOURNAL_HEADER, sizeof(GOLDEN_JOURNAL_HEADER));
    memcpy(memory + 32, GOLDEN_MARKER, sizeof(GOLDEN_MARKER));
    memory[GOLDEN_JOURNAL_METADATA_BYTES] = 0xFE;
    reopen(JOURNAL_BYTES, ND_PULSE_PROGRAM_GRANULARITY);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(1, pulse_slot_count(&set, ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT32(1, pulse_total(&set));
}

static void test_argument_validation(void)
{
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_append(nd_pulse_store_port(&storage), ND_PULSE_LANES));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_append(nd_pulse_store_port(&storage), 0xFFu));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_replay(nd_pulse_store_port(&storage), NULL, NULL));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(nd_pulse_store_port(&storage), NULL, 0, &length));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(nd_pulse_store_port(&storage), buffer,
                            sizeof(buffer), NULL));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_commit(nd_pulse_store_port(&storage), NULL, 0));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_commit(nd_pulse_store_port(&storage), buffer, 0));
    TEST_ASSERT_EQUAL_INT(ND_PULSE_ERROR,
                          nd_pulse_store_append(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_pulse_store_init(NULL, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_first_boot_is_empty_and_needs_a_commit);
    RUN_TEST(test_lane_split_by_direction_and_tariff);
    RUN_TEST(test_tariff_changes_replay_in_order);
    RUN_TEST(test_partial_byte_resumes_across_reboots);
    RUN_TEST(test_full_returns_full_until_a_commit);
    RUN_TEST(test_granularity_trades_durability);
    RUN_TEST(test_granularity_commit_absorbs_pending_bits);
    RUN_TEST(test_base_reports_required_capacity);
    RUN_TEST(test_lane_encoding_folds_like_the_book);
    RUN_TEST(test_base_record_matches_the_documented_golden);
    RUN_TEST(test_base_record_with_an_old_version_is_a_fault);
    RUN_TEST(test_journal_metadata_matches_the_documented_golden);
    RUN_TEST(test_literal_journal_metadata_replays_one_pulse);
    RUN_TEST(test_argument_validation);
    return UNITY_END();
}
