#include "unity.h"

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_mbus.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static uint32_t fake_identification;
static uint8_t install_commands;

static int fake_read(void *ctx, uint8_t channel, nd_mbus_readout_t *out)
{
    (void)ctx;
    *out = (nd_mbus_readout_t){
        .identity = {
            .identification_number = fake_identification,
            .manufacturer_id = 0x1234,
            .version = 1,
            .device_type = 3,
        },
        .time = {.year = 2026, .month = 11, .day = 11},
        .access_number = (uint8_t)(10 + channel),
        .status = 0x40,
        .alarm = 1,
        .configuration = 0x0005,
        .value_count = 4,
    };
    for (uint8_t index = 0; index < out->value_count; index++) {
        out->values[index] = (nd_mbus_value_t){
            .value = 1000 + channel * 100 + index,
            .scaler = -1,
            .unit = 35,
        };
    }
    return 0;
}

static int fake_write(void *ctx, uint8_t channel, const uint8_t *data,
                      size_t length)
{
    (void)ctx;
    (void)channel;
    (void)data;
    (void)length;
    return 0;
}

static int fake_command(void *ctx, uint8_t channel, nd_mbus_command_t command,
                        uint8_t argument)
{
    (void)ctx;
    (void)channel;
    (void)argument;
    if (command == ND_MBUS_COMMAND_INSTALL) {
        install_commands++;
    }
    return 0;
}

static const nd_mbus_port_t fake_port = {
    .ctx = NULL,
    .read = fake_read,
    .write = fake_write,
    .command = fake_command,
};

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_idis_mbus_init(&objects.mbus);
    nd_mbus_bind_port(&objects.mbus.state, &fake_port);
    fake_identification = 12345678u;
    install_commands = 0;
}

void tearDown(void) {}

static int call_method(const nd_obis_t *ln, uint8_t method_id,
                       const uint8_t *parameter, size_t length)
{
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, length);
    return nd_cosem_port_action(&objects, 16, ND_IC_MBUS_CLIENT, ln, method_id,
                                &in, NULL);
}

static int get_client(const nd_obis_t *ln, uint8_t attribute_id,
                      uint8_t *buffer, size_t size)
{
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, size);
    return nd_cosem_port_get(&objects, 16, ND_IC_MBUS_CLIENT, ln, attribute_id,
                             NULL, &out);
}

static void install_and_capture(const nd_obis_t *ln, uint8_t address,
                                uint8_t channel)
{
    uint8_t parameter[3] = {ND_AXDR_UNSIGNED, address, 0};
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(ln, 1, parameter, 2));
    uint8_t capture[2] = {ND_AXDR_INTEGER, 0};
    TEST_ASSERT_EQUAL_INT(ND_OK, call_method(ln, 3, capture, 2));
    TEST_ASSERT_EQUAL_UINT8(address,
                            objects.mbus.state.clients[channel - 1]
                                .primary_address);
}

static void test_install_and_capture(void)
{
    uint8_t parameter[3] = {ND_AXDR_UNSIGNED, 7, 0};
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(&ND_OBIS_MBUS_CLIENT_2, 1, parameter, 2));
    TEST_ASSERT_EQUAL_UINT8(1, install_commands);
    TEST_ASSERT_EQUAL_UINT8(7,
                            objects.mbus.state.clients[1].primary_address);

    uint8_t buffer[16];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_client(&ND_OBIS_MBUS_CLIENT_2, 5, buffer,
                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_UNSIGNED, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(7, buffer[1]);

    uint8_t capture[2] = {ND_AXDR_INTEGER, 0};
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(&ND_OBIS_MBUS_CLIENT_2, 3, capture, 2));
    TEST_ASSERT_EQUAL_UINT32(12345678u,
                             objects.mbus.state.clients[1]
                                 .identification_number);
    TEST_ASSERT_EQUAL_UINT8(12, objects.mbus.state.clients[1].access_number);
    TEST_ASSERT_EQUAL_UINT8(0x40, objects.mbus.state.clients[1].status);
    TEST_ASSERT_EQUAL_UINT16(5, objects.mbus.state.clients[1].configuration);
    TEST_ASSERT_EQUAL_UINT32(1200, objects.mbus.values[1][0].value);
    TEST_ASSERT_EQUAL_INT8(-1, objects.mbus.values[1][0].scaler);
    TEST_ASSERT_EQUAL_UINT8(35, objects.mbus.values[1][0].unit);
    TEST_ASSERT_EQUAL_UINT16(2026, objects.mbus.values[1][0].capture_time.year);

    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER,
                                            &ND_OBIS_MBUS_VALUE_2_1, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_DOUBLE_LONG_UNSIGNED, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(1200,
                             ((uint32_t)buffer[1] << 24)
                                 | ((uint32_t)buffer[2] << 16)
                                 | ((uint32_t)buffer[3] << 8) | buffer[4]);
}

static void test_capture_rejects_changed_identity(void)
{
    install_and_capture(&ND_OBIS_MBUS_CLIENT_1, 1, 1);
    fake_identification = 87654321u;
    uint8_t capture[2] = {ND_AXDR_INTEGER, 0};
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE,
                          call_method(&ND_OBIS_MBUS_CLIENT_1, 3, capture, 2));
    TEST_ASSERT_EQUAL_UINT32(12345678u,
                             objects.mbus.state.clients[0]
                                 .identification_number);
}

static void test_key_status_machine(void)
{
    install_and_capture(&ND_OBIS_MBUS_CLIENT_1, 1, 1);
    uint8_t set_key[18] = {ND_AXDR_OCTET_STRING, 16};
    for (uint8_t index = 0; index < 16; index++) {
        set_key[2 + index] = index;
    }
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(&ND_OBIS_MBUS_CLIENT_1, 7, set_key, 18));
    TEST_ASSERT_EQUAL_UINT8(1, objects.mbus.state.clients[0].key_status);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(&ND_OBIS_MBUS_CLIENT_1, 8, set_key, 18));
    TEST_ASSERT_EQUAL_UINT8(3, objects.mbus.state.clients[0].key_status);
    uint8_t capture[2] = {ND_AXDR_INTEGER, 0};
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(&ND_OBIS_MBUS_CLIENT_1, 3, capture, 2));
    TEST_ASSERT_EQUAL_UINT8(4, objects.mbus.state.clients[0].key_status);

    uint8_t buffer[8];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_client(&ND_OBIS_MBUS_CLIENT_1, 14, buffer,
                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(4, buffer[1]);

    uint8_t zero[2] = {ND_AXDR_UNSIGNED, 0};
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          call_method(&ND_OBIS_MBUS_CLIENT_1, 2, zero, 2));
    TEST_ASSERT_EQUAL_UINT8(0,
                            objects.mbus.state.clients[0].primary_address);
    TEST_ASSERT_EQUAL_UINT8(0, objects.mbus.state.clients[0].key_status);
}

static void test_value_scaler_unit_set(void)
{
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(&out, (uint8_t)-2);
    nd_axdr_put_tag(&out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&out, 30);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER,
                                            &ND_OBIS_MBUS_VALUE_3_2, 3, &in));
    TEST_ASSERT_EQUAL_INT8(-2, objects.mbus.values[2][1].scaler);
    TEST_ASSERT_EQUAL_UINT8(30, objects.mbus.values[2][1].unit);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER,
                                            &ND_OBIS_MBUS_VALUE_3_2, 3, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFE, buffer[3]); /* -2 */
}

static void test_port_setup_object(void)
{
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_MBUS_PORT_SETUP,
                                            &ND_OBIS_MBUS_PORT_SETUP_1, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]); /* 2400 baud */
}

static void test_mbus_state_persists(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    install_and_capture(&ND_OBIS_MBUS_CLIENT_1, 3, 1);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_mbus_save(&objects.mbus,
                                               nd_byte_store_port(&store), 0));

    nd_idis_objects_t restored = {0};
    nd_idis_mbus_init(&restored.mbus);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_mbus_load(&restored.mbus,
                                               nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(3,
                            restored.mbus.state.clients[0].primary_address);
    TEST_ASSERT_EQUAL_UINT32(12345678u,
                             restored.mbus.state.clients[0]
                                 .identification_number);
    TEST_ASSERT_EQUAL_UINT32(1100, restored.mbus.values[0][0].value);
    TEST_ASSERT_EQUAL_UINT8(3, restored.mbus.port_speed[0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_install_and_capture);
    RUN_TEST(test_capture_rejects_changed_identity);
    RUN_TEST(test_key_status_machine);
    RUN_TEST(test_value_scaler_unit_set);
    RUN_TEST(test_port_setup_object);
    RUN_TEST(test_mbus_state_persists);
    return UNITY_END();
}
