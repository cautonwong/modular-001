#include "unity.h"

#include "targets/idis_ip_meter/cosem/classes/nd_ic_limiter.h"
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
    nd_idis_limiter_init(&objects.limiter);
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

static void set_unsigned(uint8_t attribute, uint32_t value)
{
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&out, value);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_LIMITER,
                                            &objects.limiter.ln, attribute,
                                            &in));
}

static void configure_limiter(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_reader_t in;
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
                          nd_cosem_port_set(&objects, 16, ND_IC_LIMITER,
                                            &objects.limiter.ln, 2, &in));

    set_unsigned(4, 2350);
    set_unsigned(6, 1);
    set_unsigned(7, 1);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 2);
    for (uint8_t direction = 0; direction < 2; direction++) {
        nd_axdr_put_structure_header(&out, 2);
        nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
        nd_axdr_put_length(&out, 6);
        nd_axdr_put_octets(&out,
                           (const uint8_t *)&ND_OBIS_SCRIPT_DISCONNECT, 6);
        nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
        nd_axdr_put_u16(&out, direction == 0 ? 1 : 2);
    }
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_LIMITER,
                                            &objects.limiter.ln, 11, &in));
}

static void test_over_and_under_actions(void)
{
    configure_limiter();
    TEST_ASSERT_EQUAL_INT(0, nd_idis_limiter_on_second(
                                 &objects.limiter, &port, &objects.scripts,
                                 16));
    voltage = 2400;
    TEST_ASSERT_EQUAL_INT(1, nd_idis_limiter_on_second(
                                 &objects.limiter, &port, &objects.scripts,
                                 16));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_limiter_on_second(
                                 &objects.limiter, &port, &objects.scripts,
                                 16));
    voltage = 2300;
    TEST_ASSERT_EQUAL_INT(1, nd_idis_limiter_on_second(
                                 &objects.limiter, &port, &objects.scripts,
                                 16));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);
}

static void test_limiter_attributes(void)
{
    configure_limiter();
    TEST_ASSERT_EQUAL_UINT32(2350, objects.limiter.threshold_normal);
    TEST_ASSERT_EQUAL_UINT32(2350, objects.limiter.threshold_active);
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_LIMITER,
                                            &objects.limiter.ln, 8, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_over_and_under_actions);
    RUN_TEST(test_limiter_attributes);
    return UNITY_END();
}
