#include "unity.h"

#include <string.h>

#include "domain/metering/metering.h"
#include "infra/memory/arena.h"
#include "infra/memory/byte_store.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"
#include "targets/stategrid_meter/persistence/legacy_energy_store.h"

static unsigned char flash[512];
static unsigned char memory[512];
static nd_byte_store_t store;
static nd_arena_t arena;
static metering_config_t config;
static metering_state_t *state;

void setUp(void)
{
    memset(flash, 0, sizeof(flash));
    nd_byte_store_init(&store, flash, sizeof(flash));
    nd_arena_init(&arena, memory, sizeof(memory));
    config.meter_const = 2000;
    config.cal_word = ND_CAL_ADD_POS | ND_CAL_ADD_NEG;
    config.save_step = 200;
    state = metering_create(nd_arena_allocator(&arena), &config);
}

void tearDown(void)
{
    metering_destroy(state, nd_arena_allocator(&arena));
}

static void test_save_writes_at_legacy_block_offset(void)
{
    metering_add_pulses(state, ND_ENERGY_FORWARD, 1, 4000);

    TEST_ASSERT_EQUAL_INT(0, legacy_energy_save(nd_byte_store_port(&store), 0, state));

    legacy_energy_record_t record;
    memcpy(&record, flash + ND_LEGACY_ENERGY_OFFSET_IN_BLOCK, sizeof(record));
    TEST_ASSERT_EQUAL_UINT32(200, record.active_p[0]);
    TEST_ASSERT_EQUAL_UINT32(200, record.active_p[1]);
}

static void test_save_preserves_checksums_and_padding(void)
{
    flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 156] = 0xDD;
    flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 157] = 0xCC;
    flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 158] = 0xBB;
    flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 159] = 0xAA;
    flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 162] = 0x5A;
    flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 163] = 0x5A;

    TEST_ASSERT_EQUAL_INT(0, legacy_energy_save(nd_byte_store_port(&store), 0, state));

    TEST_ASSERT_EQUAL_UINT8(0xDD, flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 156]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 159]);
    TEST_ASSERT_EQUAL_UINT8(0x5A, flash[ND_LEGACY_ENERGY_OFFSET_IN_BLOCK + 162]);
}

static void test_load_restores_energy(void)
{
    metering_add_pulses(state, ND_ENERGY_FORWARD, 2, 4017);
    metering_add_pulses(state, ND_ENERGY_REVERSE, 3, 37);
    TEST_ASSERT_EQUAL_INT(0, legacy_energy_save(nd_byte_store_port(&store), 64, state));

    metering_state_t *restored = metering_create(nd_arena_allocator(&arena), &config);
    TEST_ASSERT_EQUAL_INT(0, legacy_energy_load(nd_byte_store_port(&store), 64, restored));

    TEST_ASSERT_EQUAL_INT64(metering_read(state, ND_ENERGY_FORWARD, 2,
                                          ND_ENERGY_RES_0_0001_KWH),
                            metering_read(restored, ND_ENERGY_FORWARD, 2,
                                          ND_ENERGY_RES_0_0001_KWH));
    TEST_ASSERT_EQUAL_INT64(metering_read(state, ND_ENERGY_REVERSE, 3,
                                          ND_ENERGY_RES_0_0001_KWH),
                            metering_read(restored, ND_ENERGY_REVERSE, 3,
                                          ND_ENERGY_RES_0_0001_KWH));

    metering_destroy(restored, nd_arena_allocator(&arena));
}

static void test_load_near_end_of_store_fails(void)
{
    TEST_ASSERT_EQUAL_INT(-1, legacy_energy_load(nd_byte_store_port(&store), 400, state));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_save_writes_at_legacy_block_offset);
    RUN_TEST(test_save_preserves_checksums_and_padding);
    RUN_TEST(test_load_restores_energy);
    RUN_TEST(test_load_near_end_of_store_fails);
    return UNITY_END();
}
