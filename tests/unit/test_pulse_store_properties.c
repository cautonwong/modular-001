/* Purpose: exhaustive implementation checks for the two engine properties the
 * TLA+ model formal/tla/EnergyPulse.tla states, over the bounded spaces below.
 * 1. The unary lane's recovery counts exactly the cleared prefix for every
 *    byte value, so the committed bits are contiguous and no hole counts.
 * 2. A single-bit corruption of any marker byte never changes the replay
 *    total and never double counts: the CRC skip merges segments instead of
 *    dropping or repeating pulses.
 * Inputs are enumerated, not sampled, so the loop bodies assert invariants
 * only. */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/pulse_store.h"
#include "support/pulse_harness.h"

#define JOURNAL_BYTES 560u
#define BASE_BYTES 1024u
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define LANE_BITS ((JOURNAL_BYTES - 544u) / 2u * 8u)

/* The first test pokes the first two bytes of lane 0 after the 544-byte
 * metadata block. */
_Static_assert(LANE_BITS == 64u, "lane 0 must start at journal offset 544");

#define MARKERS_JOURNAL_BYTES 16384u
#define MARKERS_MEMORY_BYTES (MARKERS_JOURNAL_BYTES + 2u * BASE_BYTES)
#define MARKER_ZERO 32u
#define MARKER_BYTES 16u
#define MARKER_SLOTS 3u

static uint8_t memory[MEMORY_BYTES > MARKERS_MEMORY_BYTES ? MEMORY_BYTES
                                                          : MARKERS_MEMORY_BYTES];
static nd_block_ram_t block;
static nd_pulse_store_storage_t storage;

static nd_pulse_store_config_t config_over(uint32_t journal_bytes)
{
    nd_pulse_store_config_t config = {
        .block = nd_block_ram_port(&block),
        .journal_offset = 0,
        .journal_bytes = journal_bytes,
        .base_offset = {journal_bytes, journal_bytes + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = ND_PULSE_PROGRAM_GRANULARITY,
    };
    return config;
}

static void start(uint32_t journal_bytes)
{
    nd_pulse_store_config_t config;

    memset(memory, 0xFF, sizeof(memory));
    nd_block_ram_init(&block, memory, sizeof(memory));
    config = config_over(journal_bytes);
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
}

static void reopen(uint32_t journal_bytes)
{
    nd_pulse_store_config_t config = config_over(journal_bytes);

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

/* Same rule as scan_bits: 0x00 clears a whole byte and scanning continues,
 * any other value stops at the first set bit. */
static uint32_t cleared_prefix(uint8_t value)
{
    uint32_t count = 0;

    if (value == 0x00u) {
        return 8u;
    }
    while ((value & 1u) == 0u) {
        count++;
        value = (uint8_t)(value >> 1);
    }
    return count;
}

static uint32_t recover_total(uint32_t journal_bytes, nd_energy_set_t *set)
{
    reopen(journal_bytes);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage),
                                           set));
    return pulse_total(set);
}

static void test_every_lane_byte_recovers_its_cleared_prefix(void)
{
    nd_energy_set_t recovered;

    start(JOURNAL_BYTES);
    seal();
    for (uint32_t value = 0; value <= 0xFFu; value++) {
        memory[544] = (uint8_t)value;
        memory[545] = 0xFFu;
        TEST_ASSERT_EQUAL_UINT32(cleared_prefix((uint8_t)value),
                                 recover_total(JOURNAL_BYTES, &recovered));
    }
    memory[544] = 0x00u;
    for (uint32_t value = 0; value <= 0xFFu; value++) {
        memory[545] = (uint8_t)value;
        TEST_ASSERT_EQUAL_UINT32(8u + cleared_prefix((uint8_t)value),
                                 recover_total(JOURNAL_BYTES, &recovered));
    }
    memory[544] = 0xFFu;
    memory[545] = 0xFFu;
    TEST_ASSERT_EQUAL_UINT32(0u, recover_total(JOURNAL_BYTES, &recovered));
}

static void append_and_seal_tariff_slots(void)
{
    static const uint8_t tariffs[MARKER_SLOTS] = {1u, 2u, 3u};

    start(MARKERS_JOURNAL_BYTES);
    seal();
    for (uint32_t slot = 0; slot < MARKER_SLOTS; slot++) {
        for (uint32_t pulse = 0; pulse < 2u; pulse++) {
            TEST_ASSERT_EQUAL_INT(
                ND_PULSE_OK,
                nd_pulse_store_append(nd_pulse_store_port(&storage),
                                      nd_energy_pulse_lane(
                                          ND_ENERGY_FORWARD, tariffs[slot])));
        }
    }
}

static void test_every_marker_bit_flip_keeps_replay_whole(void)
{
    nd_energy_set_t recovered;
    uint8_t saved[MARKER_SLOTS * MARKER_BYTES];

    append_and_seal_tariff_slots();
    memcpy(saved, memory + MARKER_ZERO, sizeof(saved));
    TEST_ASSERT_EQUAL_UINT32(6u, recover_total(MARKERS_JOURNAL_BYTES,
                                               &recovered));
    TEST_ASSERT_EQUAL_UINT32(2u,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT32(2u,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD, 2));
    TEST_ASSERT_EQUAL_UINT32(2u,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD, 3));

    for (uint32_t slot = 0; slot < MARKER_SLOTS; slot++) {
        for (uint32_t byte = 0; byte < MARKER_BYTES; byte++) {
            for (uint32_t bit = 0; bit < 8u; bit++) {
                uint32_t index = slot * MARKER_BYTES + byte;
                uint32_t sum;

                memcpy(memory + MARKER_ZERO, saved, sizeof(saved));
                memory[MARKER_ZERO + index] =
                    (uint8_t)(memory[MARKER_ZERO + index] ^ (1u << bit));
                TEST_ASSERT_EQUAL_UINT32(
                    6u, recover_total(MARKERS_JOURNAL_BYTES, &recovered));
                sum = pulse_slot_count(&recovered, ND_ENERGY_FORWARD, 1)
                    + pulse_slot_count(&recovered, ND_ENERGY_FORWARD, 2)
                    + pulse_slot_count(&recovered, ND_ENERGY_FORWARD, 3);
                TEST_ASSERT_EQUAL_UINT32(6u, sum);
                TEST_ASSERT_EQUAL_UINT32(0u,
                                         pulse_slot_count(&recovered,
                                                          ND_ENERGY_REVERSE, 1));
            }
        }
    }
    memcpy(memory + MARKER_ZERO, saved, sizeof(saved));
    TEST_ASSERT_EQUAL_UINT32(6u, recover_total(MARKERS_JOURNAL_BYTES,
                                               &recovered));
}

void setUp(void) {}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_lane_byte_recovers_its_cleared_prefix);
    RUN_TEST(test_every_marker_bit_flip_keeps_replay_whole);
    return UNITY_END();
}
