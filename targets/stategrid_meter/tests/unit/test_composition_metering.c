#include "unity.h"

#include <string.h>

#include "domain/metering/metering.h"
#include "infra/memory/arena.h"
#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"

/*
 * Host composition root: the arena plays the role the target memory map plays
 * on the meter, and the metering aggregate is wired exactly as targets do.
 */
static unsigned char memory[512];
static nd_arena_t arena;
static metering_config_t config;

void setUp(void)
{
    nd_arena_init(&arena, memory, sizeof(memory));
    config.meter_const = 2000;
    config.cal_word = ND_CAL_ADD_POS | ND_CAL_ADD_NEG;
    config.save_step = 200;
}

void tearDown(void) {}

static void test_wiring_arena_to_aggregate(void)
{
    metering_state_t *state = metering_create(nd_arena_allocator(&arena), &config);
    TEST_ASSERT_NOT_NULL(state);
    TEST_ASSERT_TRUE(nd_arena_used(&arena) >= sizeof(void *));

    metering_add_pulses(state, ND_ENERGY_FORWARD, 1, 4000);
    TEST_ASSERT_EQUAL_INT64(200, metering_read(state, ND_ENERGY_FORWARD, 1,
                                               ND_ENERGY_RES_0_01_KWH));

    metering_destroy(state, nd_arena_allocator(&arena));
    TEST_ASSERT_EQUAL_size_t(0, nd_arena_used(&arena));
}

static void test_persistence_roundtrip_through_legacy_bytes(void)
{
    metering_state_t *source = metering_create(nd_arena_allocator(&arena), &config);
    metering_add_pulses(source, ND_ENERGY_FORWARD, 3, 4017);
    metering_add_pulses(source, ND_ENERGY_REVERSE, 2, 37);

    nd_metering_snapshot_t snapshot;
    metering_export(source, &snapshot);

    legacy_energy_record_t record;
    memset(&record, 0, sizeof(record));
    legacy_energy_encode(&snapshot, &record);

    nd_metering_snapshot_t decoded;
    legacy_energy_decode(&record, &decoded);

    metering_state_t *restored = metering_create(nd_arena_allocator(&arena), &config);
    metering_import(restored, &decoded);

    TEST_ASSERT_EQUAL_INT64(metering_read(source, ND_ENERGY_FORWARD, 3,
                                          ND_ENERGY_RES_0_0001_KWH),
                            metering_read(restored, ND_ENERGY_FORWARD, 3,
                                          ND_ENERGY_RES_0_0001_KWH));
    TEST_ASSERT_EQUAL_INT64(metering_read(source, ND_ENERGY_REVERSE, 2,
                                          ND_ENERGY_RES_0_0001_KWH),
                            metering_read(restored, ND_ENERGY_REVERSE, 2,
                                          ND_ENERGY_RES_0_0001_KWH));

    metering_destroy(restored, nd_arena_allocator(&arena));
    metering_destroy(source, nd_arena_allocator(&arena));
    TEST_ASSERT_EQUAL_size_t(0, nd_arena_used(&arena));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wiring_arena_to_aggregate);
    RUN_TEST(test_persistence_roundtrip_through_legacy_bytes);
    return UNITY_END();
}
