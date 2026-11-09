#include "unity.h"

#include <string.h>

#include "domain/metering/metering.h"
#include "support/fake_allocator.h"
#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"

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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_legacy_offsets_are_frozen);
    RUN_TEST(test_encode_maps_snapshot_to_legacy_fields);
    RUN_TEST(test_decode_restores_snapshot);
    RUN_TEST(test_encode_leaves_checksum_and_padding_untouched);
    return UNITY_END();
}
