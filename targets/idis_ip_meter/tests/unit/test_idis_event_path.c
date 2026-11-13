#include "unity.h"

#include <string.h>
#include "infra/memory/byte_store.h"

#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event_map.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

void setUp(void) {}
void tearDown(void) {}

static int fake_clock_read(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){
        .year = 2026,
        .month = 11,
        .day = 11,
        .hour = 12,
        .minute = 0,
        .second = 0,
    };
    return 0;
}

static void test_event_log_uses_profile_store(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));

    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    const nd_capture_object_t captures[] = {
        {ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0},
        {ND_IC_DATA, ND_OBIS_EVENT_OBJECT_STANDARD, 2, 0},
    };
    nd_idis_objects_t objects = {0};
    objects.clock.port = &clock_port;
    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);
    nd_idis_profile_config_t config = {
        .ln = ND_OBIS_STANDARD_EVENT_LOG,
        .store = nd_byte_store_port(&store),
        .base_offset = 0,
        .capacity = 4,
        .slot_size = 128,
        .clock = &clock_port,
        .capture_count = 2,
        .capture_period = 0,
    };
    config.captures[0] = captures[0];
    config.captures[1] = captures[1];
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, &config, 1));
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects.profiles, &port);
    nd_idis_event_bind_log(
        &objects.event,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_STANDARD_EVENT_LOG),
        &port, &clock_port, 16);
    nd_idis_event_bind_profiles(&objects.event, &objects.profiles);

    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 1));
    TEST_ASSERT_TRUE(nd_profile_status_has(objects.profiles.items[0].status,
                                           ND_PROFILE_STATUS_PDN));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 68));
    TEST_ASSERT_EQUAL_UINT16(2, nd_event_log_count(&objects.event.log));
    TEST_ASSERT_EQUAL_UINT16(
        2, nd_profile_store_entries(
               &objects.profiles.items[0].store));
}

static void test_event_raise_updates_object_alarm_and_counter(void)
{
    nd_idis_objects_t objects = {0};
    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);

    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 7));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 100));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 1));
    TEST_ASSERT_TRUE(nd_profile_status_has(objects.event.profile_status,
                                           ND_PROFILE_STATUS_PDN));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 68));
    TEST_ASSERT_EQUAL_UINT16(4, nd_event_log_count(&objects.event.log));

    uint8_t buffer[16] = {0};
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_EVENT_OBJECT_STANDARD, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(68, buffer[1]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_ALARM_REGISTER_1, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32((1u << 1) | (1u << 16),
                             ((uint32_t)buffer[1] << 24)
                                 | ((uint32_t)buffer[2] << 16)
                                 | ((uint32_t)buffer[3] << 8) | buffer[4]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_EVENT_COUNTER_1, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(4, ((uint32_t)buffer[1] << 24)
                                    | ((uint32_t)buffer[2] << 16)
                                    | ((uint32_t)buffer[3] << 8) | buffer[4]);
}


static void test_event_state_persists_log_and_status(void)
{
    static uint8_t memory[1024];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_idis_objects_t objects = {0};
    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise(&objects.event, 7));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_save(&objects.event,
                                                nd_byte_store_port(&store), 0));

    nd_idis_objects_t restored = {0};
    nd_idis_event_init(&restored.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_load(&restored.event,
                                                nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(7, restored.event.latest_event_code);
    TEST_ASSERT_EQUAL_UINT16(2, nd_event_log_count(&restored.event.log));
    TEST_ASSERT_EQUAL_UINT32(2, restored.event.counters[0]);
    TEST_ASSERT_TRUE(nd_profile_status_has(restored.event.profile_status,
                                           ND_PROFILE_STATUS_PDN));
}


static void test_event_raise_ex_keeps_business_fields(void)
{
    nd_idis_objects_t objects = {0};
    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_event_raise_ex(&objects.event, 1, 2, 3,
                                                    120, -45));
    const nd_event_record_t *record = nd_event_log_latest(&objects.event.log);
    TEST_ASSERT_NOT_NULL(record);
    TEST_ASSERT_EQUAL_UINT8(2, record->source);
    TEST_ASSERT_EQUAL_UINT8(3, record->severity);
    TEST_ASSERT_EQUAL_UINT32(120, record->duration_seconds);
    TEST_ASSERT_EQUAL_INT32(-45, record->value);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_POWER_OFF, record->kind);
    TEST_ASSERT_EQUAL_UINT16(1, objects.event.log_codes[0]);
}

static void test_event_data_objects(void)
{
    static nd_idis_objects_t objects;
    objects = (nd_idis_objects_t){0};
    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);
    objects.event.counters[1] = 5;
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
    };

    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_EVENT_COUNTERS[1], 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(5, ((uint32_t)buffer[1] << 24)
                                    | ((uint32_t)buffer[2] << 16)
                                    | ((uint32_t)buffer[3] << 8) | buffer[4]);

    uint8_t set_buffer[8];
    nd_axdr_writer_t set;
    nd_axdr_writer_init(&set, set_buffer, sizeof(set_buffer));
    nd_axdr_put_tag(&set, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&set, 0x12345678u);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, set_buffer, set.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_ALARM_FILTER_1, 2, &in));
    TEST_ASSERT_EQUAL_UINT32(0x12345678u, objects.event.alarm.filters[0]);

    nd_axdr_writer_init(&set, set_buffer, sizeof(set_buffer));
    nd_axdr_put_tag(&set, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&set, 0xA5A5A5A5u);
    nd_axdr_reader_init(&in, set_buffer, set.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_ALARM_DESCRIPTOR_2, 2,
                                            &in));
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5u, objects.event.descriptors[1]);

    nd_axdr_writer_init(&set, set_buffer, sizeof(set_buffer));
    nd_axdr_put_tag(&set, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&set, 0x00000042u);
    nd_axdr_reader_init(&in, set_buffer, set.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_ERROR_REGISTER, 2, &in));
    TEST_ASSERT_EQUAL_UINT32(0x42u, objects.event.error_register);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_ALARM_REGISTER_2, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);

    objects.ldn_len = 16;
    for (uint8_t index = 0; index < 16; index++) {
        objects.ldn[index] = (uint8_t)('A' + index);
    }
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_LDN, 2, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(16, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8('A', buffer[2]);

    objects.device_id_len[0] = 8;
    memcpy(objects.device_id[0], "DEV00001", 8);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_DEVICE_IDS[0], 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(8, buffer[1]);
    TEST_ASSERT_EQUAL_MEMORY("DEV00001", buffer + 2, 8);

    uint8_t text[] = {ND_AXDR_OCTET_STRING, 5, 'H', 'E', 'L', 'L', 'O'};
    nd_axdr_reader_init(&in, text, sizeof(text));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_CONSUMER_TEXT, 2, &in));
    TEST_ASSERT_EQUAL_UINT8(5, objects.consumer_text_len);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_CONSUMER_TEXT, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(5, buffer[1]);
    TEST_ASSERT_EQUAL_MEMORY("HELLO", buffer + 2, 5);

    uint8_t mbus_id[] = {ND_AXDR_OCTET_STRING, 4, 'M', 'B', '1', '0'};
    nd_axdr_reader_init(&in, mbus_id, sizeof(mbus_id));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_MBUS_DEVICE_IDS[0][0], 2,
                                            &in));
    TEST_ASSERT_EQUAL_UINT8(4, objects.mbus.device_id_len[0][0]);
    TEST_ASSERT_EQUAL_MEMORY("MB10", objects.mbus.device_id[0][0], 4);
}


static void test_communication_events_26_to_29(void)
{
    nd_idis_objects_t objects = {0};
    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);

    nd_idis_event_communication(&objects.event, ND_IDIS_COMMUNICATION_REMOTE,
                                1);
    TEST_ASSERT_EQUAL_UINT8(26, objects.event.latest_event_code);
    TEST_ASSERT_EQUAL_UINT8(
        26, nd_idis_event_latest_code(&objects.event,
                                      ND_IDIS_EVENT_LOG_COMMUNICATION));
    nd_idis_event_communication(&objects.event, ND_IDIS_COMMUNICATION_REMOTE,
                                0);
    nd_idis_event_communication(&objects.event, ND_IDIS_COMMUNICATION_LOCAL,
                                1);
    nd_idis_event_communication(&objects.event, ND_IDIS_COMMUNICATION_LOCAL,
                                0);
    TEST_ASSERT_EQUAL_UINT8(29, objects.event.latest_event_code);
    TEST_ASSERT_EQUAL_UINT16(4, nd_event_log_count(&objects.event.log));
    TEST_ASSERT_EQUAL_UINT32(4, objects.event.counters[0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_event_log_uses_profile_store);
    RUN_TEST(test_event_raise_updates_object_alarm_and_counter);
    RUN_TEST(test_event_state_persists_log_and_status);
    RUN_TEST(test_event_raise_ex_keeps_business_fields);
    RUN_TEST(test_event_data_objects);
    RUN_TEST(test_communication_events_26_to_29);
    return UNITY_END();
}
