#include "unity.h"

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_register_monitor.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_cosem_port_t port;
static int64_t voltage;

static int fake_measurement(void *ctx, nd_measure_quantity_t quantity,
                            uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    (void)phase;
    *out = (nd_measurement_value_t){
        .value = quantity == ND_MEASURE_VOLTAGE ? voltage : 0,
        .scaler = -1,
        .unit = ND_MEASUREMENT_UNIT_VOLT,
    };
    return 0;
}

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_measurement_port_t measurement = {.ctx = NULL, .read = fake_measurement};
    objects.reg.measurement = &measurement;
    nd_disconnect_init(&objects.disconnect, 2, NULL);
    nd_idis_script_init(&objects.scripts);
    nd_idis_monitors_init(&objects.monitors);
    port = (nd_cosem_port_t){
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_script_bind_port(&objects.scripts, &port);
    voltage = 2300;
}

void tearDown(void) {}

static void configure_monitor(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_reader_t in;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 2350);
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_REGISTER_MONITOR,
                                            &objects.monitors.items[0].ln, 2,
                                            &in));

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 3);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, ND_IC_REGISTER);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_VOLTAGE_L1, 6);
    nd_axdr_put_tag(&out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(&out, 2);
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_REGISTER_MONITOR,
                                            &objects.monitors.items[0].ln, 3,
                                            &in));

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_SCRIPT_DISCONNECT, 6);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_SCRIPT_DISCONNECT, 6);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 2);
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_REGISTER_MONITOR,
                                            &objects.monitors.items[0].ln, 4,
                                            &in));
}

static void test_threshold_crossings_run_scripts(void)
{
    configure_monitor();
    TEST_ASSERT_EQUAL_INT(0, nd_idis_monitor_on_second(
                                 &objects.monitors, &port, &objects.scripts,
                                 16));
    voltage = 2400;
    TEST_ASSERT_EQUAL_INT(1, nd_idis_monitor_on_second(
                                 &objects.monitors, &port, &objects.scripts,
                                 16));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.disconnect.control_state);
    voltage = 2300;
    TEST_ASSERT_EQUAL_INT(1, nd_idis_monitor_on_second(
                                 &objects.monitors, &port, &objects.scripts,
                                 16));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_monitor_on_second(
                                 &objects.monitors, &port, &objects.scripts,
                                 16));
}

static void test_monitor_attributes(void)
{
    configure_monitor();
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_REGISTER_MONITOR,
                                            &objects.monitors.items[0].ln, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    TEST_ASSERT_EQUAL_UINT16(2350, (uint16_t)((buffer[3] << 8) | buffer[4]));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_REGISTER_MONITOR,
                                            &objects.monitors.items[0].ln, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT16(ND_IC_REGISTER,
                             (uint16_t)((buffer[3] << 8) | buffer[4]));
}

static void test_monitor_persists(void)
{
    static uint8_t memory[1024];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    configure_monitor();
    TEST_ASSERT_EQUAL_INT(0, nd_idis_monitor_save(&objects.monitors,
                                                  nd_byte_store_port(&store),
                                                  0));
    nd_idis_monitors_t restored;
    nd_idis_monitors_init(&restored);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_monitor_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(1, restored.items[0].threshold_count);
    TEST_ASSERT_EQUAL_UINT16(2350, restored.items[0].thresholds[0]);
    TEST_ASSERT_EQUAL_UINT16(1, restored.items[0].up[0].selector);
    TEST_ASSERT_EQUAL_UINT16(2, restored.items[0].down[0].selector);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_threshold_crossings_run_scripts);
    RUN_TEST(test_monitor_attributes);
    RUN_TEST(test_monitor_persists);
    return UNITY_END();
}
