/* Purpose: a device error must never be mistaken for a normal state. The
 * journal engine has three fault paths where that conflation loses pulses:
 * a torn base region reported as EMPTY (the consumer then adopts a stale
 * legacy record), a base whose write succeeded but whose verify read failed
 * (RAM keeps the old epoch while the disk has the new one), and a marker
 * program that failed but was still counted (the slot hole hides later
 * markers). Each case injects the exact device error over the RAM block,
 * reboots, and asserts the recovered pulse and tariff counts. */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/pulse_store.h"
#include "support/pulse_harness.h"

#define JOURNAL_BYTES 560u
#define BASE_BYTES 1024u
#define BASE_ZERO (JOURNAL_BYTES)
#define BASE_ONE (JOURNAL_BYTES + BASE_BYTES)
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define BASE_HEADER_BYTES 12u
#define MARKER_BYTES 16u
#define MARKER_ZERO (2u * 16u) /* markers follow the two journal headers */

typedef struct {
    nd_block_ops_t ops;
    uint8_t *data;
    uint32_t read_fail_offset;
    uint32_t read_fail_bytes;
    uint32_t read_failures;
    uint32_t corrupt_read_offset;
    uint32_t corrupt_read_bytes;
    uint32_t corrupt_reads;
    uint32_t program_fail_offset;
    uint32_t program_fail_bytes;
    uint32_t program_failures;
    uint8_t program_apply;
} fault_block_t;

static uint8_t memory[MEMORY_BYTES];
static fault_block_t block;
static nd_pulse_store_storage_t storage;

static int fault_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    fault_block_t *fault = ctx;

    if (offset > fault->ops.size || length > fault->ops.size - offset) {
        return -1;
    }
    if (fault->read_failures != 0u
        && offset >= fault->read_fail_offset
        && offset + (uint32_t)length
               <= fault->read_fail_offset + fault->read_fail_bytes) {
        fault->read_failures--;
        return -1;
    }
    memcpy(buffer, fault->data + offset, length);
    /* Flip one read-back byte: the media holds the correct data, the read
     * path hands the engine a CRC-bad record. */
    if (fault->corrupt_reads != 0u
        && offset >= fault->corrupt_read_offset
        && offset + (uint32_t)length
               <= fault->corrupt_read_offset + fault->corrupt_read_bytes) {
        fault->corrupt_reads--;
        buffer[0] ^= 0xFFu;
    }
    return 0;
}

static int fault_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    fault_block_t *fault = ctx;

    if (offset > fault->ops.size || length > fault->ops.size - offset) {
        return -1;
    }
    if (fault->program_failures != 0u
        && offset >= fault->program_fail_offset
        && offset + (uint32_t)length
               <= fault->program_fail_offset + fault->program_fail_bytes) {
        fault->program_failures--;
        if (fault->program_apply != 0u) {
            memcpy(fault->data + offset, buffer, length);
        }
        return -1;
    }
    memcpy(fault->data + offset, buffer, length);
    return 0;
}

static int fault_erase(void *ctx, uint32_t offset, uint32_t length)
{
    fault_block_t *fault = ctx;

    if (offset > fault->ops.size || length > fault->ops.size - offset) {
        return -1;
    }
    memset(fault->data + offset, 0xFF, length);
    return 0;
}

static nd_pulse_store_config_t config_over(void)
{
    nd_pulse_store_config_t config = {
        .block = &block.ops,
        .journal_offset = 0,
        .journal_bytes = JOURNAL_BYTES,
        .base_offset = {BASE_ZERO, BASE_ONE},
        .base_capacity = BASE_BYTES,
        .granularity = ND_PULSE_PROGRAM_GRANULARITY,
    };

    return config;
}

static void boot(void)
{
    nd_pulse_store_config_t config = config_over();

    memset(memory, 0xFF, sizeof(memory));
    block = (fault_block_t){
        .ops = {
            .ctx = &block,
            .read = fault_read,
            .program = fault_program,
            .erase = fault_erase,
            .size = MEMORY_BYTES,
            .erase_unit = 1u,
        },
        .data = memory,
    };
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
}

static void reboot(void)
{
    nd_pulse_store_config_t config = config_over();

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
}

static uint32_t base_generation(void)
{
    const uint8_t *header = memory + BASE_ZERO + 8u; /* after magic/version/bytes */

    return (uint32_t)header[0] | ((uint32_t)header[1] << 8)
         | ((uint32_t)header[2] << 16) | ((uint32_t)header[3] << 24);
}

static nd_pulse_store_t *port(void)
{
    return nd_pulse_store_port(&storage);
}

static void seal(nd_energy_set_t *set)
{
    pulse_set_init(set);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(port(),
                                                (const uint8_t *)set,
                                                sizeof(*set)));
}

static void append_lane(nd_energy_direction_t direction, uint8_t tariff)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_append(port(),
                              nd_energy_pulse_lane(direction, tariff)));
}

static void fail_append(nd_energy_direction_t direction, uint8_t tariff)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_append(port(),
                              nd_energy_pulse_lane(direction, tariff)));
}

static int recover(nd_energy_set_t *set)
{
    reboot();
    return pulse_recover(port(), set);
}

void setUp(void)
{
    boot();
}

void tearDown(void) {}

static void test_torn_base_with_one_blank_slot_is_a_fault(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    seal(&set); /* generation 1 lives in slot 1; slot 0 is blank */
    memory[BASE_ONE + BASE_HEADER_BYTES] ^= 0xFFu; /* tear the only record */
    reboot();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(-1, pulse_recover(port(), &set));
}

static void test_two_torn_base_slots_are_a_fault(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    seal(&set);
    append_lane(ND_ENERGY_FORWARD, 1);
    pulse_add(&set, ND_ENERGY_FORWARD, 1);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(port(),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    memory[BASE_ZERO + BASE_HEADER_BYTES] ^= 0xFFu;
    memory[BASE_ONE + BASE_HEADER_BYTES] ^= 0xFFu;
    reboot();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(-1, pulse_recover(port(), &set));
}

static void test_torn_first_base_write_is_a_fault(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    pulse_set_init(&set);
    /* Generation 1 writes slot 1: its header lands, then the blob program
     * applies and reports an error, so the CRC tail is never written. */
    block.program_fail_offset = BASE_ONE + BASE_HEADER_BYTES;
    block.program_fail_bytes = sizeof(set);
    block.program_failures = 1u;
    block.program_apply = 1u;
    TEST_ASSERT_EQUAL_INT(ND_PULSE_ERROR,
                          nd_pulse_store_commit(port(),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    block.program_failures = 0u;
    reboot();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_ERROR,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(-1, pulse_recover(port(), &set));
}

static void test_blank_base_region_is_still_empty(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_EMPTY,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(port(), &set));
    TEST_ASSERT_EQUAL_UINT32(0, pulse_total(&set));
}

static void test_torn_slot_next_to_a_valid_one_is_skipped(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    seal(&set);
    memory[BASE_ZERO] = 0x00u; /* the blank slot becomes non-blank invalid */
    reboot();
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(nd_energy_set_t), length);
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(port(), &set));
    TEST_ASSERT_EQUAL_UINT32(0, pulse_total(&set));
}

static void test_commit_verify_read_failure_keeps_the_epoch(void)
{
    nd_energy_set_t set;
    nd_energy_set_t recovered;

    seal(&set); /* generation 1 in slot 1 */
    for (uint32_t index = 0; index < 3u; index++) {
        append_lane(ND_ENERGY_FORWARD, 1);
        pulse_add(&set, ND_ENERGY_FORWARD, 1);
    }
    /* Generation 2 writes slot 0. Its programs succeed; the header read that
     * verifies the record fails. */
    block.read_fail_offset = BASE_ZERO;
    block.read_fail_bytes = BASE_HEADER_BYTES;
    block.read_failures = 1u;
    TEST_ASSERT_EQUAL_INT(ND_PULSE_ERROR,
                          nd_pulse_store_commit(port(),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    block.read_failures = 0u;
    append_lane(ND_ENERGY_FORWARD, 1); /* accepted in the committed epoch */
    pulse_add(&set, ND_ENERGY_FORWARD, 1);

    TEST_ASSERT_EQUAL_INT(0, recover(&recovered));
    TEST_ASSERT_EQUAL_UINT32(4, pulse_total(&recovered));
    TEST_ASSERT_EQUAL_UINT32(4,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD,
                                              1));
}

static void test_commit_verify_crc_failure_keeps_the_epoch(void)
{
    nd_energy_set_t set;
    nd_energy_set_t recovered;

    seal(&set); /* generation 1 in slot 1 */
    for (uint32_t index = 0; index < 3u; index++) {
        append_lane(ND_ENERGY_FORWARD, 1);
        pulse_add(&set, ND_ENERGY_FORWARD, 1);
    }
    /* Generation 2 writes slot 0. Its programs succeed, but the read-back
     * hands the engine a corrupted blob, so the CRC check rejects the record. */
    block.corrupt_read_offset = BASE_ZERO + BASE_HEADER_BYTES;
    block.corrupt_read_bytes = sizeof(set);
    block.corrupt_reads = 1u;
    TEST_ASSERT_EQUAL_INT(ND_PULSE_ERROR,
                          nd_pulse_store_commit(port(),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
    block.corrupt_reads = 0u;
    /* The write landed, so the disk slot holds generation 2 even though the
     * engine could not verify it. */
    TEST_ASSERT_EQUAL_UINT32(2, base_generation());
    append_lane(ND_ENERGY_FORWARD, 1); /* accepted in the committed epoch */
    pulse_add(&set, ND_ENERGY_FORWARD, 1);

    TEST_ASSERT_EQUAL_INT(0, recover(&recovered));
    TEST_ASSERT_EQUAL_UINT32(4, pulse_total(&recovered));
    TEST_ASSERT_EQUAL_UINT32(4,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD,
                                              1));
}

static void test_marker_not_programmed_is_not_consumed(void)
{
    nd_energy_set_t set;
    nd_energy_set_t recovered;

    seal(&set);
    append_lane(ND_ENERGY_FORWARD, 1); /* marker slot 0, tariff 1 */
    pulse_add(&set, ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 1);
    pulse_add(&set, ND_ENERGY_FORWARD, 1);
    block.program_fail_offset = MARKER_ZERO + MARKER_BYTES;
    block.program_fail_bytes = MARKER_BYTES;
    block.program_failures = 1u;
    block.program_apply = 0u; /* the marker slot stays erased */
    fail_append(ND_ENERGY_FORWARD, 2);
    block.program_failures = 0u;
    append_lane(ND_ENERGY_FORWARD, 2); /* retry lands in the same slot */
    pulse_add(&set, ND_ENERGY_FORWARD, 2);

    TEST_ASSERT_EQUAL_INT(0, recover(&recovered));
    TEST_ASSERT_EQUAL_UINT32(2,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD,
                                              1));
    TEST_ASSERT_EQUAL_UINT32(1,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD,
                                              2));
    TEST_ASSERT_EQUAL_UINT32(3, pulse_total(&recovered));
}

static void test_unverified_marker_does_not_misattribute_later_pulses(void)
{
    nd_energy_set_t set;
    nd_energy_set_t recovered;

    seal(&set);
    append_lane(ND_ENERGY_FORWARD, 1); /* marker slot 0, tariff 1 */
    pulse_add(&set, ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 1);
    pulse_add(&set, ND_ENERGY_FORWARD, 1);
    /* The marker for the tariff change lands, then the driver reports an
     * error before the verify read. The engine does not know it landed. */
    block.program_fail_offset = MARKER_ZERO + MARKER_BYTES;
    block.program_fail_bytes = MARKER_BYTES;
    block.program_failures = 1u;
    block.program_apply = 1u;
    fail_append(ND_ENERGY_FORWARD, 2);
    block.program_failures = 0u;
    /* Metering continues on the tariff the engine still believes in. */
    append_lane(ND_ENERGY_FORWARD, 1);
    pulse_add(&set, ND_ENERGY_FORWARD, 1);

    TEST_ASSERT_EQUAL_INT(0, recover(&recovered));
    TEST_ASSERT_EQUAL_UINT32(3,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD,
                                              1));
    TEST_ASSERT_EQUAL_UINT32(0,
                             pulse_slot_count(&recovered, ND_ENERGY_FORWARD,
                                              2));
    TEST_ASSERT_EQUAL_UINT32(3, pulse_total(&recovered));
}

static void test_base_read_error_is_io_not_torn(void)
{
    nd_energy_set_t set;
    uint8_t buffer[sizeof(nd_energy_set_t)];
    size_t length = 0;

    seal(&set); /* generation 1 lives in slot 1 */
    reboot();
    block.read_fail_offset = BASE_ONE;
    block.read_fail_bytes = BASE_HEADER_BYTES;
    block.read_failures = 1u;
    /* A device read error leaves the region unknown; only a completed scan
     * with no valid slot is a torn region the consumer may replace. */
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_IO_ERROR,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    /* The fault is transient: the next read reports the real base. */
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_base(port(), buffer, sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_UINT(sizeof(nd_energy_set_t), length);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_torn_base_with_one_blank_slot_is_a_fault);
    RUN_TEST(test_two_torn_base_slots_are_a_fault);
    RUN_TEST(test_torn_first_base_write_is_a_fault);
    RUN_TEST(test_blank_base_region_is_still_empty);
    RUN_TEST(test_torn_slot_next_to_a_valid_one_is_skipped);
    RUN_TEST(test_base_read_error_is_io_not_torn);
    RUN_TEST(test_commit_verify_read_failure_keeps_the_epoch);
    RUN_TEST(test_commit_verify_crc_failure_keeps_the_epoch);
    RUN_TEST(test_marker_not_programmed_is_not_consumed);
    RUN_TEST(test_unverified_marker_does_not_misattribute_later_pulses);
    return UNITY_END();
}
