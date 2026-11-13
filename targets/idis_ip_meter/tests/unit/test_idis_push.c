#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_clock_port_t clock_port;
static int send_result;

static int fake_send(void *ctx, uint8_t setup_index,
                     const nd_push_setup_t *setup)
{
    (void)ctx;
    (void)setup_index;
    (void)setup;
    return send_result;
}

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 11, .day = 11,
                                .hour = 10, .minute = 7, .second = 31};
    return 0;
}

static const nd_push_port_t push_port = {.ctx = NULL, .send = fake_send};

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_idis_push_init(&objects.push);
    nd_push_bind_port(&objects.push.state, &push_port);
    clock_port = (nd_clock_port_t){.ctx = NULL, .read = fake_clock};
    objects.clock.port = &clock_port;
    send_result = 0;
}

void tearDown(void) {}

static void set_object_list(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 5);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, ND_IC_REGISTER);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_ACTIVE_ENERGY_IMPORT, 6);
    nd_axdr_put_tag(&out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 0);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&out, 0);
    nd_axdr_put_tag(&out, ND_AXDR_NULL);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_PUSH_SETUP,
                                            &ND_OBIS_PUSH_1, 2, &in));
}

static void set_destination(void)
{
    uint8_t buffer[48];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 3);
    nd_axdr_put_tag(&out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&out, 0); /* TCP */
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 4);
    nd_axdr_put_octets(&out, (const uint8_t *)"host", 4);
    nd_axdr_put_tag(&out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&out, 4); /* A-XDR encoded APDU */
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_PUSH_SETUP,
                                            &ND_OBIS_PUSH_1, 3, &in));
}

static int trigger_push(void)
{
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    return nd_cosem_port_action(&objects, 16, ND_IC_PUSH_SETUP, &ND_OBIS_PUSH_1,
                                1, &in, NULL);
}

static void test_default_attributes(void)
{
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PUSH_SETUP,
                                            &ND_OBIS_PUSH_0, 2, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PUSH_SETUP,
                                            &ND_OBIS_PUSH_0, 3, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[5]); /* empty destination */
}

static void test_object_list_and_destination_round_trip(void)
{
    set_object_list();
    set_destination();
    TEST_ASSERT_EQUAL_UINT8(1, objects.push.object_count[1]);
    TEST_ASSERT_EQUAL_UINT16(ND_IC_REGISTER,
                             objects.push.objects[1][0].class_id);
    TEST_ASSERT_EQUAL_UINT8(2,
                            objects.push.objects[1][0].attribute_index);
    TEST_ASSERT_EQUAL_UINT8(4,
                            objects.push.state.setups[1].destination.destination_length);
    TEST_ASSERT_EQUAL_MEMORY("host",
                             objects.push.state.setups[1].destination.destination, 4);

    uint8_t buffer[96];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PUSH_SETUP,
                                            &ND_OBIS_PUSH_1, 2, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(5, buffer[3]);
}

static void test_push_disabled_without_objects(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE, trigger_push());
    TEST_ASSERT_EQUAL_UINT32(0, objects.push.state.setups[1].attempts);
}

static void test_push_success(void)
{
    set_object_list();
    set_destination();
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 2); /* number_of_retries */
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_PUSH_SETUP,
                                            &ND_OBIS_PUSH_1, 6, &in));

    TEST_ASSERT_EQUAL_INT(ND_OK, trigger_push());
    TEST_ASSERT_EQUAL_UINT32(1, objects.push.state.setups[1].attempts);
    TEST_ASSERT_EQUAL_UINT32(1, objects.push.state.setups[1].pushes);
    TEST_ASSERT_EQUAL_UINT8(0, objects.push.state.setups[1].pending);
    TEST_ASSERT_EQUAL_UINT16(2026,
                             objects.push.state.setups[1].last_confirmation.year);
}

static void test_push_retries_then_gives_up(void)
{
    set_object_list();
    send_result = -1;
    objects.push.state.setups[1].number_of_retries = 2;
    objects.push.state.setups[1].repetition_delay = 1;

    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE, trigger_push());
    TEST_ASSERT_EQUAL_UINT32(1, objects.push.state.setups[1].attempts);
    TEST_ASSERT_EQUAL_UINT8(1, objects.push.state.setups[1].pending);

    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(1, nd_push_on_second(&objects.push.state, &now));
    TEST_ASSERT_EQUAL_UINT32(2, objects.push.state.setups[1].attempts);
    TEST_ASSERT_EQUAL_INT(1, nd_push_on_second(&objects.push.state, &now));
    TEST_ASSERT_EQUAL_UINT32(3, objects.push.state.setups[1].attempts);
    TEST_ASSERT_EQUAL_UINT8(0, objects.push.state.setups[1].pending);
    TEST_ASSERT_EQUAL_INT(0, nd_push_on_second(&objects.push.state, &now));
    TEST_ASSERT_EQUAL_UINT32(3, objects.push.state.setups[1].failures);
}

static void test_push_reset_clears_counters(void)
{
    set_object_list();
    set_destination();
    TEST_ASSERT_EQUAL_INT(ND_OK, trigger_push());
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_PUSH_SETUP,
                                               &ND_OBIS_PUSH_1, 2, &in,
                                               NULL));
    TEST_ASSERT_EQUAL_UINT32(0, objects.push.state.setups[1].pushes);
    TEST_ASSERT_EQUAL_UINT32(0, objects.push.state.setups[1].attempts);
}

static void test_push_persists(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    set_object_list();
    set_destination();
    TEST_ASSERT_EQUAL_INT(0, nd_idis_push_save(&objects.push,
                                               nd_byte_store_port(&store), 0));
    nd_idis_objects_t restored = {0};
    nd_idis_push_init(&restored.push);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_push_load(&restored.push,
                                               nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(1, restored.push.object_count[1]);
    TEST_ASSERT_EQUAL_UINT8(4,
                            restored.push.state.setups[1].destination
                                .destination_length);
    TEST_ASSERT_EQUAL_UINT16(ND_IC_REGISTER,
                             restored.push.objects[1][0].class_id);
}

static void test_push_gateway_and_window_attributes(void)
{
    const nd_obis_t ln = ND_OBIS_PUSH_1;
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_reader_t in;
    uint8_t gateway[] = {ND_AXDR_OCTET_STRING, 3, 'G', 'W', '1'};
    nd_axdr_reader_init(&in, gateway, sizeof(gateway));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_PUSH_SETUP, &ln,
                                            11, &in));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PUSH_SETUP, &ln,
                                            11, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8('G', buffer[2]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 4);
    nd_axdr_put_octets(&out, (const uint8_t *)"0700", 4);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 4);
    nd_axdr_put_octets(&out, (const uint8_t *)"2200", 4);
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_PUSH_SETUP, &ln,
                                            4, &in));
    uint8_t reply[32];
    nd_axdr_writer_init(&out, reply, sizeof(reply));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PUSH_SETUP, &ln,
                                            4, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, reply[0]);
    TEST_ASSERT_EQUAL_UINT8(1, reply[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, reply[2]); /* structure */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_default_attributes);
    RUN_TEST(test_object_list_and_destination_round_trip);
    RUN_TEST(test_push_disabled_without_objects);
    RUN_TEST(test_push_success);
    RUN_TEST(test_push_retries_then_gives_up);
    RUN_TEST(test_push_reset_clears_counters);
    RUN_TEST(test_push_persists);
    RUN_TEST(test_push_gateway_and_window_attributes);
    return UNITY_END();
}
