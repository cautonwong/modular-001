#include "unity.h"

#include <string.h>

#include "domain/metering/metering.h"
#include "support/fake_allocator.h"
#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"

/* The frozen 172 B legacy layout from legacy_energy_record.h and the Lean
 * model: 13 x u32 active_p, 13 x u32 active_n, two 13 x u16 fraction arrays,
 * the energy checksum, the month pair with its padding word, and the use
 * checksum. Fields not named here stay zero. */
static const uint8_t GOLDEN_LEGACY_ENERGY[172] = {
    0x04, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x14, 0x13, 0x12, 0x11, 0x00, 0x00, 0x00, 0x00, 0x24, 0x23, 0x22, 0x21, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x34, 0x33, 0x32, 0x31, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x16, 0x15, 0x00, 0x00, 0x26, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x35, 0x39, 0xA8, 0x00, 0x00,
    0x46, 0x45, 0x00, 0x00, 0x44, 0x43, 0x42, 0x41, 0x3A, 0xA7, 0x00, 0x00,
};

static unsigned char arena[512];
static fake_allocator_t fake;
static metering_state_t *state;
static legacy_energy_record_t record;

void setUp(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
    metering_config_t config = {2000, ND_CAL_ADD_POS | ND_CAL_ADD_NEG, 200};
    state = metering_create(&fake.port, &config);
    memset(&record, 0, sizeof(record));
}

void tearDown(void)
{
    metering_destroy(state, &fake.port);
}

static void test_legacy_offsets_are_frozen(void)
{
    TEST_ASSERT_EQUAL_size_t(172, sizeof(legacy_energy_record_t));
    TEST_ASSERT_EQUAL_size_t(3, ND_LEGACY_ENERGY_OFFSET_IN_BLOCK);
}

static void test_encode_maps_snapshot_to_legacy_fields(void)
{
    metering_add_pulses(state, ND_ENERGY_FORWARD, 2, 4000);
    metering_add_pulses(state, ND_ENERGY_REVERSE, 3, 20);

    nd_metering_snapshot_t snapshot;
    metering_export(state, &snapshot);
    legacy_energy_encode(&snapshot, &record);

    TEST_ASSERT_EQUAL_UINT32(200, record.active_p[0]);
    TEST_ASSERT_EQUAL_UINT32(200, record.active_p[2]);
    TEST_ASSERT_EQUAL_UINT32(1, record.active_n[3]);
    TEST_ASSERT_EQUAL_UINT16(0, record.active_p_dec[0]);
    TEST_ASSERT_EQUAL_UINT32(201, record.month_energy);
}

static void test_decode_restores_snapshot(void)
{
    metering_add_pulses(state, ND_ENERGY_FORWARD, 2, 4017);
    metering_add_pulses(state, ND_ENERGY_REVERSE, 3, 37);

    nd_metering_snapshot_t snapshot;
    metering_export(state, &snapshot);
    legacy_energy_encode(&snapshot, &record);

    nd_metering_snapshot_t restored;
    legacy_energy_decode(&record, &restored);

    TEST_ASSERT_EQUAL_UINT32(snapshot.forward.slots[0].integer,
                             restored.forward.slots[0].integer);
    TEST_ASSERT_EQUAL_UINT16(snapshot.forward.slots[2].fraction,
                             restored.forward.slots[2].fraction);
    TEST_ASSERT_EQUAL_UINT32(snapshot.reverse.slots[3].integer,
                             restored.reverse.slots[3].integer);
    TEST_ASSERT_EQUAL_UINT32(snapshot.month.integer, restored.month.integer);
    TEST_ASSERT_EQUAL_UINT16(snapshot.month.fraction, restored.month.fraction);
}

static void test_encode_leaves_checksum_and_padding_untouched(void)
{
    record.energy_checksum = 0xAABBCCDD;
    record.use_energy_checksum = 0x11223344;
    record.reserved = 0x5A5A;

    nd_metering_snapshot_t snapshot;
    metering_export(state, &snapshot);
    legacy_energy_encode(&snapshot, &record);

    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, record.energy_checksum);
    TEST_ASSERT_EQUAL_UINT32(0x11223344, record.use_energy_checksum);
    TEST_ASSERT_EQUAL_UINT16(0x5A5A, record.reserved);
}

static void test_sealed_record_matches_the_legacy_golden(void)
{
    nd_metering_snapshot_t snapshot = {0};

    snapshot.forward.slots[0] = (nd_energy_amount_t){
        .integer = 0x01020304u, .fraction = 0x0506u};
    snapshot.forward.slots[12] = (nd_energy_amount_t){
        .integer = 0x11121314u, .fraction = 0x1516u};
    snapshot.reverse.slots[1] = (nd_energy_amount_t){
        .integer = 0x21222324u, .fraction = 0x2526u};
    snapshot.reverse.slots[12] = (nd_energy_amount_t){
        .integer = 0x31323334u, .fraction = 0x3536u};
    snapshot.month = (nd_energy_amount_t){
        .integer = 0x41424344u, .fraction = 0x4546u};

    legacy_energy_encode(&snapshot, &record);
    legacy_energy_seal(&record);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_LEGACY_ENERGY,
                                  (const uint8_t *)&record,
                                  sizeof(GOLDEN_LEGACY_ENERGY));
}

static void test_literal_golden_decodes(void)
{
    nd_metering_snapshot_t decoded;

    memcpy(&record, GOLDEN_LEGACY_ENERGY, sizeof(GOLDEN_LEGACY_ENERGY));
    TEST_ASSERT_EQUAL_INT(1, legacy_energy_valid(&record));
    legacy_energy_decode(&record, &decoded);
    TEST_ASSERT_EQUAL_UINT32(0x01020304u, decoded.forward.slots[0].integer);
    TEST_ASSERT_EQUAL_UINT16(0x0506u, decoded.forward.slots[0].fraction);
    TEST_ASSERT_EQUAL_UINT32(0x21222324u, decoded.reverse.slots[1].integer);
    TEST_ASSERT_EQUAL_UINT16(0x4546u, decoded.month.fraction);
    TEST_ASSERT_EQUAL_UINT32(0x41424344u, decoded.month.integer);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_legacy_offsets_are_frozen);
    RUN_TEST(test_encode_maps_snapshot_to_legacy_fields);
    RUN_TEST(test_decode_restores_snapshot);
    RUN_TEST(test_encode_leaves_checksum_and_padding_untouched);
    RUN_TEST(test_sealed_record_matches_the_legacy_golden);
    RUN_TEST(test_literal_golden_decodes);
    return UNITY_END();
}
