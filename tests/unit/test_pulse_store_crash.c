/* Purpose: prove the power-loss bound by fault injection, not by argument.
 * For every program and erase the engine issues over one fixed scenario, the
 * scenario is replayed with that operation failing before it happens and with
 * it applied before reporting failure, then rebooted from the same bytes.
 * Every case must recover at least accepted-1 pulses (the one-pulse bound at
 * granularity 1) and never more pulses than were physically attempted (zero
 * double count). */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/pulse_store.h"
#include "support/pulse_harness.h"

#define JOURNAL_BYTES 560u
#define BASE_BYTES 1024u
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define PULSES 20u
#define MID_COMMIT 10u

typedef struct {
    nd_block_ops_t ops;
    uint8_t *data;
    uint32_t seen;
    uint32_t fail_at;
    uint32_t mode; /* 0: fail before applying, 1: apply then fail */
    int failed;
    uint32_t programs;
    uint32_t erases;
} fault_block_t;

static int in_range(const fault_block_t *block, uint32_t offset,
                    size_t length)
{
    return offset <= block->ops.size && length <= block->ops.size - offset;
}

static int fault_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    fault_block_t *block = ctx;

    if (!in_range(block, offset, length)) {
        return -1;
    }
    memcpy(buffer, block->data + offset, length);
    return 0;
}

static int fault_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    fault_block_t *block = ctx;

    if (!in_range(block, offset, length)) {
        return -1;
    }
    block->seen++;
    if (block->failed == 0 && block->fail_at != 0u
        && block->seen == block->fail_at) {
        block->failed = 1;
        if (block->mode != 0u) {
            memcpy(block->data + offset, buffer, length);
        }
        return -1;
    }
    memcpy(block->data + offset, buffer, length);
    block->programs++;
    return 0;
}

static int fault_erase(void *ctx, uint32_t offset, uint32_t length)
{
    fault_block_t *block = ctx;

    if (!in_range(block, offset, length)) {
        return -1;
    }
    block->seen++;
    if (block->failed == 0 && block->fail_at != 0u
        && block->seen == block->fail_at) {
        block->failed = 1;
        if (block->mode != 0u) {
            memset(block->data + offset, 0xFF, length);
        }
        return -1;
    }
    memset(block->data + offset, 0xFF, length);
    block->erases++;
    return 0;
}

static void block_init(fault_block_t *block, uint8_t *data, uint32_t bytes)
{
    *block = (fault_block_t){
        .ops = {
            .ctx = block,
            .read = fault_read,
            .program = fault_program,
            .erase = fault_erase,
            .size = bytes,
            .erase_unit = 1u,
        },
        .data = data,
    };
}

static nd_pulse_store_config_t config_of(fault_block_t *block)
{
    nd_pulse_store_config_t config = {
        .block = &block->ops,
        .journal_offset = 0,
        .journal_bytes = JOURNAL_BYTES,
        .base_offset = {JOURNAL_BYTES, JOURNAL_BYTES + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = ND_PULSE_PROGRAM_GRANULARITY,
    };

    return config;
}

static void run(fault_block_t *block, uint32_t fail_at, uint32_t mode,
                uint32_t *accepted, uint32_t *attempted)
{
    nd_pulse_store_storage_t storage;
    nd_pulse_store_config_t config = config_of(block);
    nd_energy_set_t set;
    nd_pulse_store_t *store;

    *accepted = 0;
    *attempted = 0;
    block->seen = 0;
    block->fail_at = fail_at;
    block->mode = mode;
    block->failed = 0;
    block->programs = 0;
    block->erases = 0;
    memset(block->data, 0xFF, MEMORY_BYTES);
    if (nd_pulse_store_init(&storage, &config) != 0) {
        return;
    }
    store = nd_pulse_store_port(&storage);
    pulse_set_init(&set);
    if (nd_pulse_store_commit(store, (const uint8_t *)&set, sizeof(set))
        != ND_PULSE_OK || block->failed != 0) {
        return;
    }
    for (uint32_t index = 0; index < PULSES; index++) {
        uint8_t tariff = index < 5u ? 1u : (index < 12u ? 2u : 1u);
        nd_energy_direction_t direction =
            (index & 1u) != 0u ? ND_ENERGY_REVERSE : ND_ENERGY_FORWARD;
        nd_pulse_status_t status;

        (*attempted)++;
        status = nd_pulse_store_append(store,
                                       nd_energy_pulse_lane(direction, tariff));
        if (status == ND_PULSE_FULL) {
            if (nd_pulse_store_commit(store, (const uint8_t *)&set,
                                      sizeof(set))
                    != ND_PULSE_OK
                || block->failed != 0) {
                return;
            }
            status = nd_pulse_store_append(
                store, nd_energy_pulse_lane(direction, tariff));
        }
        if (status != ND_PULSE_OK) {
            return;
        }
        (*accepted)++;
        pulse_add(&set, direction, tariff);
        if (block->failed != 0) {
            return;
        }
        if (index + 1u == MID_COMMIT
            && (nd_pulse_store_commit(store, (const uint8_t *)&set,
                                      sizeof(set))
                    != ND_PULSE_OK
                || block->failed != 0)) {
            return;
        }
    }
}

static uint32_t recover(fault_block_t *block)
{
    nd_pulse_store_storage_t storage;
    nd_pulse_store_config_t config = config_of(block);
    nd_energy_set_t set;

    block->fail_at = 0;
    block->failed = 0;
    if (nd_pulse_store_init(&storage, &config) != 0) {
        return UINT32_MAX;
    }
    if (pulse_recover(nd_pulse_store_port(&storage), &set) != 0) {
        return UINT32_MAX;
    }
    return pulse_total(&set);
}

static uint32_t block_total_ops(void)
{
    static uint8_t memory[MEMORY_BYTES];
    static fault_block_t block;
    uint32_t accepted = 0;
    uint32_t attempted = 0;
    uint32_t total;

    block_init(&block, memory, MEMORY_BYTES);
    run(&block, 0, 0, &accepted, &attempted);
    TEST_ASSERT_EQUAL_UINT32(PULSES, accepted);
    TEST_ASSERT_EQUAL_UINT32(PULSES, attempted);
    total = block.seen;
    TEST_ASSERT_TRUE(total > PULSES);
    return total;
}

void setUp(void) {}

void tearDown(void) {}

static void test_every_operation_keeps_the_one_pulse_bound(void)
{
    static uint8_t memory[MEMORY_BYTES];
    static fault_block_t block;
    uint32_t total = block_total_ops();

    block_init(&block, memory, MEMORY_BYTES);
    for (uint32_t fail_at = 1; fail_at <= total; fail_at++) {
        for (uint32_t mode = 0; mode < 2u; mode++) {
            uint32_t accepted = 0;
            uint32_t attempted = 0;
            uint32_t recovered;

            run(&block, fail_at, mode, &accepted, &attempted);
            TEST_ASSERT_TRUE(block.failed);
            TEST_ASSERT_TRUE(attempted - accepted <= 1u);
            recovered = recover(&block);
            if (recovered == UINT32_MAX) {
                /* A torn first base commit leaves no valid slot and no
                 * legacy source; the engine reports a storage fault. That is
                 * safe only because no pulse was accepted yet. */
                TEST_ASSERT_EQUAL_UINT32(0u, accepted);
                continue;
            }
            TEST_ASSERT_TRUE(recovered + 1u >= accepted);
            TEST_ASSERT_TRUE(recovered <= attempted);
        }
    }
}

static void test_no_fault_recovers_every_accepted_pulse(void)
{
    static uint8_t memory[MEMORY_BYTES];
    static fault_block_t block;
    uint32_t accepted = 0;
    uint32_t attempted = 0;

    block_init(&block, memory, MEMORY_BYTES);
    run(&block, 0, 0, &accepted, &attempted);
    TEST_ASSERT_EQUAL_UINT32(PULSES, accepted);
    TEST_ASSERT_EQUAL_UINT32(PULSES, recover(&block));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_operation_keeps_the_one_pulse_bound);
    RUN_TEST(test_no_fault_recovers_every_accepted_pulse);
    return UNITY_END();
}
