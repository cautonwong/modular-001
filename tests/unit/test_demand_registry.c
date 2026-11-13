#include "unity.h"

#include "application/metering/demand_registry.h"
#include "support/fake_allocator.h"

static unsigned char arena[512];
static fake_allocator_t fake;

void setUp(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
}

void tearDown(void) {}

typedef struct {
    uint32_t oad;
    nd_demand_update_t update;
    uint32_t value;
    uint32_t fee_value;
} update_record_t;

typedef struct {
    update_record_t records[4];
    uint8_t count;
} update_log_t;

static void on_update(void *ctx, uint32_t oad, nd_demand_update_t update,
                      uint32_t value, uint32_t fee_value)
{
    update_log_t *log = ctx;
    update_record_t *record = &log->records[log->count++];

    record->oad = oad;
    record->update = update;
    record->value = value;
    record->fee_value = fee_value;
}

static void accumulate_seconds(nd_demand_registry_t *registry, uint8_t minutes)
{
    for (uint8_t minute = 0; minute < minutes; minute++) {
        for (uint8_t second = 0; second < 60; second++) {
            nd_demand_registry_on_second(registry);
        }
    }
}

static void test_registry_reports_each_entry_with_its_oad(void)
{
    int32_t forward_power = 100;
    int32_t reverse_power = -60;
    nd_demand_entry_config_t entries[2] = {
        {&forward_power, 0x20210200, {3, 1, 3, 0}},
        {&reverse_power, 0x20210201, {3, 1, 3, 0}},
    };
    nd_demand_registry_t *registry =
        nd_demand_registry_create(&fake.port, entries, 2);
    TEST_ASSERT_NOT_NULL(registry);

    accumulate_seconds(registry, 1);
    update_log_t log = {0};
    nd_demand_registry_on_minute(registry, on_update, &log);

    TEST_ASSERT_EQUAL_UINT8(2, log.count);
    TEST_ASSERT_EQUAL_UINT32(0x20210200, log.records[0].oad);
    TEST_ASSERT_EQUAL_UINT32(33, log.records[0].value);
    TEST_ASSERT_EQUAL_UINT32(0x20210201, log.records[1].oad);
    TEST_ASSERT_EQUAL_UINT32(20, log.records[1].value);

    nd_demand_registry_destroy(registry, &fake.port);
}

static void test_mode_one_entry_waits_for_the_full_window(void)
{
    int32_t power = 100;
    nd_demand_config_t config = {3, 1, 3, 1};
    nd_demand_entry_config_t entry = {&power, 0x20210200, config};
    nd_demand_registry_t *registry =
        nd_demand_registry_create(&fake.port, &entry, 1);
    TEST_ASSERT_NOT_NULL(registry);

    update_log_t log = {0};
    accumulate_seconds(registry, 1);
    nd_demand_registry_on_minute(registry, on_update, &log);
    TEST_ASSERT_EQUAL_UINT8(0, log.count);

    accumulate_seconds(registry, 2);
    nd_demand_registry_on_minute(registry, on_update, &log);
    TEST_ASSERT_EQUAL_UINT8(0, log.count);

    accumulate_seconds(registry, 1);
    nd_demand_registry_on_minute(registry, on_update, &log);
    TEST_ASSERT_EQUAL_UINT8(1, log.count);
    TEST_ASSERT_EQUAL_UINT32(100, log.records[0].value);

    nd_demand_registry_destroy(registry, &fake.port);
}

static void test_registry_uses_the_injected_allocator(void)
{
    int32_t power = 100;
    nd_demand_entry_config_t entry = {&power, 0x20210200, {3, 1, 3, 0}};

    nd_demand_registry_t *registry =
        nd_demand_registry_create(&fake.port, &entry, 1);
    TEST_ASSERT_NOT_NULL(registry);
    TEST_ASSERT_EQUAL_size_t(2, fake.alloc_calls);

    nd_demand_registry_destroy(registry, &fake.port);
    TEST_ASSERT_EQUAL_size_t(2, fake.dealloc_calls);
}

static void test_registry_rejects_invalid_arguments(void)
{
    nd_demand_entry_config_t entry = {NULL, 0, {3, 1, 3, 0}};

    TEST_ASSERT_NULL(nd_demand_registry_create(NULL, &entry, 1));
    TEST_ASSERT_NULL(nd_demand_registry_create(&fake.port, NULL, 1));
    TEST_ASSERT_NULL(nd_demand_registry_create(&fake.port, &entry, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_registry_reports_each_entry_with_its_oad);
    RUN_TEST(test_mode_one_entry_waits_for_the_full_window);
    RUN_TEST(test_registry_uses_the_injected_allocator);
    RUN_TEST(test_registry_rejects_invalid_arguments);
    return UNITY_END();
}
