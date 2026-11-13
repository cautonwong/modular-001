#include "unity.h"

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profile_table.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static uint8_t flash[ND_IDIS_PROFILE_FLASH_BYTES];
static nd_byte_store_t store;
static nd_idis_objects_t objects;
static nd_cosem_port_t port;

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 11, .day = 11};
    return 0;
}

static int fake_measurement(void *ctx, nd_measure_quantity_t quantity,
                            uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    *out = (nd_measurement_value_t){
        .value = quantity == ND_MEASURE_FREQUENCY ? 5000 : 2300,
        .scaler = quantity == ND_MEASURE_FREQUENCY ? -2 : -1,
        .unit = quantity == ND_MEASURE_FREQUENCY ? ND_MEASUREMENT_UNIT_HERTZ
                                                 : ND_MEASUREMENT_UNIT_VOLT,
        .phase = phase,
    };
    return 0;
}

void setUp(void)
{
    nd_byte_store_init(&store, flash, sizeof(flash));
}

void tearDown(void) {}

static void build(nd_idis_profile_config_t *configs, nd_clock_port_t *clock)
{
    *clock = (nd_clock_port_t){.ctx = NULL, .read = fake_clock,
                               .write = NULL};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_build(
                              configs, ND_IDIS_PROFILE_TABLE_COUNT,
                              nd_byte_store_port(&store), clock));
}

static void test_table_covers_all_pack3_profiles(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    const nd_obis_t expected[] = {
        ND_OBIS_BILLING_PROFILE,     ND_OBIS_LOAD_PROFILE_1,
        ND_OBIS_LOAD_PROFILE_2,      ND_OBIS_PQ_PROFILE,
        ND_OBIS_STANDARD_EVENT_LOG,  ND_OBIS_FRAUD_LOG,
        ND_OBIS_DISCONNECTOR_LOG,    ND_OBIS_MBUS_EVENT_LOG,
        ND_OBIS_PQ_LOG,              ND_OBIS_COMM_LOG,
        ND_OBIS_COMM_DETAILS_LOG,    ND_OBIS_POWER_FAILURE_LOG,
        ND_OBIS_MBUS_LOAD_PROFILE_1, ND_OBIS_MBUS_LOAD_PROFILE_2,
        ND_OBIS_MBUS_LOAD_PROFILE_3, ND_OBIS_MBUS_LOAD_PROFILE_4,
        ND_OBIS_MBUS_CONTROL_LOG_1,  ND_OBIS_MBUS_CONTROL_LOG_2,
        ND_OBIS_MBUS_CONTROL_LOG_3,  ND_OBIS_MBUS_CONTROL_LOG_4,
        ND_OBIS_DISPLAY_GENERAL,     ND_OBIS_DISPLAY_ALTERNATE,
    };
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        TEST_ASSERT_TRUE(nd_obis_eq(&configs[index].ln, &expected[index]));
        TEST_ASSERT_TRUE(configs[index].capture_count > 0);
        TEST_ASSERT_TRUE(configs[index].clock != NULL);
        TEST_ASSERT_TRUE(configs[index].store != NULL);
        TEST_ASSERT_TRUE(configs[index].capacity > 0);
        TEST_ASSERT_TRUE(configs[index].slot_size > 0);
    }
    TEST_ASSERT_EQUAL_UINT32(900, configs[1].capture_period);
    TEST_ASSERT_EQUAL_UINT32(86400, configs[2].capture_period);
    TEST_ASSERT_EQUAL_UINT32(60, configs[3].capture_period);
    TEST_ASSERT_EQUAL_UINT8(12, configs[3].capture_count);
}

static void test_table_regions_do_not_overlap(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    for (uint8_t left = 0; left < ND_IDIS_PROFILE_TABLE_COUNT; left++) {
        uint32_t left_end = configs[left].base_offset + 64
                          + (uint32_t)configs[left].capacity
                                * configs[left].slot_size;
        for (uint8_t right = left + 1; right < ND_IDIS_PROFILE_TABLE_COUNT;
             right++) {
            uint32_t right_end = configs[right].base_offset + 64
                               + (uint32_t)configs[right].capacity
                                     * configs[right].slot_size;
            TEST_ASSERT_TRUE(left_end <= configs[right].base_offset
                             || right_end <= configs[left].base_offset);
            TEST_ASSERT_FALSE(nd_obis_eq(&configs[left].ln,
                                         &configs[right].ln));
        }
        TEST_ASSERT_TRUE(left_end <= ND_IDIS_PROFILE_FLASH_BYTES);
    }
}

static void test_profiles_init_accepts_table(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, configs,
                                                   ND_IDIS_PROFILE_TABLE_COUNT));
    TEST_ASSERT_EQUAL_UINT8(ND_IDIS_PROFILE_TABLE_COUNT, objects.profiles.count);
    TEST_ASSERT_NOT_NULL(nd_idis_profiles_find(&objects.profiles,
                                               &ND_OBIS_BILLING_PROFILE));
    TEST_ASSERT_NOT_NULL(nd_idis_profiles_find(&objects.profiles,
                                               &ND_OBIS_PQ_PROFILE));
    TEST_ASSERT_NOT_NULL(nd_idis_profiles_find(&objects.profiles,
                                               &ND_OBIS_DISPLAY_ALTERNATE));
}

static void test_every_profile_captures(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);

    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    objects = (nd_idis_objects_t){0};
    objects.clock.port = &clock;
    objects.reg.measurement = &measurement;
    objects.reg.energy = &objects.energy;
    nd_energy_set_init(&objects.energy, 2000);
    (void)nd_energy_set_add_pulses(&objects.energy,
                                   ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 0, 4000);
    nd_idis_average_init(&objects.average, &measurement);
    port = (nd_cosem_port_t){
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, configs,
                                                   ND_IDIS_PROFILE_TABLE_COUNT));
    nd_idis_profiles_bind_port(&objects.profiles, &port);

    nd_clock_value_t now = {
        .time = {.year = 2026, .month = 11, .day = 11, .hour = 9},
    };
    for (uint8_t index = 0; index < objects.profiles.count; index++) {
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(
                                  &objects.profiles.items[index], &port, 16,
                                  &now));
        TEST_ASSERT_EQUAL_UINT16(
            1, nd_profile_store_entries(&objects.profiles.items[index].store));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_table_covers_all_pack3_profiles);
    RUN_TEST(test_table_regions_do_not_overlap);
    RUN_TEST(test_profiles_init_accepts_table);
    RUN_TEST(test_every_profile_captures);
    return UNITY_END();
}
