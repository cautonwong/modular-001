#include "unity.h"

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_clock_port_t clock_port;
static nd_cosem_port_t port;

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 11, .day = 11};
    return 0;
}

static int fake_push_send(void *ctx, uint8_t setup_index,
                          const nd_push_setup_t *setup)
{
    (void)ctx;
    (void)setup_index;
    (void)setup;
    return 0;
}

static const nd_push_port_t fake_push_port = {.ctx = NULL,
                                              .send = fake_push_send};

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    clock_port = (nd_clock_port_t){.ctx = NULL, .read = fake_clock};
    objects.clock.port = &clock_port;
    nd_disconnect_init(&objects.disconnect, 2, NULL);
    nd_disconnect_init(&objects.load_control_1, 2, NULL);
    nd_disconnect_init(&objects.load_control_2, 2, NULL);
    for (uint8_t relay = 0; relay < 4; relay++) {
        nd_disconnect_init(&objects.mbus_disconnect[relay], 2, NULL);
    }
    nd_ic_register_activation_init(&objects.activation);
    nd_ic_function_control_init(&objects.functions);
    nd_idis_push_init(&objects.push);
    nd_push_bind_port(&objects.push.state, &fake_push_port);
    nd_idis_script_init(&objects.scripts);
    port = (nd_cosem_port_t){
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_script_bind_port(&objects.scripts, &port);
}

void tearDown(void) {}

static int execute_script(const nd_obis_t *table, uint16_t script_id)
{
    uint8_t buffer[4];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, script_id);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    return nd_cosem_port_action(&objects, 16, ND_IC_SCRIPT_TABLE, table, 1,
                                &in, NULL);
}

static void test_get_logical_name(void)
{
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 1,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(6, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)&ND_OBIS_SCRIPT_DISCONNECT,
                                  buffer + 2, 6);
}

static void test_get_scripts_structure(void)
{
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 2,
                                            NULL, &out));
    size_t index = 0;
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(4, buffer[index++]); /* four scripts */
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG_UNSIGNED, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[index++]); /* script id 1 */
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[index++]); /* one action */
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(5, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[index++]); /* SET */
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG_UNSIGNED, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[index++]);
    TEST_ASSERT_EQUAL_UINT8(70, buffer[index++]); /* disconnect control */
}

static void test_disconnect_scripts_change_state(void)
{
    /* Script 1 sets control_state 2 = local disconnect. */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 1));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.disconnect.control_state);
    /* Script 2 sets control_state 1 = local reconnect. */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 2));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);
    /* Script 3 is the remote_disconnect method. */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 3));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_DISCONNECTED,
                            objects.disconnect.control_state);
    /* Script 4 is the remote_connect method. */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 4));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);
}

static void test_unknown_script_and_empty_table(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 99));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE,
                          execute_script(&ND_OBIS_SCRIPT_MBUS_PROFILE_CONTROL,
                                         1));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          execute_script(&(nd_obis_t){9, 9, 9, 9, 9, 9}, 1));
}

static void test_multiple_actions_per_script(void)
{
    /* Tariffication script 2 sets mask "2" on both register activation
     * objects. */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_TARIFF, 2));
    TEST_ASSERT_EQUAL_UINT8(1, objects.activation.energy.length);
    TEST_ASSERT_EQUAL_MEMORY("2", objects.activation.energy.mask, 1);
    TEST_ASSERT_EQUAL_MEMORY("2", objects.activation.demand.mask, 1);
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);

    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_ACTIVE_TARIFF, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8('2', buffer[2]);
}

static void test_load_control_and_mbus_targets(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK, execute_script(&ND_OBIS_SCRIPT_LOAD_MGMT, 1));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.load_control_1.control_state);
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_INT(
        ND_OK, execute_script(&ND_OBIS_SCRIPT_MBUS_DISCONNECT, 1));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.mbus_disconnect[0].control_state);
    TEST_ASSERT_EQUAL_INT(
        ND_OK, execute_script(&ND_OBIS_SCRIPT_MBUS_DISCONNECT, 2));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.mbus_disconnect[0].control_state);
}

static void test_function_control_and_push_targets(void)
{
    TEST_ASSERT_EQUAL_INT(
        ND_OK, execute_script(&ND_OBIS_SCRIPT_FUNCTION_CONTROL, 2));
    TEST_ASSERT_EQUAL_UINT8(0,
                            objects.functions.local.functions[0].enabled);
    TEST_ASSERT_EQUAL_UINT8(0,
                            objects.functions.local.functions[1].enabled);
    TEST_ASSERT_EQUAL_UINT8(1,
                            objects.functions.local.functions[2].enabled);
    objects.push.state.setups[1].enabled = 1;
    TEST_ASSERT_EQUAL_INT(ND_OK, execute_script(&ND_OBIS_SCRIPT_PUSH, 1));
    TEST_ASSERT_EQUAL_UINT32(1, objects.push.state.setups[1].pushes);
    TEST_ASSERT_EQUAL_UINT8(1, objects.push.state.setups[1].has_confirmation);
}

static int build_script_set(uint8_t *buffer, size_t capacity)
{
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, capacity);
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 5); /* new script id */
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 5);
    nd_axdr_put_tag(&out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&out, 2); /* ACTION */
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, ND_IC_DISCONNECT_CONTROL);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_DISCONNECT_CONTROL, 6);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 1); /* remote_disconnect */
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 0);
    return nd_axdr_writer_ok(&out) ? (int)out.len : -1;
}

static void test_script_table_set(void)
{
    uint8_t payload[64];
    int length = build_script_set(payload, sizeof(payload));
    TEST_ASSERT_TRUE(length > 0);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, payload, (size_t)length);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 2,
                                            &in));

    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]); /* one script after SET */

    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 5));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_DISCONNECTED,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 1));
}

static void test_script_table_set_rejects_bad_payload(void)
{
    static const uint8_t broken[] = {ND_AXDR_ARRAY, 1, ND_AXDR_STRUCTURE, 2,
                                     ND_AXDR_LONG_UNSIGNED, 0, 9};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, broken, sizeof(broken));
    TEST_ASSERT_EQUAL_INT(ND_ERR_TYPE_MISMATCH,
                          nd_cosem_port_set(&objects, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 2,
                                            &in));
    /* The predefined table is still in use. */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          execute_script(&ND_OBIS_SCRIPT_DISCONNECT, 1));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.disconnect.control_state);
}

static void test_script_table_set_persists(void)
{
    static uint8_t memory[8192];
    static nd_idis_objects_t restored;
    static nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_idis_script_bind_store(&objects.scripts, nd_byte_store_port(&store), 0);

    uint8_t payload[64];
    int length = build_script_set(payload, sizeof(payload));
    TEST_ASSERT_TRUE(length > 0);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, payload, (size_t)length);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 2,
                                            &in));

    restored = (nd_idis_objects_t){0};
    nd_disconnect_init(&restored.disconnect, 2, NULL);
    nd_idis_script_init(&restored.scripts);
    nd_cosem_port_t restored_port = {
        .ctx = &restored,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_script_bind_port(&restored.scripts, &restored_port);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_script_load(
                                 &restored.scripts, nd_byte_store_port(&store),
                                 0));

    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&restored, 16, ND_IC_SCRIPT_TABLE,
                                            &ND_OBIS_SCRIPT_DISCONNECT, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]); /* one persisted script */

    uint8_t id[3] = {ND_AXDR_LONG_UNSIGNED, 0, 5};
    nd_axdr_reader_init(&in, id, sizeof(id));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&restored, 16,
                                               ND_IC_SCRIPT_TABLE,
                                               &ND_OBIS_SCRIPT_DISCONNECT, 1,
                                               &in, NULL));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_DISCONNECTED,
                            restored.disconnect.control_state);
}

static void test_script_table_load_rejects_corrupt_record(void)
{
    static uint8_t memory[8192];
    static nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_script_save(
                                 &objects.scripts, nd_byte_store_port(&store),
                                 0));
    memory[0] ^= 0xFF; /* corrupt the magic */
    nd_script_state_t loaded;
    nd_idis_script_init(&loaded);
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_script_load(
                                  &loaded, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(0, loaded.ram_valid[2]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_get_logical_name);
    RUN_TEST(test_get_scripts_structure);
    RUN_TEST(test_disconnect_scripts_change_state);
    RUN_TEST(test_unknown_script_and_empty_table);
    RUN_TEST(test_multiple_actions_per_script);
    RUN_TEST(test_load_control_and_mbus_targets);
    RUN_TEST(test_function_control_and_push_targets);
    RUN_TEST(test_script_table_set);
    RUN_TEST(test_script_table_set_rejects_bad_payload);
    RUN_TEST(test_script_table_set_persists);
    RUN_TEST(test_script_table_load_rejects_corrupt_record);
    return UNITY_END();
}
