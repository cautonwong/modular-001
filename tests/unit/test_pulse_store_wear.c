/* Purpose: turn the wear model into a measured number. A block that counts
 * every program and erase runs one million pulses at program granularity 1
 * over the documented 16 KiB, four-sector region, so the commits, the sector
 * erases and the base writes are the documented flash counts, not estimates. */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/pulse_store.h"

#define JOURNAL_BYTES 16384u
#define BASE_BYTES 1024u
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define METADATA_BYTES 544u
#define LANE_BITS (((JOURNAL_BYTES - METADATA_BYTES) / 2u) * 8u)
#define PULSES 1000000u
#define SECTOR_BYTES 4096u
#define SECTORS_PER_ERASE (JOURNAL_BYTES / SECTOR_BYTES)

typedef struct {
    nd_block_ops_t ops;
    uint8_t *data;
    uint32_t programs;
    uint32_t erase_calls;
    uint64_t erase_bytes;
} count_block_t;

static int count_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    count_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memcpy(buffer, block->data + offset, length);
    return 0;
}

static int count_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    count_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memcpy(block->data + offset, buffer, length);
    block->programs++;
    return 0;
}

static int count_erase(void *ctx, uint32_t offset, uint32_t length)
{
    count_block_t *block = ctx;

    if (offset > block->ops.size || length > block->ops.size - offset) {
        return -1;
    }
    memset(block->data + offset, 0xFF, length);
    block->erase_calls++;
    block->erase_bytes += length;
    return 0;
}

void setUp(void) {}

void tearDown(void) {}

static void test_one_million_pulses_match_the_documented_erase_model(void)
{
    static uint8_t memory[MEMORY_BYTES];
    static count_block_t block;
    nd_pulse_store_storage_t storage;
    nd_pulse_store_config_t config;
    nd_pulse_store_t *store;
    uint32_t blob = 0;
    uint32_t commits = 1;
    uint32_t expected_commits = 1u + (PULSES - 1u) / LANE_BITS;

    memset(memory, 0xFF, sizeof(memory));
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
    config = (nd_pulse_store_config_t){
        .block = &block.ops,
        .journal_offset = 0,
        .journal_bytes = JOURNAL_BYTES,
        .base_offset = {JOURNAL_BYTES, JOURNAL_BYTES + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = ND_PULSE_PROGRAM_GRANULARITY,
    };
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
    store = nd_pulse_store_port(&storage);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(store, (const uint8_t *)&blob,
                                                sizeof(blob)));
    for (uint32_t index = 0; index < PULSES; index++) {
        nd_pulse_status_t status = nd_pulse_store_append(
            store, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1));

        if (status == ND_PULSE_FULL) {
            blob++;
            TEST_ASSERT_EQUAL_INT(
                ND_PULSE_OK,
                nd_pulse_store_commit(store, (const uint8_t *)&blob,
                                      sizeof(blob)));
            commits++;
            status = nd_pulse_store_append(
                store, nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1));
        }
        TEST_ASSERT_EQUAL_INT(ND_PULSE_OK, status);
    }
    TEST_ASSERT_EQUAL_UINT32(expected_commits, commits);
    TEST_ASSERT_EQUAL_UINT32(expected_commits, block.erase_calls);
    TEST_ASSERT_EQUAL_UINT32(expected_commits * SECTORS_PER_ERASE,
                             (uint32_t)(block.erase_bytes / SECTOR_BYTES));
    /* One program per pulse, five record programs per commit (base header,
     * blob, tail, two journal headers) and one tariff marker per epoch. */
    TEST_ASSERT_EQUAL_UINT32(PULSES + expected_commits * 5u + expected_commits,
                             block.programs);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_one_million_pulses_match_the_documented_erase_model);
    return UNITY_END();
}
