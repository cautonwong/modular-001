#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profile.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_single_action.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_cosem_port_t port;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_disconnect_init(&objects.disconnect, 2, NULL);
    nd_idis_script_init(&objects.scripts);
    nd_idis_single_action_init(&objects.single_action);
    port = (nd_cosem_port_t){
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_script_bind_port(&objects.scripts, &port);
}

void tearDown(void) {}

static void set_executed_script(uint16_t selector)
{
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_SCRIPT_DISCONNECT, 6);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, selector);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_SINGLE_ACTION,
                                            &ND_OBIS_SINGLE_ACTION_BILLING, 2,
                                            &in));
}

static void set_time(uint8_t hour, uint8_t minute, uint8_t second)
{
    uint8_t buffer[24];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 4);
    nd_axdr_put_u8(&out, hour);
    nd_axdr_put_u8(&out, minute);
    nd_axdr_put_u8(&out, second);
    nd_axdr_put_u8(&out, 0);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 5);
    for (uint8_t index = 0; index < 5; index++) {
        nd_axdr_put_u8(&out, 0xFF);
    }
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_SINGLE_ACTION,
                                            &ND_OBIS_SINGLE_ACTION_BILLING, 4,
                                            &in));
}

static void test_defaults_and_set_round_trip(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_SINGLE_ACTION,
                                            &ND_OBIS_SINGLE_ACTION_BILLING, 4,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);

    set_executed_script(1);
    set_time(10, 7, 31);
    const nd_single_action_state_t *state =
        &objects.single_action.items[0];
    TEST_ASSERT_EQUAL_UINT16(1, state->selector);
    TEST_ASSERT_EQUAL_UINT8(1, state->time_count);
    TEST_ASSERT_EQUAL_UINT8(10, state->times[0].time[0]);
    TEST_ASSERT_EQUAL_UINT8(5, state->times[0].date_len);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_SINGLE_ACTION,
                                            &ND_OBIS_SINGLE_ACTION_BILLING, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
}

static void test_tick_executes_script_once(void)
{
    set_executed_script(1);
    set_time(10, 7, 31);
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11,
                         .hour = 10, .minute = 7, .second = 31};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_single_action_on_second(
                                 &objects.single_action, &port,
                                 &objects.scripts, 16, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_single_action_on_second(
                                 &objects.single_action, &port,
                                 &objects.scripts, 16, &now));

    now.second = 30;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_single_action_on_second(
                                 &objects.single_action, &port,
                                 &objects.scripts, 16, &now));
}

static void test_single_action_persists(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    set_executed_script(2);
    set_time(6, 30, 0);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_single_action_save(
                                 &objects.single_action,
                                 nd_byte_store_port(&store), 0));

    nd_single_action_t restored;
    nd_idis_single_action_init(&restored);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_single_action_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT16(2, restored.items[0].selector);
    TEST_ASSERT_EQUAL_UINT8(1, restored.items[0].time_count);
    TEST_ASSERT_EQUAL_UINT8(6, restored.items[0].times[0].time[0]);
    TEST_ASSERT_TRUE(nd_obis_eq(&restored.items[0].ln,
                                &ND_OBIS_SINGLE_ACTION_BILLING));
}


static void test_default_billing_schedule(void)
{
    nd_idis_single_action_apply_defaults(&objects.single_action);
    const nd_single_action_state_t *state = &objects.single_action.items[0];
    TEST_ASSERT_EQUAL_UINT8(1, state->selector);
    TEST_ASSERT_EQUAL_UINT8(1, state->time_count);
    TEST_ASSERT_EQUAL_UINT8(0, state->times[0].time[0]);
    TEST_ASSERT_EQUAL_UINT8(0, state->times[0].time[1]);
    TEST_ASSERT_EQUAL_UINT8(0, state->times[0].time[2]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, state->times[0].date[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, state->times[0].date[1]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, state->times[0].date[2]);
    TEST_ASSERT_EQUAL_UINT8(1, state->times[0].date[3]);
    TEST_ASSERT_EQUAL_UINT8(0, state->script_ln[0]);
    TEST_ASSERT_EQUAL_UINT8(10, state->script_ln[2]);
    TEST_ASSERT_EQUAL_UINT8(1, state->script_ln[4]);

    set_executed_script(2);
    set_time(10, 7, 31);
    nd_idis_single_action_apply_defaults(&objects.single_action);
    TEST_ASSERT_EQUAL_UINT16(2, state->selector);
    TEST_ASSERT_EQUAL_UINT8(10, state->times[0].time[0]);
}


static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 1, .day = 1};
    return 0;
}

static void test_default_billing_schedule_captures(void)
{
    static uint8_t memory[1024];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_clock_port_t clock = {.ctx = NULL, .read = fake_clock};
    objects.clock.port = &clock;
    nd_idis_profile_config_t config = {
        .ln = ND_OBIS_BILLING_PROFILE,
        .store = nd_byte_store_port(&store),
        .base_offset = 0,
        .capacity = 4,
        .slot_size = 128,
        .clock = &clock,
        .capture_count = 1,
        .capture_period = 0,
    };
    config.captures[0] = (nd_capture_object_t){
        ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0};
    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_profiles_init(&objects.profiles, &config, 1));
    nd_idis_profiles_bind_port(&objects.profiles, &port);
    nd_idis_single_action_apply_defaults(&objects.single_action);

    nd_datetime_t now = {.year = 2026, .month = 1, .day = 1,
                         .hour = 0, .minute = 0, .second = 0};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_single_action_on_second(
                                 &objects.single_action, &port,
                                 &objects.scripts, 1, &now));
    TEST_ASSERT_EQUAL_UINT16(
        1, nd_profile_store_entries(&objects.profiles.items[0].store));
    TEST_ASSERT_EQUAL_UINT32(1, objects.event.counters[1]);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_single_action_on_second(
                                 &objects.single_action, &port,
                                 &objects.scripts, 1, &now));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults_and_set_round_trip);
    RUN_TEST(test_default_billing_schedule);
    RUN_TEST(test_default_billing_schedule_captures);
    RUN_TEST(test_tick_executes_script_once);
    RUN_TEST(test_single_action_persists);
    return UNITY_END();
}
