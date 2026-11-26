/* The adapter stays fail-closed when the store cannot be read. A persistent
 * fault on one base slot, with the other slot valid, must not fall back to the
 * older generation: the meter runs at zero, clear/set are refused, and
 * restore_pending keeps retrying until the media recovers. On first boot a
 * persistent EC2 energy fault also keeps the meter at zero and pending. A torn
 * snapshot may recover the legacy mirror, but the adapter raises storage_fault
 * because that value may be the last power-off total. storage_fault stays
 * latched until the reporter acknowledges it. */
#include "unity.h"

#include <stddef.h>
#include <string.h>

#include "application/ports/resource_access.h"
#include "metering_energy_harness.h"
#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"

#define EC2_ENERGY_BYTES 160u
#define EC2_MONTH_BYTES 12u

static uint8_t ec2_energy[EC2_ENERGY_BYTES];
static uint8_t ec2_month[EC2_MONTH_BYTES];
static int ec2_fail;

static int16_t test_get_index(void *ctx, uint32_t index, uint8_t *buf,
                              uint16_t size)
{
    (void)ctx;
    if (ec2_fail) {
        return -1;
    }
    if (index == HARNESS_EC2_ENERGY_INDEX && size == EC2_ENERGY_BYTES) {
        memcpy(buf, ec2_energy, size);
        return 0;
    }
    if (index == HARNESS_EC2_MONTH_INDEX && size == EC2_MONTH_BYTES) {
        memcpy(buf, ec2_month, size);
        return 0;
    }
    return -1;
}

static const nd_resource_port_t test_port = {
    .ctx = NULL,
    .get_index = test_get_index,
};

static void seal_steps(uint32_t steps, legacy_energy_record_t *record)
{
    metering_config_t config = {
        .meter_const = HARNESS_METER_CONST,
        .cal_word = ND_CAL_ADD_POS,
        .save_step = HARNESS_SAVE_STEP,
    };
    static uint8_t memory[1024];
    nd_arena_t arena;
    nd_metering_snapshot_t book;
    metering_state_t *state;

    nd_arena_init(&arena, memory, sizeof(memory));
    state = metering_create(nd_arena_allocator(&arena), &config);
    TEST_ASSERT_NOT_NULL(state);
    metering_add_pulses(state, ND_ENERGY_FORWARD, 1,
                        steps * HARNESS_PULSES_PER_STEP);
    metering_export(state, &book);
    memset(record, 0, sizeof(*record));
    legacy_energy_encode(&book, record);
    legacy_energy_seal(record);
    metering_destroy(state, nd_arena_allocator(&arena));
}

static void seed_ec1(uint32_t steps)
{
    legacy_energy_record_t record;

    seal_steps(steps, &record);
    memcpy(harness.eeprom + HARNESS_LEGACY_OFFSET, &record, sizeof(record));
}

static void seed_ec2(uint32_t steps)
{
    legacy_energy_record_t record;

    seal_steps(steps, &record);
    memcpy(ec2_energy, &record, EC2_ENERGY_BYTES);
    memcpy(ec2_month, (const uint8_t *)&record + EC2_ENERGY_BYTES,
           EC2_MONTH_BYTES);
}

void setUp(void)
{
    metering_energy_adapter_reset();
    harness_init();
    ec2_fail = 0;
    memset(ec2_energy, 0xFF, sizeof(ec2_energy));
    memset(ec2_month, 0xFF, sizeof(ec2_month));
}

void tearDown(void) {}

/* Slot 0 holds generation 2 (7 steps), slot 1 generation 1 (5 steps). A
 * persistent read fault on slot 1 fails the whole scan even though slot 0 is
 * valid: no automatic fallback, no commit, no clear/set. */
static void test_single_slot_read_fault_keeps_the_meter_fail_closed(void)
{
    nd_energy_ledger_t *ledger;

    harness_seed_snapshot(5u);
    harness_seed_snapshot(7u);
    harness_reboot();
    harness.fault.fail_read_range = 1;
    harness.fault.read_fail_offset =
        ND_PULSE_SNAPSHOT_RECORD_BYTES(sizeof(legacy_energy_record_t));
    harness.fault.read_fail_bytes = ND_PULSE_SNAPSHOT_HEADER_BYTES;
    ledger = harness_bind();

    TEST_ASSERT_EQUAL_INT64(0, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);
    TEST_ASSERT_EQUAL_INT(1, metering_energy_restore_pending(&harness.instance));
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_clear(ledger));
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_set_total(ledger,
                                                        ND_ENERGY_FORWARD, 5u));

    /* The valid older slot is not adopted; the retry reads the newer one only
     * after the media recovers. */
    harness.fault.fail_read_range = 0;
    TEST_ASSERT_EQUAL_INT64(7, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_INT(0, metering_energy_restore_pending(&harness.instance));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);
}

/* The power-off flag is clear, so EC2 is the newer source. A persistent EC2
 * energy read fault must not adopt the older EC1 half; the restore fails and
 * stays pending until EC2 reads again. */
static void test_persistent_ec2_energy_read_fault_stays_pending_until_recovered(void)
{
    legacy_energy_record_t record;
    nd_energy_ledger_t *ledger;

    seed_ec1(2u);
    seed_ec2(5u);
    harness.instance.resource = test_port;
    ec2_fail = 1;
    ledger = harness_bind();

    TEST_ASSERT_EQUAL_INT64(0, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);
    TEST_ASSERT_EQUAL_INT(1, metering_energy_restore_pending(&harness.instance));
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_clear(ledger));

    ec2_fail = 0;
    TEST_ASSERT_EQUAL_INT64(5, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_INT(0, metering_energy_restore_pending(&harness.instance));
    TEST_ASSERT_EQUAL_INT(0, harness_read_base(&record));
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);
}

/* A torn snapshot falls back to the legacy mirror, but the adapter reports
 * the degraded source: the recovered total may be the last power-off value. */
static void test_torn_store_ec2_energy_fault_recovers_the_mirror_with_a_fault(void)
{
    legacy_energy_record_t record;
    nd_energy_ledger_t *ledger;
    uint32_t record_bytes =
        ND_PULSE_SNAPSHOT_RECORD_BYTES(sizeof(legacy_energy_record_t));

    harness_seed_snapshot(5u);
    harness.snapshot_memory[0u * record_bytes
                            + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 1u;
    harness.snapshot_memory[1u * record_bytes
                            + ND_PULSE_SNAPSHOT_HEADER_BYTES] ^= 2u;
    seed_ec1(2u);
    seed_ec2(5u);
    harness.instance.resource = test_port;
    ec2_fail = 1;
    harness_reboot();
    ledger = harness_bind();

    TEST_ASSERT_EQUAL_INT64(2, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);
    TEST_ASSERT_EQUAL_INT(0, metering_energy_restore_pending(&harness.instance));
    TEST_ASSERT_EQUAL_INT(0, harness_read_base(&record));
    TEST_ASSERT_EQUAL_UINT32(2u, record.active_p[1]);

    /* The degraded value is committed; a later successful EC2 read does not
     * rewrite the base. */
    ec2_fail = 0;
    TEST_ASSERT_EQUAL_INT64(2, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_single_slot_read_fault_keeps_the_meter_fail_closed);
    RUN_TEST(test_persistent_ec2_energy_read_fault_stays_pending_until_recovered);
    RUN_TEST(test_torn_store_ec2_energy_fault_recovers_the_mirror_with_a_fault);
    return UNITY_END();
}
