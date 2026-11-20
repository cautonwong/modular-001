/* Purpose: the consumer side of the dual-slot snapshot. On an empty store the
 * legacy sources are adopted exactly once and committed, mirroring the old
 * boot recovery: EC1 (the power-off record) only while the 0x5AA5 flag is set,
 * otherwise the EC2 backup. A torn store (a completed scan with no valid slot)
 * is a known state that an explicit generation-1 commit may replace, so the
 * last durable legacy mirror is adopted once. A device read error is not a
 * torn store: it must fail the restore and retry, because an error that
 * clears before the commit would otherwise write the legacy value as the
 * next generation over a real base. A failed EC2 half is retried once; a
 * persistent EC2 energy read fault fails a first-boot adoption instead of
 * falling back to the older EC1 half, and a torn-store recovery that uses EC1
 * reports a degraded restore. */
#include "unity.h"

#include <string.h>

#include "domain/metering/metering.h"
#include "infra/memory/arena.h"
#include "infra/memory/byte_store.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/pulse_snapshot.h"
#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"
#include "targets/stategrid_meter/persistence/legacy_energy_store.h"

#define SNAPSHOT_MEMORY 512u
#define LEGACY_MEMORY 512u
#define LEGACY_BLOCK_OFFSET 0x20u
#define LEGACY_OFFSET (LEGACY_BLOCK_OFFSET + ND_LEGACY_ENERGY_OFFSET_IN_BLOCK)
#define FLAG_OFFSET 0x100u
#define EC2_ENERGY_INDEX 2000u
#define EC2_MONTH_INDEX 2001u
#define EC2_ENERGY_BYTES 160u
#define EC2_MONTH_BYTES 12u
#define POWEROFF_FLAG 0x5AA5u
#define RECORD_BYTES                                                         \
    ND_PULSE_SNAPSHOT_RECORD_BYTES(sizeof(legacy_energy_record_t))
#define RECORD_GENERATION_OFFSET 8u /* magic 4, version 2, bytes 2 */
#define PULSES_PER_STEP 32u
#define METER_CONST 3200u

static uint8_t snapshot_memory[SNAPSHOT_MEMORY];
static uint8_t legacy_memory[LEGACY_MEMORY];
static uint8_t ec2_energy[EC2_ENERGY_BYTES];
static uint8_t ec2_month[EC2_MONTH_BYTES];
static uint8_t arena_memory[1024];
static nd_block_ram_t snapshot_block;
static nd_byte_store_t legacy_store;
static nd_store_t ec2_store;
static nd_pulse_snapshot_storage_t snapshot;
static nd_arena_t arena;
static metering_config_t config;
static legacy_energy_sources_t sources;
static unsigned ec2_energy_reads;
static unsigned ec2_month_reads;
/* Positive: fail this many energy reads. Negative: fail every one. */
static int ec2_energy_failures;

/* The snapshot device with an armable read failure over one byte range. */
typedef struct {
    nd_block_ops_t ops;
    nd_block_ops_t *inner;
    uint32_t read_fail_offset;
    uint32_t read_fail_bytes;
    uint32_t read_failures;
} fault_block_t;

static fault_block_t snapshot_fault;

static int fault_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    fault_block_t *fault = ctx;

    if (fault->read_failures != 0u
        && offset >= fault->read_fail_offset
        && offset + (uint32_t)length
               <= fault->read_fail_offset + fault->read_fail_bytes) {
        fault->read_failures--;
        return -1;
    }
    return nd_block_read(fault->inner, offset, buffer, length);
}

static int fault_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    fault_block_t *fault = ctx;

    return nd_block_program(fault->inner, offset, buffer, length);
}

static int fault_erase(void *ctx, uint32_t offset, uint32_t length)
{
    fault_block_t *fault = ctx;

    return nd_block_erase(fault->inner, offset, length);
}

static metering_state_t *new_state(void)
{
    return metering_create(nd_arena_allocator(&arena), &config);
}

static void add_steps(metering_state_t *state, uint32_t steps)
{
    metering_add_pulses(state, ND_ENERGY_FORWARD, 1, steps * PULSES_PER_STEP);
}

static uint32_t forward_steps(metering_state_t *state)
{
    return (uint32_t)metering_read(state, ND_ENERGY_FORWARD, 1,
                                   ND_ENERGY_RES_0_01_KWH);
}

static uint32_t load_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
         | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint32_t generation_of(uint32_t slot)
{
    return load_u32(snapshot_memory + slot * RECORD_BYTES
                    + RECORD_GENERATION_OFFSET);
}

static void seal_steps(uint32_t steps, legacy_energy_record_t *record)
{
    metering_state_t *source = new_state();
    nd_metering_snapshot_t book;

    TEST_ASSERT_NOT_NULL(source);
    add_steps(source, steps);
    metering_export(source, &book);
    memset(record, 0, sizeof(*record));
    legacy_energy_encode(&book, record);
    legacy_energy_seal(record);
    metering_destroy(source, nd_arena_allocator(&arena));
}

static void seed_legacy(uint32_t steps)
{
    legacy_energy_record_t record;

    seal_steps(steps, &record);
    TEST_ASSERT_EQUAL_INT(
        0, nd_store_write(nd_byte_store_port(&legacy_store), LEGACY_OFFSET,
                          (const uint8_t *)&record, sizeof(record)));
}

static void seed_ec2(uint32_t steps)
{
    legacy_energy_record_t record;

    seal_steps(steps, &record);
    memcpy(ec2_energy, &record, EC2_ENERGY_BYTES);
    memcpy(ec2_month, (const uint8_t *)&record + EC2_ENERGY_BYTES,
           EC2_MONTH_BYTES);
}

static void set_poweroff_flag(uint16_t flag)
{
    TEST_ASSERT_EQUAL_INT(0,
                          nd_store_write(nd_byte_store_port(&legacy_store),
                                         FLAG_OFFSET, (const uint8_t *)&flag,
                                         sizeof(flag)));
}

/* The parameter manager serves each EC2 item by its resource index. */
static int ec2_read(void *ctx, uint32_t index, uint8_t *buffer, size_t length)
{
    (void)ctx;
    if (index == EC2_ENERGY_INDEX && length == EC2_ENERGY_BYTES) {
        ec2_energy_reads++;
        if (ec2_energy_failures != 0) {
            if (ec2_energy_failures > 0) {
                ec2_energy_failures--;
            }
            return -1;
        }
        memcpy(buffer, ec2_energy, EC2_ENERGY_BYTES);
        return 0;
    }
    if (index == EC2_MONTH_INDEX && length == EC2_MONTH_BYTES) {
        ec2_month_reads++;
        memcpy(buffer, ec2_month, EC2_MONTH_BYTES);
        return 0;
    }
    return -1;
}

static int commit(metering_state_t *state)
{
    return legacy_energy_persist(nd_pulse_snapshot_port(&snapshot), state);
}

static int restore(metering_state_t *state)
{
    return legacy_energy_restore(nd_pulse_snapshot_port(&snapshot), &sources,
                                 state);
}

static void reboot(void)
{
    nd_pulse_snapshot_config_t snapshot_config = {
        .block = &snapshot_fault.ops,
        .offset = 0,
        .blob_bytes = sizeof(legacy_energy_record_t),
    };

    TEST_ASSERT_EQUAL_INT(0,
                          nd_pulse_snapshot_init(&snapshot, &snapshot_config));
}

static int read_record(legacy_energy_record_t *record)
{
    size_t length = 0;
    nd_pulse_status_t status =
        nd_pulse_store_base(nd_pulse_snapshot_port(&snapshot),
                            (uint8_t *)record, sizeof(*record), &length);

    return status == ND_PULSE_OK && length == sizeof(*record) ? 0 : -1;
}

/* A real committed base: generation 1 lives in slot 1. */
static void seed_base(uint32_t steps)
{
    metering_state_t *state = new_state();

    TEST_ASSERT_NOT_NULL(state);
    add_steps(state, steps);
    TEST_ASSERT_EQUAL_INT(0, commit(state));
    metering_destroy(state, nd_arena_allocator(&arena));
}

/* Fail the first read of the valid slot's header once. */
static void arm_base_read_fault(void)
{
    snapshot_fault.read_fail_offset = RECORD_BYTES;
    snapshot_fault.read_fail_bytes = ND_PULSE_SNAPSHOT_HEADER_BYTES;
    snapshot_fault.read_failures = 1u;
}

void setUp(void)
{
    memset(snapshot_memory, 0xFF, sizeof(snapshot_memory));
    memset(legacy_memory, 0xFF, sizeof(legacy_memory));
    memset(ec2_energy, 0xFF, sizeof(ec2_energy));
    memset(ec2_month, 0xFF, sizeof(ec2_month));
    nd_block_ram_init(&snapshot_block, snapshot_memory, sizeof(snapshot_memory));
    memset(&snapshot_fault, 0, sizeof(snapshot_fault));
    snapshot_fault.inner = nd_block_ram_port(&snapshot_block);
    snapshot_fault.ops = *snapshot_fault.inner;
    snapshot_fault.ops.ctx = &snapshot_fault;
    snapshot_fault.ops.read = fault_read;
    snapshot_fault.ops.program = fault_program;
    snapshot_fault.ops.erase = fault_erase;
    nd_byte_store_init(&legacy_store, legacy_memory, sizeof(legacy_memory));
    ec2_energy_reads = 0;
    ec2_month_reads = 0;
    ec2_energy_failures = 0;
    ec2_store.ctx = NULL;
    ec2_store.read = ec2_read;
    ec2_store.write = NULL;
    ec2_store.erase = NULL;
    sources.eeprom = nd_byte_store_port(&legacy_store);
    sources.poweroff_offset = LEGACY_OFFSET;
    sources.flag_offset = FLAG_OFFSET;
    sources.backup = &ec2_store;
    sources.backup_energy = EC2_ENERGY_INDEX;
    sources.backup_month = EC2_MONTH_INDEX;
    reboot();
    nd_arena_init(&arena, arena_memory, sizeof(arena_memory));
    config.meter_const = METER_CONST;
    config.cal_word = ND_CAL_ADD_POS;
    config.save_step = 30;
}

void tearDown(void) {}

static void test_poweroff_flag_set_adopts_the_ec1_record(void)
{
    metering_state_t *state = new_state();

    seed_legacy(7u);
    seed_ec2(2u);
    set_poweroff_flag(POWEROFF_FLAG);
    TEST_ASSERT_EQUAL_INT(0, restore(state));
    TEST_ASSERT_EQUAL_UINT32(7u, forward_steps(state));
}

static void test_empty_store_prefers_the_ec2_backup(void)
{
    metering_state_t *state = new_state();
    legacy_energy_record_t record;

    seed_legacy(1u);
    seed_ec2(5u);
    set_poweroff_flag(0u);
    TEST_ASSERT_EQUAL_INT(0, restore(state));
    TEST_ASSERT_EQUAL_UINT32(5u, forward_steps(state));
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_TRUE(legacy_energy_valid(&record));
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);
    /* Generation 1 lands in slot 1, the alternate of generation & 1. */
    TEST_ASSERT_EQUAL_UINT32(1u, generation_of(1u));
}

static void test_torn_ec1_falls_back_to_the_ec2_backup(void)
{
    metering_state_t *state = new_state();

    seed_legacy(1u);
    legacy_memory[LEGACY_OFFSET + 8u] ^= 0x5Au;
    seed_ec2(5u);
    set_poweroff_flag(POWEROFF_FLAG);
    TEST_ASSERT_EQUAL_INT(0, restore(state));
    TEST_ASSERT_EQUAL_UINT32(5u, forward_steps(state));
}

static void test_blank_sources_start_at_zero(void)
{
    metering_state_t *state = new_state();
    legacy_energy_record_t record;

    set_poweroff_flag(0u);
    TEST_ASSERT_EQUAL_INT(0, restore(state));
    TEST_ASSERT_EQUAL_UINT32(0u, forward_steps(state));
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_TRUE(legacy_energy_valid(&record));
    TEST_ASSERT_EQUAL_UINT32(1u, generation_of(1u));
}

static void test_torn_store_recovers_the_legacy_mirror_once(void)
{
    metering_state_t *state = new_state();
    metering_state_t *torn = new_state();
    legacy_energy_record_t record;

    seed_legacy(9u);
    seed_ec2(9u);
    add_steps(state, 1u);
    TEST_ASSERT_EQUAL_INT(0, commit(state)); /* generation 1, slot 1 */
    add_steps(state, 1u);
    TEST_ASSERT_EQUAL_INT(0, commit(state)); /* generation 2, slot 0 */
    snapshot_memory[0u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 1u;
    snapshot_memory[1u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 2u;
    reboot();

    /* A damaged region is a known state an explicit commit may replace; the
     * legacy mirror is the last durable source. Refusing forever would leave
     * the meter at zero with no persistence. */
    TEST_ASSERT_EQUAL_INT(0, restore(torn));
    TEST_ASSERT_EQUAL_UINT32(9u, forward_steps(torn));
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_EQUAL_UINT32(1u, generation_of(1u));
    TEST_ASSERT_EQUAL_UINT32(9u, record.active_p[1]);
}

static void test_torn_ec2_month_keeps_the_ec2_energy_half(void)
{
    metering_state_t *state = new_state();
    nd_metering_snapshot_t book;

    seed_legacy(1u);
    seed_ec2(5u);
    ec2_month[0] ^= 0xFFu; /* the month item checksum no longer matches */
    set_poweroff_flag(0u);
    TEST_ASSERT_EQUAL_INT(0, restore(state));
    TEST_ASSERT_EQUAL_UINT32(5u, forward_steps(state));
    metering_export(state, &book);
    TEST_ASSERT_EQUAL_UINT32(1u, book.month.integer);
}

static void test_unreadable_poweroff_without_a_backup_half_is_a_fault(void)
{
    metering_state_t *state = new_state();

    sources.poweroff_offset = LEGACY_MEMORY - 4u; /* the 172-byte read fails */
    set_poweroff_flag(0u);
    TEST_ASSERT_EQUAL_INT(-1, restore(state));
    TEST_ASSERT_EQUAL_UINT32(0u, forward_steps(state));
}

static void test_commit_round_trips_and_advances_the_generation(void)
{
    metering_state_t *state = new_state();
    metering_state_t *restored = new_state();
    legacy_energy_record_t record;

    add_steps(state, 3u);
    TEST_ASSERT_EQUAL_INT(0, commit(state));
    TEST_ASSERT_EQUAL_UINT32(1u, generation_of(1u));
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_EQUAL_UINT32(3u, record.active_p[1]);
    TEST_ASSERT_TRUE(legacy_energy_valid(&record));

    add_steps(state, 2u);
    TEST_ASSERT_EQUAL_INT(0, commit(state));
    TEST_ASSERT_EQUAL_UINT32(2u, generation_of(0u));
    TEST_ASSERT_EQUAL_INT(0, restore(restored));
    TEST_ASSERT_EQUAL_UINT32(5u, forward_steps(restored));
}

static void test_torn_newest_falls_back_to_the_previous_commit(void)
{
    metering_state_t *state = new_state();
    metering_state_t *restored = new_state();
    legacy_energy_record_t record;

    add_steps(state, 1u);
    TEST_ASSERT_EQUAL_INT(0, commit(state));
    TEST_ASSERT_EQUAL_UINT32(1u, generation_of(1u));
    add_steps(state, 1u);
    TEST_ASSERT_EQUAL_INT(0, commit(state));
    TEST_ASSERT_EQUAL_UINT32(2u, generation_of(0u));
    snapshot_memory[0u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 4u;
    reboot();

    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_TRUE(legacy_energy_valid(&record));
    TEST_ASSERT_EQUAL_UINT32(1u, record.active_p[1]);
    TEST_ASSERT_EQUAL_INT(0, restore(restored));
    TEST_ASSERT_EQUAL_UINT32(1u, forward_steps(restored));
}

/* A transient device read error while scanning the base is not a torn store.
 * The restore must fail without adopting, or the retried commit writes the
 * legacy value as generation 2 over the real base. */
static void test_transient_base_read_error_never_adopts_legacy(void)
{
    metering_state_t *boot = new_state();
    legacy_energy_record_t record;
    int result;

    seed_base(5u);
    seed_legacy(2u);
    seed_ec2(2u);
    set_poweroff_flag(0u);
    reboot();
    arm_base_read_fault();

    result = restore(boot);

    /* Slot 0 stays blank: no adopted generation was written. */
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, generation_of(0u));
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_EQUAL_UINT32(0u, forward_steps(boot));
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);
}

/* Once the error clears, the retry loads the real base. The fault must not
 * lock the meter out of its own store. */
static void test_base_read_error_retries_and_loads_the_real_base(void)
{
    metering_state_t *boot = new_state();
    legacy_energy_record_t record;

    seed_base(5u);
    seed_legacy(2u);
    seed_ec2(2u);
    set_poweroff_flag(0u);
    reboot();
    arm_base_read_fault();

    TEST_ASSERT_EQUAL_INT(-1, restore(boot));
    TEST_ASSERT_EQUAL_INT(0, restore(boot));
    TEST_ASSERT_EQUAL_UINT32(5u, forward_steps(boot));
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, generation_of(0u));
}

/* One transient EC2 energy read must not drop the fresh backup half: the
 * stale EC1 half would be adopted and committed as generation 1 instead. */
static void test_transient_ec2_energy_read_failure_is_retried(void)
{
    metering_state_t *state = new_state();

    seed_legacy(2u);
    seed_ec2(5u);
    set_poweroff_flag(0u);
    ec2_energy_failures = 1;

    TEST_ASSERT_EQUAL_INT(0, restore(state));
    TEST_ASSERT_EQUAL_UINT32(5u, forward_steps(state));
    TEST_ASSERT_EQUAL_UINT(2u, ec2_energy_reads);
}

/* The retry is exactly one. On first boot a persistent EC2 energy read fault
 * is a storage fault, not an absent item: falling back to the older EC1 half
 * would silently regress the total to the last power-off value. The restore
 * fails without adopting or committing anything. */
static void test_persistent_ec2_energy_read_failure_fails_closed(void)
{
    metering_state_t *state = new_state();

    seed_legacy(2u);
    seed_ec2(5u);
    set_poweroff_flag(0u);
    ec2_energy_failures = -1;

    TEST_ASSERT_EQUAL_INT(-1, restore(state));
    TEST_ASSERT_EQUAL_UINT32(0u, forward_steps(state));
    TEST_ASSERT_EQUAL_UINT(2u, ec2_energy_reads);
    /* No generation-1 adoption was written over the blank snapshot. */
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, generation_of(0u));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, generation_of(1u));
}

/* A torn snapshot has no newer source left, so the legacy mirror is the
 * recovery source. Recovering EC1 is allowed, but the restore reports the
 * degraded fallback so the caller raises a storage fault: the total may be
 * the last power-off value, not the newer EC2 backup. */
static void test_torn_store_with_a_persistent_ec2_read_fault_is_degraded(void)
{
    metering_state_t *state = new_state();
    metering_state_t *torn = new_state();
    legacy_energy_record_t record;

    seed_legacy(7u);
    seed_ec2(5u);
    add_steps(state, 1u);
    TEST_ASSERT_EQUAL_INT(0, commit(state)); /* generation 1, slot 1 */
    add_steps(state, 1u);
    TEST_ASSERT_EQUAL_INT(0, commit(state)); /* generation 2, slot 0 */
    snapshot_memory[0u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 1u;
    snapshot_memory[1u * RECORD_BYTES + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 2u;
    reboot();
    set_poweroff_flag(0u);
    ec2_energy_failures = -1;

    TEST_ASSERT_EQUAL_INT(1, restore(torn));
    TEST_ASSERT_EQUAL_UINT32(7u, forward_steps(torn));
    TEST_ASSERT_EQUAL_UINT(2u, ec2_energy_reads);
    TEST_ASSERT_EQUAL_INT(0, read_record(&record));
    TEST_ASSERT_EQUAL_UINT32(1u, generation_of(1u));
    TEST_ASSERT_EQUAL_UINT32(7u, record.active_p[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_poweroff_flag_set_adopts_the_ec1_record);
    RUN_TEST(test_empty_store_prefers_the_ec2_backup);
    RUN_TEST(test_torn_ec1_falls_back_to_the_ec2_backup);
    RUN_TEST(test_blank_sources_start_at_zero);
    RUN_TEST(test_torn_store_recovers_the_legacy_mirror_once);
    RUN_TEST(test_torn_ec2_month_keeps_the_ec2_energy_half);
    RUN_TEST(test_unreadable_poweroff_without_a_backup_half_is_a_fault);
    RUN_TEST(test_commit_round_trips_and_advances_the_generation);
    RUN_TEST(test_torn_newest_falls_back_to_the_previous_commit);
    RUN_TEST(test_transient_base_read_error_never_adopts_legacy);
    RUN_TEST(test_base_read_error_retries_and_loads_the_real_base);
    RUN_TEST(test_transient_ec2_energy_read_failure_is_retried);
    RUN_TEST(test_persistent_ec2_energy_read_failure_fails_closed);
    RUN_TEST(test_torn_store_with_a_persistent_ec2_read_fault_is_degraded);
    return UNITY_END();
}
