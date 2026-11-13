#include "unity.h"
#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    objects.demand[0].current = (nd_demand_snapshot_t){
        .current_value = 100,
        .current_fee_value = 200,
        .period_seconds = 900,
        .periods = 15,
    };
    objects.demand[0].history = (nd_demand_history_t){
        .last_value = 50,
        .last_fee_value = 60,
        .last_capture_time = {.year = 2026, .month = 11, .day = 11},
        .period_start = {.year = 2026, .month = 11, .day = 11},
        .status = 4,
    };
}

void tearDown(void) {}

static uint32_t read_u32(uint8_t attribute_id)
{
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER,
                                            &ND_OBIS_DEMAND_ACTIVE_IMPORT,
                                            attribute_id, NULL, &out));
    return ((uint32_t)buffer[1] << 24) | ((uint32_t)buffer[2] << 16)
        | ((uint32_t)buffer[3] << 8) | buffer[4];
}

static void test_demand_attributes(void)
{
    TEST_ASSERT_EQUAL_UINT32(100, read_u32(2));
    TEST_ASSERT_EQUAL_UINT32(50, read_u32(3));
    TEST_ASSERT_EQUAL_UINT32(900, read_u32(8));
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER,
                                            &ND_OBIS_DEMAND_ACTIVE_IMPORT, 5,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(4, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER,
                                            &ND_OBIS_DEMAND_ACTIVE_IMPORT, 9,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x12, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(15, buffer[2]);
}

static void test_demand_reset_and_next_period(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER,
                                               &ND_OBIS_DEMAND_ACTIVE_IMPORT, 1,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(0, objects.demand[0].current.current_value);
    TEST_ASSERT_EQUAL_UINT32(0, objects.demand[0].history.last_value);
    objects.demand[0].current.current_value = 7;
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER,
                                               &ND_OBIS_DEMAND_ACTIVE_IMPORT, 2,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(7, objects.demand[0].history.last_value);
    TEST_ASSERT_EQUAL_UINT32(7, objects.max_demand[0].value);
}


static void test_capture_updates_active_rate(void)
{
    uint8_t buffer[4];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 1);
    nd_axdr_put_u8(&out, '3');
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_REGISTER_ACTIVATION,
                                            &ND_OBIS_TARIFF_ACTIVE_MASK_1, 4,
                                            &in));
    objects.demand[0].current.current_value = 7;
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER,
                                               &ND_OBIS_DEMAND_ACTIVE_IMPORT, 2,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(7, objects.max_demand[0].value); /* total */
    TEST_ASSERT_EQUAL_UINT32(7, objects.max_demand[3].value); /* rate 3 */
    TEST_ASSERT_EQUAL_UINT32(0, objects.max_demand[1].value); /* rate 1 */
}

static void test_capture_updates_reactive_group(void)
{
    objects.demand[2].current = (nd_demand_snapshot_t){
        .current_value = 11,
        .period_seconds = 900,
        .periods = 15,
    };
    objects.demand[2].history = (nd_demand_history_t){
        .last_capture_time = {.year = 2026, .month = 11, .day = 11},
        .period_start = {.year = 2026, .month = 11, .day = 11},
    };
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER,
                                               &ND_OBIS_DEMAND_REACTIVE_IMPORT,
                                               2, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(11, objects.max_demand[10].value); /* group 3 */
    TEST_ASSERT_EQUAL_UINT32(0, objects.max_demand[0].value);
}

static void test_last_average_and_apparent_groups(void)
{
    objects.demand[4].history.last_value = 42; /* apparent import */
    objects.demand[4].history.status = 7;
    const nd_obis_t last = {1, 0, 9, 5, 0, 255};
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &last, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(42, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &last, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(32, buffer[5]); /* VA */

    const nd_obis_t apparent_demand = {1, 0, 9, 4, 0, 255};
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER,
                                            &apparent_demand, 3, NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(42, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);

    uint8_t zero[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, zero, sizeof(zero));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_EXTENDED_REGISTER, &last,
                                               1, &in, NULL));
    TEST_ASSERT_EQUAL_UINT32(0, objects.demand[4].history.last_value);
}

static int fake_power(void *ctx, nd_measure_quantity_t quantity,
                      uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    (void)phase;
    *out = (nd_measurement_value_t){
        .value = quantity == ND_MEASURE_TOTAL_ACTIVE_POWER ? 6000 : 4000,
        .scaler = 0,
        .unit = ND_MEASUREMENT_UNIT_WATT,
    };
    return 0;
}

static void test_average_total_and_net_power(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_power};
    objects.reg.measurement = &measurement;
    const nd_obis_t total = {1, 0, 15, 24, 0, 255};
    const nd_obis_t net = {1, 0, 16, 24, 0, 255};
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER, &total, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(6000, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER, &net, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(4000, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER, &total, 4,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(27, buffer[5]); /* W */

    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER, &total, 2,
                                               &in, NULL));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER, &total, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(6000, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER, &total, 1,
                                               &in, NULL));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DEMAND_REGISTER, &total, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(0, ((uint32_t)buffer[1] << 24)
                                    | ((uint32_t)buffer[2] << 16)
                                    | ((uint32_t)buffer[3] << 8) | buffer[4]);
}

static void test_demand_state_persists(void)
{
    static uint8_t memory[512];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(0, nd_ic_demand_save(objects.demand,
                                                nd_byte_store_port(&store), 0));
    nd_idis_objects_t restored = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_ic_demand_load(restored.demand,
                                                nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT32(100, restored.demand[0].current.current_value);
    TEST_ASSERT_EQUAL_UINT32(50, restored.demand[0].history.last_value);
    TEST_ASSERT_EQUAL_UINT8(4, restored.demand[0].history.status);
}

static void test_accumulated_power_feeds_capture(void)
{
    nd_idis_demand_accumulate(&objects.demand_acc[0], 1000);
    nd_idis_demand_accumulate(&objects.demand_acc[0], 2000);
    nd_idis_demand_accumulate(&objects.demand_acc[0], 3000);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DEMAND_REGISTER,
                                               &ND_OBIS_DEMAND_ACTIVE_IMPORT, 2,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(2000, objects.demand[0].history.last_value);
    TEST_ASSERT_EQUAL_UINT32(2000, read_u32(3));
    TEST_ASSERT_EQUAL_UINT32(0, objects.demand_acc[0].samples);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_demand_attributes);
    RUN_TEST(test_demand_reset_and_next_period);
    RUN_TEST(test_capture_updates_active_rate);
    RUN_TEST(test_capture_updates_reactive_group);
    RUN_TEST(test_last_average_and_apparent_groups);
    RUN_TEST(test_average_total_and_net_power);
    RUN_TEST(test_demand_state_persists);
    RUN_TEST(test_accumulated_power_feeds_capture);
    return UNITY_END();
}
