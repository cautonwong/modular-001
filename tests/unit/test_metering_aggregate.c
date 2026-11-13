#include "unity.h"

#include "domain/metering/metering.h"
#include "support/fake_allocator.h"

static unsigned char arena[512];
static fake_allocator_t fake;
static metering_config_t config;

void setUp(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
    config.meter_const = 2000;
    config.cal_word = ND_CAL_ADD_POS | ND_CAL_ADD_NEG;
    config.save_step = 200;
}

void tearDown(void) {}

static void test_create_uses_injected_allocator(void)
{
    metering_state_t *state = metering_create(&fake.port, &config);

    TEST_ASSERT_NOT_NULL(state);
    TEST_ASSERT_EQUAL_size_t(1, fake.alloc_calls);
    metering_destroy(state, &fake.port);
    TEST_ASSERT_EQUAL_size_t(1, fake.dealloc_calls);
}

static void test_create_rejects_missing_inputs(void)
{
    TEST_ASSERT_NULL(metering_create(NULL, &config));
    TEST_ASSERT_NULL(metering_create(&fake.port, NULL));
}

static void test_create_reports_exhausted_arena(void)
{
    fake_allocator_init(&fake, arena, 4);

    TEST_ASSERT_NULL(metering_create(&fake.port, &config));
}

static void test_aggregate_accumulates_and_reads(void)
{
    metering_state_t *state = metering_create(&fake.port, &config);

    metering_add_pulses(state, ND_ENERGY_FORWARD, 0, 4000);

    TEST_ASSERT_EQUAL_INT64(200, metering_read(state, ND_ENERGY_FORWARD, 0,
                                               ND_ENERGY_RES_0_01_KWH));
    metering_destroy(state, &fake.port);
}

static void test_export_import_roundtrip(void)
{
    metering_state_t *source = metering_create(&fake.port, &config);
    metering_add_pulses(source, ND_ENERGY_FORWARD, 4, 4020);

    nd_metering_snapshot_t snapshot;
    metering_export(source, &snapshot);

    metering_state_t *target = metering_create(&fake.port, &config);
    metering_import(target, &snapshot);

    TEST_ASSERT_EQUAL_INT64(metering_read(source, ND_ENERGY_FORWARD, 4,
                                          ND_ENERGY_RES_0_0001_KWH),
                            metering_read(target, ND_ENERGY_FORWARD, 4,
                                          ND_ENERGY_RES_0_0001_KWH));

    metering_destroy(source, &fake.port);
    metering_destroy(target, &fake.port);
}

static void test_clear_and_set_total(void)
{
    metering_state_t *state = metering_create(&fake.port, &config);
    metering_add_pulses(state, ND_ENERGY_FORWARD, 2, 4000);

    metering_clear(state);
    TEST_ASSERT_EQUAL_INT64(0, metering_read(state, ND_ENERGY_FORWARD, 0,
                                             ND_ENERGY_RES_0_01_KWH));

    metering_set_total(state, ND_ENERGY_FORWARD, 1234);
    TEST_ASSERT_EQUAL_INT64(1234, metering_read(state, ND_ENERGY_FORWARD, 0,
                                                ND_ENERGY_RES_0_01_KWH));

    metering_destroy(state, &fake.port);
}

static void test_configure_updates_the_energy_parameters(void)
{
    metering_state_t *state = metering_create(&fake.port, &config);

    metering_config_t changed = config;
    changed.meter_const = 1000;
    metering_configure(state, &changed);
    metering_add_pulses(state, ND_ENERGY_FORWARD, 0, 20);
    TEST_ASSERT_EQUAL_INT64(2, metering_read(state, ND_ENERGY_FORWARD, 0,
                                             ND_ENERGY_RES_0_01_KWH));

    metering_configure(state, NULL);
    metering_destroy(state, &fake.port);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_uses_injected_allocator);
    RUN_TEST(test_create_rejects_missing_inputs);
    RUN_TEST(test_create_reports_exhausted_arena);
    RUN_TEST(test_aggregate_accumulates_and_reads);
    RUN_TEST(test_export_import_roundtrip);
    RUN_TEST(test_clear_and_set_total);
    RUN_TEST(test_configure_updates_the_energy_parameters);
    return UNITY_END();
}
