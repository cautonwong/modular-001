#include "unity.h"

#include <string.h>

#include "infra/memory/arena.h"
#include "targets/stategrid_meter/app/metering/demand_adapter.h"

/* The first tick lands on a minute boundary so each window is exactly five
 * whole minutes. */
#define BASE 999960u
#define AT(seconds) (BASE + (seconds))

/* Goldens are the 28 B maxima block whose layout is nd_demand_persist_t in
 * targets/stategrid_meter/app/metering/demand_adapter.c: one version byte, one
 * reserved byte, a little-endian CRC-16/X-25 over the block with the CRC field
 * zero, then the max value, max fee and max time arrays for both demand
 * objects. record-retrieval.md only records that the block occupies 28 B of
 * persistent_blank_space. The CRC is derived at the documented offsets, not
 * from a firmware run. */
static const uint8_t GOLDEN_DEMAND[28] = {
    0x01, 0x00, 0x9C, 0x97, 0xE8, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00,
    0xE8, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x44, 0x43, 0x0F, 0x00,
    0x44, 0x43, 0x0F, 0x00,
};
static const uint8_t GOLDEN_DEMAND_V0[28] = {
    0x00, 0x00, 0x82, 0x59, 0xE8, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00,
    0xE8, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x44, 0x43, 0x0F, 0x00,
    0x44, 0x43, 0x0F, 0x00,
};

static unsigned char arena_buffer[512];
static nd_arena_t arena;
static int32_t power;
static nd_demand_adapter_t *adapter;

void setUp(void)
{
    nd_arena_init(&arena, arena_buffer, sizeof(arena_buffer));
    power = 1000;
    adapter = nd_demand_adapter_create(nd_arena_allocator(&arena), &power);
    TEST_ASSERT_NOT_NULL(adapter);
}

void tearDown(void)
{
    nd_demand_adapter_destroy(adapter, nd_arena_allocator(&arena));
}

static void run_seconds(uint32_t start, uint32_t count)
{
    for (uint32_t index = 0; index < count; index++) {
        nd_demand_adapter_on_second(adapter, start + index);
    }
}

static const nd_demand_view_t *view(uint32_t oad)
{
    const nd_demand_view_t *found = nd_demand_adapter_view(adapter, oad);
    TEST_ASSERT_NOT_NULL(found);
    return found;
}

static void test_both_configs_publish_after_five_minutes(void)
{
    run_seconds(AT(0), 300);
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_INTERVAL)->value);
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_SLIDING)->value);

    nd_demand_adapter_on_second(adapter, AT(300));

    const nd_demand_view_t *interval = view(ND_DEMAND_OAD_INTERVAL);
    const nd_demand_view_t *sliding = view(ND_DEMAND_OAD_SLIDING);
    TEST_ASSERT_EQUAL_UINT32(1000, interval->value);
    TEST_ASSERT_EQUAL_UINT32(1000, interval->fee_value);
    TEST_ASSERT_EQUAL_UINT32(1000, interval->max_value);
    TEST_ASSERT_EQUAL_UINT32(1000, interval->max_fee_value);
    TEST_ASSERT_EQUAL_UINT32(AT(300), interval->max_time);
    TEST_ASSERT_EQUAL_UINT32(AT(300), interval->start_time);
    TEST_ASSERT_EQUAL_UINT32(1000, sliding->value);
    TEST_ASSERT_EQUAL_UINT32(1000, sliding->max_value);
}

static void test_interval_and_sliding_differ_after_a_step(void)
{
    run_seconds(AT(0), 300);
    power = 2000;
    nd_demand_adapter_on_second(adapter, AT(300));
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->value);
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_SLIDING)->value);

    run_seconds(AT(301), 59);
    nd_demand_adapter_on_second(adapter, AT(360));
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->value);
    TEST_ASSERT_EQUAL_UINT32(1200, view(ND_DEMAND_OAD_SLIDING)->value);

    run_seconds(AT(361), 239);
    nd_demand_adapter_on_second(adapter, AT(600));
    TEST_ASSERT_EQUAL_UINT32(2000, view(ND_DEMAND_OAD_INTERVAL)->value);
    TEST_ASSERT_EQUAL_UINT32(2000, view(ND_DEMAND_OAD_SLIDING)->value);
    TEST_ASSERT_EQUAL_UINT32(2000, view(ND_DEMAND_OAD_INTERVAL)->max_value);
    TEST_ASSERT_EQUAL_UINT32(AT(600), view(ND_DEMAND_OAD_INTERVAL)->max_time);
}

static void test_a_tie_keeps_the_earlier_maximum_time(void)
{
    run_seconds(AT(0), 301);
    TEST_ASSERT_EQUAL_UINT32(AT(300), view(ND_DEMAND_OAD_INTERVAL)->max_time);

    run_seconds(AT(301), 300);
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->max_value);
    TEST_ASSERT_EQUAL_UINT32(AT(300), view(ND_DEMAND_OAD_INTERVAL)->max_time);
}

static void test_a_duplicate_second_does_not_restart_the_window(void)
{
    run_seconds(AT(0), 61);
    nd_demand_adapter_on_second(adapter, AT(60));
    run_seconds(AT(61), 239);
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_INTERVAL)->value);

    nd_demand_adapter_on_second(adapter, AT(300));
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->value);
}

static void test_a_time_set_restarts_the_window_and_keeps_the_maximum(void)
{
    run_seconds(AT(0), 301);
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->max_value);

    uint32_t jumped = AT(300) + 3600u;
    nd_demand_adapter_on_second(adapter, jumped);
    const nd_demand_view_t *interval = view(ND_DEMAND_OAD_INTERVAL);
    TEST_ASSERT_EQUAL_UINT32(0, interval->value);
    TEST_ASSERT_EQUAL_UINT32(1000, interval->max_value);
    TEST_ASSERT_EQUAL_UINT32(jumped, interval->start_time);
}

static void test_restore_keeps_the_maxima_and_resets_the_current_value(void)
{
    unsigned char blank[36];
    const uint32_t restore_time = 2000000u;

    run_seconds(AT(0), 301);
    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_save(adapter, blank, sizeof(blank)));

    nd_demand_adapter_destroy(adapter, nd_arena_allocator(&arena));
    nd_arena_reset(&arena);
    power = 1000;
    adapter = nd_demand_adapter_create(nd_arena_allocator(&arena), &power);
    TEST_ASSERT_NOT_NULL(adapter);

    TEST_ASSERT_EQUAL_INT(1, nd_demand_adapter_restore(adapter, blank,
                                                       sizeof(blank),
                                                       restore_time));
    const nd_demand_view_t *interval = view(ND_DEMAND_OAD_INTERVAL);
    TEST_ASSERT_EQUAL_UINT32(0, interval->value);
    TEST_ASSERT_EQUAL_UINT32(1000, interval->max_value);
    TEST_ASSERT_EQUAL_UINT32(AT(300), interval->max_time);
    TEST_ASSERT_EQUAL_UINT32(restore_time, interval->start_time);
}

static void test_blank_and_corrupt_persistence_load_zero_maxima(void)
{
    unsigned char blank[36];
    memset(blank, 0xFF, sizeof(blank));

    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_restore(adapter, blank,
                                                       sizeof(blank), AT(0)));
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_INTERVAL)->max_value);

    run_seconds(AT(0), 301);
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->max_value);
    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_save(adapter, blank, sizeof(blank)));
    blank[4] ^= 0x5A;
    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_restore(adapter, blank,
                                                       sizeof(blank), AT(0)));
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_INTERVAL)->max_value);
}

static void test_clear_zeroes_everything(void)
{
    run_seconds(AT(0), 301);
    nd_demand_adapter_clear(adapter, AT(400));

    const nd_demand_view_t *interval = view(ND_DEMAND_OAD_INTERVAL);
    TEST_ASSERT_EQUAL_UINT32(0, interval->value);
    TEST_ASSERT_EQUAL_UINT32(0, interval->fee_value);
    TEST_ASSERT_EQUAL_UINT32(0, interval->max_value);
    TEST_ASSERT_EQUAL_UINT32(0, interval->max_fee_value);
    TEST_ASSERT_EQUAL_UINT32(0, interval->max_time);
    TEST_ASSERT_EQUAL_UINT32(AT(400), interval->start_time);
}

static void test_unknown_oad_and_invalid_arguments_are_rejected(void)
{
    TEST_ASSERT_NULL(nd_demand_adapter_view(adapter, 0x20040200u));
    TEST_ASSERT_NULL(nd_demand_adapter_view(NULL, ND_DEMAND_OAD_INTERVAL));
    TEST_ASSERT_NULL(nd_demand_adapter_create(NULL, &power));
    TEST_ASSERT_NULL(nd_demand_adapter_create(nd_arena_allocator(&arena), NULL));
}

static void test_saved_block_matches_the_documented_golden(void)
{
    unsigned char blank[36];
    const uint32_t restore_time = 2000000u;

    memset(blank, 0xEE, sizeof(blank));
    TEST_ASSERT_EQUAL_INT(1, nd_demand_adapter_restore(adapter, GOLDEN_DEMAND,
                                                       sizeof(GOLDEN_DEMAND),
                                                       restore_time));
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_INTERVAL)->max_value);
    TEST_ASSERT_EQUAL_UINT32(1000260, view(ND_DEMAND_OAD_INTERVAL)->max_time);
    TEST_ASSERT_EQUAL_UINT32(1000, view(ND_DEMAND_OAD_SLIDING)->max_value);
    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_save(adapter, blank, sizeof(blank)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_DEMAND, blank, sizeof(GOLDEN_DEMAND));
    for (uint32_t index = sizeof(GOLDEN_DEMAND); index < sizeof(blank); index++) {
        TEST_ASSERT_EQUAL_HEX8(0xEE, blank[index]);
    }
}

static void test_operating_maxima_match_the_documented_golden(void)
{
    unsigned char blank[36];

    run_seconds(AT(0), 301);
    memset(blank, 0xEE, sizeof(blank));
    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_save(adapter, blank, sizeof(blank)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_DEMAND, blank, sizeof(GOLDEN_DEMAND));
}

static void test_old_version_block_loads_zero_maxima(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_demand_adapter_restore(adapter,
                                                       GOLDEN_DEMAND_V0,
                                                       sizeof(GOLDEN_DEMAND_V0),
                                                       AT(0)));
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_INTERVAL)->max_value);
    TEST_ASSERT_EQUAL_UINT32(0, view(ND_DEMAND_OAD_SLIDING)->max_value);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_both_configs_publish_after_five_minutes);
    RUN_TEST(test_interval_and_sliding_differ_after_a_step);
    RUN_TEST(test_a_tie_keeps_the_earlier_maximum_time);
    RUN_TEST(test_a_duplicate_second_does_not_restart_the_window);
    RUN_TEST(test_a_time_set_restarts_the_window_and_keeps_the_maximum);
    RUN_TEST(test_restore_keeps_the_maxima_and_resets_the_current_value);
    RUN_TEST(test_blank_and_corrupt_persistence_load_zero_maxima);
    RUN_TEST(test_clear_zeroes_everything);
    RUN_TEST(test_unknown_oad_and_invalid_arguments_are_rejected);
    RUN_TEST(test_saved_block_matches_the_documented_golden);
    RUN_TEST(test_operating_maxima_match_the_documented_golden);
    RUN_TEST(test_old_version_block_loads_zero_maxima);
    return UNITY_END();
}
