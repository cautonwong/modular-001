#include "unity.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_idis_calendar_init(&objects.calendar);
    for (uint8_t index = 0; index < 3; index++) {
        nd_idis_calendar_init(&objects.calendar_extra[index]);
    }
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

static void set_name(const char *name, uint8_t length)
{
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    put_string(&out, name, length);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 6, &in));
}

static void set_season(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 3);
    put_string(&out, "S1", 2);
    put_datetime(&out, 2026, 1, 1);
    put_string(&out, "W1", 2);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 7, &in));
}

static void set_week(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 8);
    put_string(&out, "W1", 2);
    for (uint8_t day = 0; day < 7; day++) {
        nd_axdr_put_tag(&out, ND_AXDR_UNSIGNED);
        nd_axdr_put_u8(&out, 1);
    }
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 8, &in));
}

static void set_day(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&out, 1);
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 3);
    const uint8_t time[4] = {8, 0, 0, 0};
    put_string(&out, time, sizeof(time));
    put_string(&out, "\1\2\3\4\5\6", 6);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 2);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 9, &in));
}

static void set_activation_time(void)
{
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    put_datetime(&out, 2026, 11, 12);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 10, &in));
}

static void test_passive_activation_and_get(void)
{
    set_name("P1", 2);
    set_season();
    set_week();
    set_day();
    set_activation_time();
    uint8_t buffer[128];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]); /* active name empty */
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_ACTIVITY_CALENDAR,
                                               &ND_OBIS_ACTIVITY_CALENDAR, 1,
                                               NULL, NULL));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8('P', buffer[2]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDAR, 5,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
}

static void test_time_activation(void)
{
    set_name("P2", 2);
    set_activation_time();
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 12};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_calendar_tick(&objects.calendar, &now));
    TEST_ASSERT_EQUAL_UINT8(2, objects.calendar.calendar.active.name.len);
    now.day = 11;
    set_name("P3", 2);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_calendar_tick(&objects.calendar, &now));
}

static void test_extra_calendar_and_special_day_instances(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDARS[1], 1,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_MEMORY(&ND_OBIS_ACTIVITY_CALENDARS[1], buffer + 2, 6);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_SPECIAL_DAYS,
                                            &ND_OBIS_SPECIAL_DAYS_TABLES[2], 1,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_MEMORY(&ND_OBIS_SPECIAL_DAYS_TABLES[2], buffer + 2, 6);

    /* The main and the first extra calendar are distinct state instances. */
    objects.calendar_extra[0].active.name.len = 5;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDARS[1], 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(5, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ACTIVITY_CALENDAR,
                                            &ND_OBIS_ACTIVITY_CALENDARS[0], 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
}


static void test_default_tariff_calendar(void)
{
    nd_idis_calendar_apply_defaults(&objects.calendar, &ND_OBIS_SCRIPT_TARIFF, 1);
    TEST_ASSERT_EQUAL_UINT8(1, objects.calendar.active.season_count);
    TEST_ASSERT_EQUAL_UINT8(1, objects.calendar.active.week_count);
    TEST_ASSERT_EQUAL_UINT8(1, objects.calendar.active.day_count);
    TEST_ASSERT_EQUAL_UINT8(1, objects.calendar.active.action_count[0]);

    nd_datetime_t now = {.year = 2026, .month = 6, .day = 17,
                         .hour = 12, .minute = 0, .second = 0};
    uint8_t day_id = nd_calendar_day_id(&objects.calendar.calendar.active,
                                        &now);
    const nd_calendar_day_t *day =
        nd_calendar_find_day(&objects.calendar.calendar.active, day_id);
    TEST_ASSERT_NOT_NULL(day);
    const nd_calendar_action_t *action = nd_calendar_select_action(day, &now);
    TEST_ASSERT_NOT_NULL(action);
    TEST_ASSERT_EQUAL_UINT8(0, action->start.hour);
    TEST_ASSERT_EQUAL_UINT8(0, action->start.minute);
    TEST_ASSERT_EQUAL_UINT16(1, action->script_selector);
    TEST_ASSERT_EQUAL_MEMORY(&ND_OBIS_SCRIPT_TARIFF, action->script_ln, 6);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_passive_activation_and_get);
    RUN_TEST(test_time_activation);
    RUN_TEST(test_extra_calendar_and_special_day_instances);
    RUN_TEST(test_default_tariff_calendar);
    return UNITY_END();
}
