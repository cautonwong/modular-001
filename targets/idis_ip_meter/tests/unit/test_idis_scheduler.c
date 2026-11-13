#include "unity.h"

#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/cosem/nd_idis_scheduler.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_clock_port_t clock_port;
static nd_cosem_port_t port;

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 1, .day = 1};
    return 0;
}

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    clock_port = (nd_clock_port_t){.ctx = NULL, .read = fake_clock};
    objects.clock.port = &clock_port;
    nd_disconnect_init(&objects.disconnect, 2, NULL);
    nd_idis_calendar_init(&objects.calendar);
    nd_idis_script_init(&objects.scripts);
    nd_idis_scheduler_init(&objects.scheduler);
    port = (nd_cosem_port_t){
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_script_bind_port(&objects.scripts, &port);
}

void tearDown(void) {}

static void put_string(nd_axdr_writer_t *out, const void *data, uint8_t length)
{
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, length);
    nd_axdr_put_octets(out, data, length);
}

static void put_datetime(nd_axdr_writer_t *out, uint16_t year, uint8_t month,
                         uint8_t day)
{
    uint8_t data[12] = {0};
    data[0] = (uint8_t)(year >> 8);
    data[1] = (uint8_t)year;
    data[2] = month;
    data[3] = day;
    put_string(out, data, sizeof(data));
}

static void calendar_set(uint8_t attribute_id, nd_axdr_writer_t *out)
{
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, out->buf, out->len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR,
                                            attribute_id, &in));
}

static void install_schedule(void)
{
    uint8_t buffer[128];
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    put_string(&out, "P1", 2);
    calendar_set(6, &out);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 3);
    put_string(&out, "S1", 2);
    put_datetime(&out, 2026, 1, 1);
    put_string(&out, "W1", 2);
    calendar_set(7, &out);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 8);
    put_string(&out, "W1", 2);
    for (uint8_t day = 0; day < 7; day++) {
        nd_axdr_put_tag(&out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(&out, 1);
    }
    calendar_set(8, &out);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&out, 1);
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 3);
    const uint8_t start[4] = {0, 0, 0, 0};
    put_string(&out, start, sizeof(start));
    const uint8_t script[6] = {0, 0, 10, 0, 106, 255};
    put_string(&out, script, sizeof(script));
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 1);
    calendar_set(9, &out);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    put_datetime(&out, 2026, 1, 1);
    calendar_set(10, &out);
}

static void activate(int *tick_result)
{
    nd_datetime_t now = {.year = 2026, .month = 1, .day = 1};
    *tick_result = nd_idis_calendar_tick(&objects.calendar, &now);
}

static void test_action_runs_once_at_start_time(void)
{
    install_schedule();
    int tick_result = 0;
    activate(&tick_result);
    TEST_ASSERT_EQUAL_INT(1, tick_result);

    nd_datetime_t now = {.year = 2026, .month = 1, .day = 1,
                         .hour = 0, .minute = 0, .second = 0};
    TEST_ASSERT_EQUAL_INT(1,
                          nd_idis_scheduler_on_second(
                              &objects.scheduler, &objects.calendar,
                              &objects.scripts, &port, 16, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_UINT32(1, objects.scheduler.executed);

    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_scheduler_on_second(
                              &objects.scheduler, &objects.calendar,
                              &objects.scripts, &port, 16, &now));
    now.second = 5;
    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_scheduler_on_second(
                              &objects.scheduler, &objects.calendar,
                              &objects.scripts, &port, 16, &now));
    TEST_ASSERT_EQUAL_UINT32(1, objects.scheduler.executed);
}

static void test_action_runs_again_next_day(void)
{
    install_schedule();
    int tick_result = 0;
    activate(&tick_result);
    nd_datetime_t now = {.year = 2026, .month = 1, .day = 1};
    TEST_ASSERT_EQUAL_INT(1,
                          nd_idis_scheduler_on_second(
                              &objects.scheduler, &objects.calendar,
                              &objects.scripts, &port, 16, &now));
    nd_datetime_t reconnect = now;
    TEST_ASSERT_EQUAL_INT(0,
                          nd_disconnect_request(&objects.disconnect,
                                                ND_DISCONNECT_LOCAL_RECONNECT,
                                                &reconnect));
    now.day = 2;
    TEST_ASSERT_EQUAL_INT(1,
                          nd_idis_scheduler_on_second(
                              &objects.scheduler, &objects.calendar,
                              &objects.scripts, &port, 16, &now));
    TEST_ASSERT_EQUAL_UINT32(2, objects.scheduler.executed);
}

static void test_no_schedule_does_nothing(void)
{
    nd_datetime_t now = {.year = 2026, .month = 1, .day = 1};
    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_scheduler_on_second(
                              &objects.scheduler, &objects.calendar,
                              &objects.scripts, &port, 16, &now));
}

static void test_multi_runs_each_calendar_once(void)
{
    install_schedule();
    int tick_result = 0;
    activate(&tick_result);
    nd_idis_calendar_init(&objects.calendar_extra[0]);
    const nd_idis_calendar_state_t *calendars[2] = {
        &objects.calendar, &objects.calendar_extra[0]};
    nd_datetime_t now = {.year = 2026, .month = 1, .day = 1,
                         .hour = 0, .minute = 0, .second = 0};
    TEST_ASSERT_EQUAL_INT(
        1, nd_idis_scheduler_on_second_multi(&objects.scheduler, calendars, 2,
                                             &objects.scripts, &port, 16,
                                             &now));
    TEST_ASSERT_EQUAL_UINT32(1, objects.scheduler.executed);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_action_runs_once_at_start_time);
    RUN_TEST(test_action_runs_again_next_day);
    RUN_TEST(test_multi_runs_each_calendar_once);
    RUN_TEST(test_no_schedule_does_nothing);
    return UNITY_END();
}
