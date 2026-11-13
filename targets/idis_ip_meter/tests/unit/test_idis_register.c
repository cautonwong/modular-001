#include "unity.h"

#include "components/protocol/dlms/include/nd_dlms_server.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_register.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "domain/metering/energy_set.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

void setUp(void) {}
void tearDown(void) {}

static int fake_measurement(void *ctx, nd_measure_quantity_t quantity,
                            uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    int64_t value = 0;
    int8_t scaler = -1;
    nd_measurement_unit_t unit = ND_MEASUREMENT_UNIT_VOLT;
    if (quantity == ND_MEASURE_VOLTAGE) {
        value = 2300 + phase * 10;
    } else if (quantity == ND_MEASURE_CURRENT) {
        value = 100 * phase;
        unit = ND_MEASUREMENT_UNIT_AMPERE;
    } else if (quantity == ND_MEASURE_ACTIVE_POWER) {
        value = 5000;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_POWER_FACTOR) {
        value = 950;
        scaler = -3;
    } else if (quantity == ND_MEASURE_FREQUENCY) {
        value = 5001;
        scaler = -2;
        unit = ND_MEASUREMENT_UNIT_HERTZ;
    } else if (quantity == ND_MEASURE_ACTIVE_POWER_EXPORT) {
        value = 1200;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_REACTIVE_POWER_IMPORT
               || quantity == ND_MEASURE_REACTIVE_POWER_EXPORT) {
        value = 2200;
        unit = ND_MEASUREMENT_UNIT_VAR;
    } else if (quantity == ND_MEASURE_APPARENT_POWER_IMPORT
               || quantity == ND_MEASURE_APPARENT_POWER_EXPORT) {
        value = 3200;
        unit = ND_MEASUREMENT_UNIT_VA;
    } else if (quantity == ND_MEASURE_TOTAL_ACTIVE_POWER) {
        value = 6200;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_ACTIVE_POWER_NET) {
        value = 4200;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_CURRENT_SUM) {
        value = 600;
        scaler = -2;
        unit = ND_MEASUREMENT_UNIT_AMPERE;
    } else if (quantity == ND_MEASURE_POWER_FACTOR_MIN
               || quantity == ND_MEASURE_POWER_FACTOR_AVG) {
        value = 900;
        scaler = -3;
    }
    *out = (nd_measurement_value_t){.value = value, .scaler = scaler,
                                    .unit = unit, .phase = phase,
                                    .status = 0};
    return 0;
}

static void test_voltage_l1_value_and_scaler(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    nd_idis_objects_t objects = {.reg = {.measurement = &measurement}};
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_dlms_server_t server = {.port = &port};
    uint8_t buffer[8] = {0};
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_dlms_server_get(&server, 16, ND_IC_REGISTER,
                                             &ND_OBIS_VOLTAGE_L1, 2, NULL,
                                             &out));
    TEST_ASSERT_EQUAL_size_t(5, out.len);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[4]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_dlms_server_get(&server, 16, ND_IC_REGISTER,
                                             &ND_OBIS_VOLTAGE_L1, 3, NULL,
                                             &out));
    TEST_ASSERT_EQUAL_size_t(6, out.len);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x0F, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x16, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(35, buffer[5]);
}

static void test_phase_and_quantity_mapping(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects = {.reg = {.measurement = &measurement}};
    uint8_t buffer[8];
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_VOLTAGE_L2, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x10, buffer[4]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_CURRENT_L3, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x2C, buffer[4]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_POWER_IMPORT, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x13, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x88, buffer[4]);
}

static void test_unknown_register_is_rejected(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    nd_idis_objects_t objects = {.reg = {.measurement = &measurement}};
    nd_obis_t unknown = {1, 0, 99, 7, 0, 255};
    uint8_t buffer[4];
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &unknown, 2, NULL, &out));
}


static void test_energy_register_mapping(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    nd_energy_set_t energy;
    nd_energy_set_init(&energy, 2000);
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &energy, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1, 4000));
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &energy, ND_ENERGY_CHANNEL_ACTIVE_EXPORT, 1, 2000));
    nd_idis_objects_t objects = {
        .reg = {.measurement = &measurement, .energy = &energy},
        .energy = energy,
    };
    objects.reg.energy = &objects.energy;
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_ENERGY_IMPORT, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(2, ((uint32_t)buffer[1] << 24)
                                  | ((uint32_t)buffer[2] << 16)
                                  | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_ENERGY_IMPORT, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x0F, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x16, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(30, buffer[5]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_ENERGY_COMBINED, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(3, ((uint32_t)buffer[1] << 24)
                                  | ((uint32_t)buffer[2] << 16)
                                  | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_ENERGY_NET, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(1, ((uint32_t)buffer[1] << 24)
                                  | ((uint32_t)buffer[2] << 16)
                                  | ((uint32_t)buffer[3] << 8) | buffer[4]);
}


static void test_average_voltage_register(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    nd_idis_objects_t objects = {0};
    objects.reg.measurement = &measurement;
    objects.reg.average = &objects.average;
    nd_idis_average_init(&objects.average, &measurement);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_average_on_second(&objects.average));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_average_on_second(&objects.average));
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_AVERAGE_VOLTAGE_L1, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(2310, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_AVERAGE_VOLTAGE_L1, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(35, buffer[5]);
}

static void test_directional_power_and_sum_registers(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    nd_idis_objects_t objects = {.reg = {.measurement = &measurement}};
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    const nd_obis_t *lns[] = {
        &ND_OBIS_ACTIVE_POWER_EXPORT,     &ND_OBIS_REACTIVE_POWER_IMPORT,
        &ND_OBIS_APPARENT_POWER_EXPORT,   &ND_OBIS_ACTIVE_POWER_COMBINED,
        &ND_OBIS_ACTIVE_POWER_NET,        &ND_OBIS_CURRENT_SUM,
        &ND_OBIS_POWER_FACTOR_MIN,        &ND_OBIS_POWER_FACTOR_AVG,
    };
    const uint32_t expected[] = {1200, 2200, 3200, 6200, 4200, 600, 900, 900};
    for (uint8_t index = 0; index < 8; index++) {
        nd_axdr_writer_init(&out, buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                                lns[index], 2, NULL, &out));
        TEST_ASSERT_EQUAL_UINT8(ND_AXDR_DOUBLE_LONG_UNSIGNED, buffer[0]);
        TEST_ASSERT_EQUAL_UINT32(expected[index],
                                 ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    }
}

static void test_measurement_period_register(void)
{
    nd_idis_objects_t objects = {0};
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_MEASUREMENT_PERIOD_3, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG_UNSIGNED, buffer[0]);
    TEST_ASSERT_EQUAL_UINT16(0, (uint16_t)((buffer[1] << 8) | buffer[2]));

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&out, 300);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_MEASUREMENT_PERIOD_3, 2,
                                            &in));
    TEST_ASSERT_EQUAL_UINT32(300, objects.reg.measurement_period_3);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_REGISTER,
                                               &ND_OBIS_MEASUREMENT_PERIOD_3,
                                               1, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(0, objects.reg.measurement_period_3);
}

static void test_register_logical_name_and_reset(void)
{
    nd_energy_set_t energy;
    nd_energy_set_init(&energy, 2000);
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &energy, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1,
                                 4000));
    nd_idis_objects_t objects = {.energy = energy};
    objects.reg.energy = &objects.energy;
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_ENERGY_IMPORT, 1,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x08, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[7]);
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_REGISTER,
                                               &ND_OBIS_ACTIVE_ENERGY_IMPORT, 1,
                                               &in, NULL));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_ACTIVE_ENERGY_IMPORT, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(0, ((uint32_t)buffer[1] << 24)
                                  | ((uint32_t)buffer[2] << 16)
                                  | ((uint32_t)buffer[3] << 8) | buffer[4]);
}

static void test_active_mask_selects_tariff(void)
{
    nd_register_activation_t activation;
    nd_ic_register_activation_init(&activation);
    TEST_ASSERT_EQUAL_UINT8(1, nd_ic_register_activation_tariff(&activation));
    activation.energy.length = 1;
    activation.energy.mask[0] = '3';
    TEST_ASSERT_EQUAL_UINT8(3, nd_ic_register_activation_tariff(&activation));
}

static void test_parameter_reset_sets_zero(void)
{
    nd_idis_objects_t objects = {0};
    objects.reg.pq = &objects.pq;
    objects.pq.config.voltage_sag = 207;
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_REGISTER,
                                               &ND_OBIS_PQ_SAG_THRESHOLD, 1,
                                               &in, NULL));
    TEST_ASSERT_EQUAL_INT32(0, objects.pq.config.voltage_sag);
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_PQ_SAG_THRESHOLD, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT16(0, ((uint16_t)buffer[1] << 8) | buffer[2]);
}


static void test_single_phase_rejects_l2_and_l3(void)
{
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    nd_idis_objects_t objects = {
        .reg = {.measurement = &measurement, .phases = 1},
    };
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_VOLTAGE_L1, 1, NULL,
                                            &out));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_VOLTAGE_L2, 1, NULL,
                                            &out));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_VOLTAGE_L3, 1, NULL,
                                            &out));

    nd_idis_average_init(&objects.average, &measurement);
    objects.reg.average = &objects.average;
    (void)nd_idis_average_on_second(&objects.average);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_AVERAGE_VOLTAGE_L2, 2,
                                            NULL, &out));

    nd_pq_config_t pq_config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 50,
        .debounce_seconds = 1,
        .phase_count = 1,
    };
    nd_idis_pq_init(&objects.pq, &pq_config);
    objects.reg.pq = &objects.pq;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, ND_IC_REGISTER,
                                            &ND_OBIS_PQ_SAG_COUNT[1], 2, NULL,
                                            &out));
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, 0);
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_action(&objects, 16, ND_IC_REGISTER,
                                               &ND_OBIS_VOLTAGE_L2, 1, &in,
                                               NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_voltage_l1_value_and_scaler);
    RUN_TEST(test_phase_and_quantity_mapping);
    RUN_TEST(test_unknown_register_is_rejected);
    RUN_TEST(test_energy_register_mapping);
    RUN_TEST(test_average_voltage_register);
    RUN_TEST(test_directional_power_and_sum_registers);
    RUN_TEST(test_measurement_period_register);
    RUN_TEST(test_register_logical_name_and_reset);
    RUN_TEST(test_parameter_reset_sets_zero);
    RUN_TEST(test_active_mask_selects_tariff);
    RUN_TEST(test_single_phase_rejects_l2_and_l3);
    return UNITY_END();
}
