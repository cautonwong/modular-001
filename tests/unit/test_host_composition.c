#include "unity.h"

#include <string.h>

#include "infra/memory/arena.h"
#include "infra/memory/byte_store.h"
#include "infra/memory/parameter_store.h"
#include "targets/host/composition.h"

static unsigned char arena[4096];
static nd_arena_t memory;
static unsigned char flash[512];
static nd_byte_store_t energy_store;

static uint8_t parameters[5];
static uint8_t weekend[1];
static uint8_t weekend_fee[1];
static uint8_t season0[3];
static uint8_t day0[3];
static nd_param_record_t records[5];
static nd_param_table_t parameter_table;

static host_config_t config;
static host_composition_t *composition;

void setUp(void)
{
    memset(flash, 0, sizeof(flash));
    nd_byte_store_init(&energy_store, flash, sizeof(flash));

    parameters[0] = 1; /* seasons */
    parameters[1] = 1; /* day tables */
    parameters[2] = 1; /* periods */
    parameters[3] = 1; /* fees */
    parameters[4] = 0; /* holidays */
    weekend[0] = 0xFF;
    weekend_fee[0] = 1;
    season0[0] = 1; season0[1] = 1; season0[2] = 1;
    day0[0] = 0; day0[1] = 0; day0[2] = 0x01;

    records[0] = (nd_param_record_t){916, parameters, sizeof(parameters)};
    records[1] = (nd_param_record_t){922, weekend, sizeof(weekend)};
    records[2] = (nd_param_record_t){923, weekend_fee, sizeof(weekend_fee)};
    records[3] = (nd_param_record_t){924, season0, sizeof(season0)};
    records[4] = (nd_param_record_t){952, day0, sizeof(day0)};
    nd_param_table_init(&parameter_table, records, 5);

    nd_arena_init(&memory, arena, sizeof(arena));
    config.energy_store = nd_byte_store_port(&energy_store);
    config.energy_block_offset = 0;
    config.parameters = nd_param_table_port(&parameter_table);
    config.metering.meter_const = 2000;
    config.metering.cal_word = ND_CAL_ADD_POS | ND_CAL_ADD_NEG;
    config.metering.save_step = 200;
    config.demand.max_slide = 3;
    config.demand.sliding = 1;
    config.demand.slide_num = 3;
    config.demand.mode = 0;

    composition = host_composition_create(nd_arena_allocator(&memory), &config);
}

void tearDown(void)
{
    if (composition != NULL) {
        host_composition_destroy(composition, nd_arena_allocator(&memory));
    }
}

static host_tick_t base_tick(void)
{
    host_tick_t tick;
    memset(&tick, 0, sizeof(tick));
    tick.direction = ND_ENERGY_FORWARD;
    tick.pulses = 20;
    tick.power = 100;
    tick.load.factory_mode = 1;
    tick.load.voltage = 220;
    tick.load.volt_low = 100;
    tick.load.volt_high = 300;
    tick.load.protection_current = 600;
    tick.load.protection_minutes = 5;
    return tick;
}

static void run_one_minute(host_composition_t *target)
{
    host_tick_t tick = base_tick();
    for (uint8_t second = 0; second < 60; second++) {
        tick.time.second = second;
        tick.time.hour = 0;
        tick.time.minute = 0;
        tick.time.day = 1;
        tick.time.month = 1;
        tick.time.year = 2024;
        host_composition_second(target, &tick);
    }
}

static void test_create_rejects_incomplete_configuration(void)
{
    TEST_ASSERT_NOT_NULL(composition);
    TEST_ASSERT_NULL(host_composition_create(nd_arena_allocator(&memory), NULL));

    host_config_t incomplete = config;
    incomplete.energy_store = NULL;
    TEST_ASSERT_NULL(host_composition_create(nd_arena_allocator(&memory), &incomplete));
}

static void test_boot_starts_empty_and_connected(void)
{
    TEST_ASSERT_EQUAL_INT(0, host_composition_boot(composition));

    host_status_t status;
    host_composition_status(composition, &status);
    TEST_ASSERT_EQUAL_UINT8(1, status.relay_connected);
    TEST_ASSERT_EQUAL_INT64(0, status.energy_0_01_kwh);
}

static void test_one_minute_wires_every_aggregate(void)
{
    TEST_ASSERT_EQUAL_INT(0, host_composition_boot(composition));
    host_composition_set_load_command(composition, ND_LOAD_REMOTE_ON);

    run_one_minute(composition);

    host_status_t status;
    host_composition_status(composition, &status);
    TEST_ASSERT_EQUAL_UINT8(1, status.fee);
    TEST_ASSERT_EQUAL_UINT8(1, status.day_table);
    TEST_ASSERT_EQUAL_UINT32(33, status.demand);
    TEST_ASSERT_EQUAL_INT64(60, status.energy_0_01_kwh);
    TEST_ASSERT_EQUAL_UINT8(1, status.relay_connected);
}

static void test_save_and_reboot_restore_energy(void)
{
    TEST_ASSERT_EQUAL_INT(0, host_composition_boot(composition));
    host_composition_set_load_command(composition, ND_LOAD_REMOTE_ON);
    run_one_minute(composition);
    TEST_ASSERT_EQUAL_INT(0, host_composition_save(composition));

    host_composition_destroy(composition, nd_arena_allocator(&memory));
    composition = host_composition_create(nd_arena_allocator(&memory), &config);
    TEST_ASSERT_NOT_NULL(composition);
    TEST_ASSERT_EQUAL_INT(0, host_composition_boot(composition));

    host_status_t status;
    host_composition_status(composition, &status);
    TEST_ASSERT_EQUAL_INT64(60, status.energy_0_01_kwh);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_rejects_incomplete_configuration);
    RUN_TEST(test_boot_starts_empty_and_connected);
    RUN_TEST(test_one_minute_wires_every_aggregate);
    RUN_TEST(test_save_and_reboot_restore_energy);
    return UNITY_END();
}
