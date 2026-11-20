/* Purpose: pin the fallback's persistence contract. Commit writes the whole
 * record into the alternate slot with a generation counter and CRC, so a torn
 * newest slot falls back to the previous commit and a torn region reports a
 * fault instead of looking empty. The journal recovers the same run with no
 * loss; the snapshot loses every pulse since the previous commit. */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/crc32.h"
#include "infra/storage/pulse_snapshot.h"
#include "infra/storage/pulse_store.h"
#include "support/pulse_harness.h"

#define SNAPSHOT_MEMORY 2048u
#define JOURNAL_BYTES 16384u
#define BASE_BYTES 1024u
#define JOURNAL_MEMORY (JOURNAL_BYTES + 2u * BASE_BYTES)
#define SAVE_PULSES 200u
#define TRUE_PULSES 399u
#define BLOB_BYTES ((uint32_t)sizeof(nd_energy_set_t))
#define RECORD_BYTES ND_PULSE_SNAPSHOT_RECORD_BYTES(BLOB_BYTES)

/* Goldens are the layout documented in docs/design/energy-pulse-store.md
 * section 4.3: little-endian header, blob, CRC-32 (reflected), slot stride
 * rounded up to 4 bytes. Generation 1 writes slot 1. */
#define GOLDEN_BLOB_BYTES 8u
#define GOLDEN_RECORD_BYTES ND_PULSE_SNAPSHOT_RECORD_BYTES(GOLDEN_BLOB_BYTES)
static const uint8_t GOLDEN_BLOB[GOLDEN_BLOB_BYTES] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
};
static const uint8_t GOLDEN_SNAPSHOT_GEN1[24] = {
    0x53, 0x50, 0x44, 0x4E, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x6C, 0x44, 0x53, 0x6D,
};
static const uint8_t GOLDEN_SNAPSHOT_GEN2[24] = {
    0x53, 0x50, 0x44, 0x4E, 0x01, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x9C, 0x96, 0xCD, 0x1A,
};
static const uint8_t GOLDEN_SNAPSHOT_V0_GEN2[24] = {
    0x53, 0x50, 0x44, 0x4E, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0D, 0x07, 0xA5, 0xB4,
};

static uint8_t snapshot_memory[SNAPSHOT_MEMORY];
static uint8_t journal_memory[JOURNAL_MEMORY];
static nd_block_ram_t snapshot_block;
static nd_block_ram_t journal_block;
static nd_pulse_snapshot_storage_t snapshot;
static nd_pulse_store_storage_t journal;

typedef struct {
    nd_block_ops_t ops;
    const nd_block_ops_t *inner;
    uint32_t programs;
    uint32_t program_bytes;
} counted_block_t;

static counted_block_t counted;
static nd_block_ram_t counted_ram;
static uint8_t counted_memory[SNAPSHOT_MEMORY];

static int counted_read(void *ctx, uint32_t offset, uint8_t *buffer,
                        size_t length)
{
    counted_block_t *block = ctx;

    return nd_block_read(block->inner, offset, buffer, length);
}

static int counted_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                           size_t length)
{
    counted_block_t *block = ctx;

    block->programs++;
    block->program_bytes += (uint32_t)length;
    return nd_block_program(block->inner, offset, buffer, length);
}

static int counted_erase(void *ctx, uint32_t offset, uint32_t length)
{
    counted_block_t *block = ctx;

    return nd_block_erase(block->inner, offset, length);
}

static void counted_init(void)
{
    memset(counted_memory, 0xFF, sizeof(counted_memory));
    memset(&counted, 0, sizeof(counted));
    nd_block_ram_init(&counted_ram, counted_memory, sizeof(counted_memory));
    counted.inner = nd_block_ram_port(&counted_ram);
    counted.ops = (nd_block_ops_t){
        .ctx = &counted,
        .read = counted_read,
        .program = counted_program,
        .erase = counted_erase,
        .size = sizeof(counted_memory),
        .erase_unit = 1u,
    };
}

static nd_pulse_snapshot_config_t snapshot_config(void)
{
    nd_pulse_snapshot_config_t config = {
        .block = nd_block_ram_port(&snapshot_block),
        .offset = 0,
        .blob_bytes = BLOB_BYTES,
    };

    return config;
}

static nd_pulse_store_config_t journal_config(void)
{
    nd_pulse_store_config_t config = {
        .block = nd_block_ram_port(&journal_block),
        .journal_offset = 0,
        .journal_bytes = JOURNAL_BYTES,
        .base_offset = {JOURNAL_BYTES, JOURNAL_BYTES + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = ND_PULSE_PROGRAM_GRANULARITY,
    };

    return config;
}

static void start_snapshot(void)
{
    nd_pulse_snapshot_config_t config;

    memset(snapshot_memory, 0xFF, sizeof(snapshot_memory));
    nd_block_ram_init(&snapshot_block, snapshot_memory, sizeof(snapshot_memory));
    config = snapshot_config();
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&snapshot, &config));
}

static void start_journal(void)
{
    nd_pulse_store_config_t config;

    memset(journal_memory, 0xFF, sizeof(journal_memory));
    nd_block_ram_init(&journal_block, journal_memory, sizeof(journal_memory));
    config = journal_config();
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&journal, &config));
}

static void reboot_snapshot(void)
{
    nd_pulse_snapshot_config_t config = snapshot_config();

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&snapshot, &config));
}

static void reboot_journal(void)
{
    nd_pulse_store_config_t config = journal_config();

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&journal, &config));
}

static void commit_snapshot(const nd_energy_set_t *set)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_commit(nd_pulse_snapshot_port(&snapshot),
                              (const uint8_t *)set, sizeof(*set)));
}

static void commit_journal(const nd_energy_set_t *set)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_commit(nd_pulse_store_port(&journal),
                              (const uint8_t *)set, sizeof(*set)));
}

static void pulse_both(nd_energy_set_t *set)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_append(nd_pulse_snapshot_port(&snapshot),
                              nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_append(nd_pulse_store_port(&journal),
                              nd_energy_pulse_lane(ND_ENERGY_FORWARD, 1)));
    pulse_add(set, ND_ENERGY_FORWARD, 1);
}

/* Write a full valid record into a chosen slot, bypassing the generation
 * parity rule the engine uses, so slot selection can be observed directly. */
static void write_record(const uint8_t *blob, uint32_t slot, uint32_t generation)
{
    struct {
        uint32_t magic;
        uint16_t version;
        uint16_t bytes;
        uint32_t generation;
    } header = {
        .magic = ND_PULSE_SNAPSHOT_MAGIC,
        .version = ND_PULSE_SNAPSHOT_VERSION,
        .bytes = (uint16_t)BLOB_BYTES,
        .generation = generation,
    };
    uint8_t tail[ND_PULSE_SNAPSHOT_CRC_BYTES];
    uint32_t offset = slot * RECORD_BYTES;
    uint32_t crc = nd_crc32_update(ND_CRC32_INIT, (const uint8_t *)&header,
                                   sizeof(header));

    crc = nd_crc32_update(crc, blob, BLOB_BYTES);
    crc = nd_crc32_finish(crc);
    tail[0] = (uint8_t)crc;
    tail[1] = (uint8_t)(crc >> 8);
    tail[2] = (uint8_t)(crc >> 16);
    tail[3] = (uint8_t)(crc >> 24);
    TEST_ASSERT_EQUAL_INT(0, nd_block_program(nd_block_ram_port(&snapshot_block),
                                              offset, (const uint8_t *)&header,
                                              sizeof(header)));
    TEST_ASSERT_EQUAL_INT(0, nd_block_program(nd_block_ram_port(&snapshot_block),
                                              offset + sizeof(header), blob,
                                              BLOB_BYTES));
    TEST_ASSERT_EQUAL_INT(0, nd_block_program(nd_block_ram_port(&snapshot_block),
                                              offset + sizeof(header) + BLOB_BYTES,
                                              tail, sizeof(tail)));
}

void setUp(void)
{
    start_snapshot();
    start_journal();
    counted_init();
}

void tearDown(void) {}

static void test_snapshot_blank_and_capacity(void)
{
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;
    nd_energy_set_t set;

    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_EMPTY,
        nd_pulse_store_base(nd_pulse_snapshot_port(&snapshot), buffer,
                            sizeof(buffer), &length));
    pulse_set_init(&set);
    commit_snapshot(&set);
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_FULL,
        nd_pulse_store_base(nd_pulse_snapshot_port(&snapshot), buffer,
                            sizeof(buffer) - 1u, &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(nd_energy_set_t), length);
}

static void test_generation_selects_the_highest_valid_slot(void)
{
    uint8_t high[sizeof(nd_energy_set_t)];
    uint8_t low[sizeof(nd_energy_set_t)];
    uint8_t recovered[sizeof(nd_energy_set_t)];
    size_t length = 0;

    memset(low, 0x11, sizeof(low));
    memset(high, 0x22, sizeof(high));
    write_record(low, 1, 4u);
    write_record(high, 0, 5u);

    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_base(nd_pulse_snapshot_port(&snapshot), recovered,
                            sizeof(recovered), &length));
    TEST_ASSERT_EQUAL_UINT(BLOB_BYTES, length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(high, recovered, BLOB_BYTES);
}

static void test_torn_newest_slot_falls_back_to_the_previous_commit(void)
{
    nd_energy_set_t first;
    nd_energy_set_t second;
    nd_energy_set_t recovered;

    pulse_set_init(&first);
    pulse_set_init(&second);
    pulse_add(&second, ND_ENERGY_FORWARD, 1);
    commit_snapshot(&first);
    commit_snapshot(&second);
    /* Generation 2 lives in slot 0; tear its blob byte. */
    snapshot_memory[0u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 0x01u;
    reboot_snapshot();
    TEST_ASSERT_EQUAL_INT(
        0, pulse_recover(nd_pulse_snapshot_port(&snapshot), &recovered));
    TEST_ASSERT_EQUAL_UINT32(0, pulse_total(&recovered));
}

static void test_torn_only_slot_is_a_fault_not_empty(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    pulse_set_init(&set);
    commit_snapshot(&set);
    /* Generation 1 lives in slot 1. */
    snapshot_memory[1u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES + 3u]
        ^= 0x80u;
    reboot_snapshot();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(nd_pulse_snapshot_port(&snapshot), buffer,
                            sizeof(buffer), &length));
}

static void test_foreign_record_is_a_fault_not_empty(void)
{
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    memset(snapshot_memory, 0x00, 64u);
    reboot_snapshot();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(nd_pulse_snapshot_port(&snapshot), buffer,
                            sizeof(buffer), &length));
}

static void test_commit_writes_one_record_to_the_alternate_slot(void)
{
    nd_pulse_snapshot_config_t config = {
        .block = &counted.ops,
        .offset = 0,
        .blob_bytes = BLOB_BYTES,
    };
    nd_pulse_snapshot_storage_t storage;
    nd_energy_set_t set;

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&storage, &config));
    pulse_set_init(&set);
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_commit(nd_pulse_snapshot_port(&storage),
                              (const uint8_t *)&set, sizeof(set)));
    TEST_ASSERT_EQUAL_UINT32(3u, counted.programs);
    TEST_ASSERT_EQUAL_UINT32(RECORD_BYTES, counted.program_bytes);
}

static void test_snapshot_loses_the_199_pulse_window(void)
{
    nd_energy_set_t set;
    nd_energy_set_t recovered;

    pulse_set_init(&set);
    commit_snapshot(&set);
    commit_journal(&set);
    for (uint32_t index = 1; index <= TRUE_PULSES; index++) {
        pulse_both(&set);
        if (index % SAVE_PULSES == 0u) {
            commit_snapshot(&set);
            commit_journal(&set);
        }
    }
    reboot_snapshot();
    TEST_ASSERT_EQUAL_INT(
        0, pulse_recover(nd_pulse_snapshot_port(&snapshot), &recovered));
    TEST_ASSERT_EQUAL_UINT32(SAVE_PULSES, pulse_total(&recovered));
    TEST_ASSERT_EQUAL_UINT32(199,
                             TRUE_PULSES - pulse_total(&recovered));
    commit_snapshot(&set);
    reboot_snapshot();
    TEST_ASSERT_EQUAL_INT(
        0, pulse_recover(nd_pulse_snapshot_port(&snapshot), &recovered));
    TEST_ASSERT_EQUAL_UINT32(TRUE_PULSES, pulse_total(&recovered));
}

static void test_journal_recovers_the_same_run(void)
{
    nd_energy_set_t set;
    nd_energy_set_t recovered;

    pulse_set_init(&set);
    commit_snapshot(&set);
    commit_journal(&set);
    for (uint32_t index = 1; index <= TRUE_PULSES; index++) {
        pulse_both(&set);
        if (index % SAVE_PULSES == 0u) {
            commit_snapshot(&set);
            commit_journal(&set);
        }
    }
    reboot_journal();
    TEST_ASSERT_EQUAL_INT(
        0, pulse_recover(nd_pulse_store_port(&journal), &recovered));
    TEST_ASSERT_EQUAL_UINT32(TRUE_PULSES, pulse_total(&recovered));
}

static void test_the_two_bindings_differ_by_the_window(void)
{
    nd_energy_set_t set;
    nd_energy_set_t snapshot_set;
    nd_energy_set_t journal_set;

    pulse_set_init(&set);
    commit_snapshot(&set);
    commit_journal(&set);
    for (uint32_t index = 1; index <= TRUE_PULSES; index++) {
        pulse_both(&set);
        if (index % SAVE_PULSES == 0u) {
            commit_snapshot(&set);
            commit_journal(&set);
        }
    }
    reboot_snapshot();
    reboot_journal();
    TEST_ASSERT_EQUAL_INT(
        0, pulse_recover(nd_pulse_snapshot_port(&snapshot), &snapshot_set));
    TEST_ASSERT_EQUAL_INT(
        0, pulse_recover(nd_pulse_store_port(&journal), &journal_set));
    TEST_ASSERT_EQUAL_UINT32(TRUE_PULSES - SAVE_PULSES,
                             pulse_total(&journal_set)
                                 - pulse_total(&snapshot_set));
}

static void test_snapshot_record_matches_the_documented_golden(void)
{
    nd_pulse_snapshot_config_t config = {
        .block = nd_block_ram_port(&snapshot_block),
        .offset = 0,
        .blob_bytes = GOLDEN_BLOB_BYTES,
    };
    nd_pulse_snapshot_storage_t storage;

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&storage, &config));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_commit(nd_pulse_snapshot_port(&storage), GOLDEN_BLOB,
                              sizeof(GOLDEN_BLOB)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_SNAPSHOT_GEN1,
                                  snapshot_memory + GOLDEN_RECORD_BYTES,
                                  sizeof(GOLDEN_SNAPSHOT_GEN1));
    for (uint32_t index = 0; index < GOLDEN_RECORD_BYTES; index++) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, snapshot_memory[index]);
    }
}

static void test_literal_snapshot_golden_loads(void)
{
    nd_pulse_snapshot_config_t config = {
        .block = nd_block_ram_port(&snapshot_block),
        .offset = 0,
        .blob_bytes = GOLDEN_BLOB_BYTES,
    };
    nd_pulse_snapshot_storage_t storage;
    uint8_t out[GOLDEN_BLOB_BYTES];
    size_t length = 0;

    memcpy(snapshot_memory, GOLDEN_SNAPSHOT_GEN2, sizeof(GOLDEN_SNAPSHOT_GEN2));
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&storage, &config));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_base(nd_pulse_snapshot_port(&storage), out, sizeof(out),
                            &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(GOLDEN_BLOB), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_BLOB, out, sizeof(GOLDEN_BLOB));
}

static void test_old_snapshot_version_falls_back_to_the_previous_commit(void)
{
    nd_pulse_snapshot_config_t config = {
        .block = nd_block_ram_port(&snapshot_block),
        .offset = 0,
        .blob_bytes = GOLDEN_BLOB_BYTES,
    };
    nd_pulse_snapshot_storage_t storage;
    uint8_t out[GOLDEN_BLOB_BYTES];
    size_t length = 0;

    memcpy(snapshot_memory, GOLDEN_SNAPSHOT_GEN1,
           sizeof(GOLDEN_SNAPSHOT_GEN1));
    memcpy(snapshot_memory + GOLDEN_RECORD_BYTES, GOLDEN_SNAPSHOT_V0_GEN2,
           sizeof(GOLDEN_SNAPSHOT_V0_GEN2));
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_snapshot_init(&storage, &config));
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_base(nd_pulse_snapshot_port(&storage), out, sizeof(out),
                            &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(GOLDEN_BLOB), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_BLOB, out, sizeof(GOLDEN_BLOB));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_snapshot_blank_and_capacity);
    RUN_TEST(test_generation_selects_the_highest_valid_slot);
    RUN_TEST(test_torn_newest_slot_falls_back_to_the_previous_commit);
    RUN_TEST(test_torn_only_slot_is_a_fault_not_empty);
    RUN_TEST(test_foreign_record_is_a_fault_not_empty);
    RUN_TEST(test_commit_writes_one_record_to_the_alternate_slot);
    RUN_TEST(test_snapshot_loses_the_199_pulse_window);
    RUN_TEST(test_journal_recovers_the_same_run);
    RUN_TEST(test_the_two_bindings_differ_by_the_window);
    RUN_TEST(test_snapshot_record_matches_the_documented_golden);
    RUN_TEST(test_literal_snapshot_golden_loads);
    RUN_TEST(test_old_snapshot_version_falls_back_to_the_previous_commit);
    return UNITY_END();
}
