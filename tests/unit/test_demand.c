#include "unity.h"

#include "domain/metering/demand.h"
#include "support/fake_allocator.h"

static unsigned char arena[512];
static fake_allocator_t fake;

void setUp(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
}

void tearDown(void) {}

static void run_minute(nd_demand_t *demand, int32_t power)
{
    for (int second = 0; second < 60; second++) {
        nd_demand_on_second(demand, power);
    }
    nd_demand_on_minute(demand);
}

static nd_demand_config_t default_config(void)
{
    nd_demand_config_t config = {3, 1, 3, 0};
    return config;
}

static void test_create_rejects_invalid_configuration(void)
{
    nd_demand_config_t config = default_config();

    TEST_ASSERT_NULL(nd_demand_create(&fake.port, NULL));

    config.max_slide = 0;
    TEST_ASSERT_NULL(nd_demand_create(&fake.port, &config));

    config = default_config();
    config.sliding = 0;
    TEST_ASSERT_NULL(nd_demand_create(&fake.port, &config));

    config = default_config();
    config.slide_num = 0;
    TEST_ASSERT_NULL(nd_demand_create(&fake.port, &config));

    config = default_config();
    config.slide_num = 4;
    TEST_ASSERT_NULL(nd_demand_create(&fake.port, &config));
}

static void test_create_and_destroy_use_the_allocator(void)
{
    nd_demand_config_t config = default_config();

    nd_demand_t *demand = nd_demand_create(&fake.port, &config);
    TEST_ASSERT_NOT_NULL(demand);
    TEST_ASSERT_EQUAL_size_t(1, fake.alloc_calls);

    nd_demand_destroy(demand, &fake.port);
    TEST_ASSERT_EQUAL_size_t(1, fake.dealloc_calls);
}

static void test_demand_ramps_up_over_the_window(void)
{
    nd_demand_config_t config = default_config();
    nd_demand_t *demand = nd_demand_create(&fake.port, &config);

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(33, nd_demand_value(demand));

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(66, nd_demand_value(demand));

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(100, nd_demand_value(demand));

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(100, nd_demand_value(demand));

    nd_demand_destroy(demand, &fake.port);
}

static void test_demand_averages_the_most_recent_slides(void)
{
    nd_demand_config_t config = default_config();
    nd_demand_t *demand = nd_demand_create(&fake.port, &config);

    run_minute(demand, 60);
    TEST_ASSERT_EQUAL_UINT32(20, nd_demand_value(demand));

    run_minute(demand, 120);
    TEST_ASSERT_EQUAL_UINT32(60, nd_demand_value(demand));

    run_minute(demand, 180);
    TEST_ASSERT_EQUAL_UINT32(120, nd_demand_value(demand));

    nd_demand_destroy(demand, &fake.port);
}

static void test_negative_power_counts_by_magnitude(void)
{
    nd_demand_config_t config = default_config();
    nd_demand_t *demand = nd_demand_create(&fake.port, &config);

    run_minute(demand, -60);

    TEST_ASSERT_EQUAL_UINT32(20, nd_demand_value(demand));
    nd_demand_destroy(demand, &fake.port);
}

static void test_mode_one_waits_for_a_full_period(void)
{
    nd_demand_config_t config = default_config();
    config.mode = 1;
    nd_demand_t *demand = nd_demand_create(&fake.port, &config);

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(0, nd_demand_value(demand));

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(0, nd_demand_value(demand));

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_UINT32(100, nd_demand_value(demand));

    nd_demand_destroy(demand, &fake.port);
}

static void test_snapshot_reset_and_next_period(void)
{
    nd_demand_config_t config = default_config();
    nd_demand_t *demand = nd_demand_create(&fake.port, &config);
    nd_demand_snapshot_t snapshot;

    run_minute(demand, 100);
    TEST_ASSERT_EQUAL_INT(0, nd_demand_snapshot(demand, &snapshot));
    TEST_ASSERT_EQUAL_UINT32(33, snapshot.current_value);
    TEST_ASSERT_EQUAL_UINT32(60, snapshot.period_seconds);
    TEST_ASSERT_EQUAL_UINT16(3, snapshot.periods);

    TEST_ASSERT_EQUAL_INT(0, nd_demand_reset(demand));
    TEST_ASSERT_EQUAL_UINT32(0, nd_demand_value(demand));
    TEST_ASSERT_EQUAL_INT(0, nd_demand_snapshot(demand, &snapshot));
    TEST_ASSERT_EQUAL_UINT32(0, snapshot.current_value);

    TEST_ASSERT_EQUAL_INT(ND_DEMAND_BOTH, nd_demand_next_period(demand));
    nd_demand_destroy(demand, &fake.port);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_rejects_invalid_configuration);
    RUN_TEST(test_create_and_destroy_use_the_allocator);
    RUN_TEST(test_demand_ramps_up_over_the_window);
    RUN_TEST(test_demand_averages_the_most_recent_slides);
    RUN_TEST(test_negative_power_counts_by_magnitude);
    RUN_TEST(test_mode_one_waits_for_a_full_period);
    RUN_TEST(test_snapshot_reset_and_next_period);
    return UNITY_END();
}
