#include "unity.h"
#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_idis_calendar_init(&objects.calendar);
}

void tearDown(void) {}

static size_t make_insert(uint8_t *buffer, size_t capacity, uint16_t index,
                          uint16_t year, uint8_t month, uint8_t day,
                          uint8_t day_id)
{
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, capacity);
    nd_axdr_put_structure_header(&out, 3);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, index);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 5);
    nd_axdr_put_u16(&out, year);
    nd_axdr_put_u8(&out, month);
    nd_axdr_put_u8(&out, day);
    nd_axdr_put_u8(&out, 0);
    nd_axdr_put_tag(&out, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&out, day_id);
    return out.len;
}

static void test_insert_overrides_and_selects_day(void)
{
    uint8_t buffer[32];
    size_t length = make_insert(buffer, sizeof(buffer), 1, 2026, 11, 11, 2);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, length);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SPECIAL_DAYS,
                                               &ND_OBIS_SPECIAL_DAYS, 1, &in,
                                               NULL));
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_UINT8(
        2, nd_calendar_day_id(&objects.calendar.calendar.active, &now));

    length = make_insert(buffer, sizeof(buffer), 1, 2026, 11, 11, 3);
    nd_axdr_reader_init(&in, buffer, length);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SPECIAL_DAYS,
                                               &ND_OBIS_SPECIAL_DAYS, 1, &in,
                                               NULL));
    TEST_ASSERT_EQUAL_UINT8(1, objects.calendar.special_day_count);
    TEST_ASSERT_EQUAL_UINT8(3, objects.calendar.special_days[0].day_id);
}

static void test_delete_and_get_entries(void)
{
    uint8_t buffer[32];
    size_t length = make_insert(buffer, sizeof(buffer), 1, 2026, 11, 11, 2);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, length);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SPECIAL_DAYS,
                                               &ND_OBIS_SPECIAL_DAYS, 1, &in,
                                               NULL));
    uint8_t get_buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, get_buffer, sizeof(get_buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_SPECIAL_DAYS,
                                            &ND_OBIS_SPECIAL_DAYS, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, get_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, get_buffer[1]);

    nd_axdr_writer_t del;
    nd_axdr_writer_init(&del, buffer, sizeof(buffer));
    nd_axdr_put_tag(&del, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&del, 1);
    nd_axdr_reader_init(&in, buffer, del.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SPECIAL_DAYS,
                                               &ND_OBIS_SPECIAL_DAYS, 2, &in,
                                               NULL));
    TEST_ASSERT_EQUAL_UINT8(0, objects.calendar.special_day_count);
}


static void test_special_days_persist(void)
{
    uint8_t buffer[32];
    size_t length = make_insert(buffer, sizeof(buffer), 1, 2026, 11, 11, 2);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, length);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SPECIAL_DAYS,
                                               &ND_OBIS_SPECIAL_DAYS, 1, &in,
                                               NULL));
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_calendar_save(
                                 &objects.calendar, nd_byte_store_port(&store),
                                 0));
    nd_idis_calendar_state_t restored;
    nd_idis_calendar_init(&restored);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_calendar_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(1, restored.special_day_count);
    TEST_ASSERT_EQUAL_UINT8(2, restored.special_days[0].day_id);
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_UINT8(2,
                            nd_calendar_day_id(&restored.calendar.active, &now));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_overrides_and_selects_day);
    RUN_TEST(test_delete_and_get_entries);
    RUN_TEST(test_special_days_persist);
    return UNITY_END();
}
