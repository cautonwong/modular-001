#include "unity.h"
#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_dlms_datetime_t now;

static int clock_read(void *ctx, nd_dlms_datetime_t *out)
{
    (void)ctx;
    *out = now;
    return 0;
}

static int clock_write(void *ctx, const nd_dlms_datetime_t *value)
{
    (void)ctx;
    now = *value;
    return 0;
}

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    now = (nd_dlms_datetime_t){
        .year = 2026, .month = 11, .day = 11, .hour = 10, .minute = 7,
        .second = 31, .status = 0,
    };
    nd_clock_port_t port = {.read = clock_read, .write = clock_write};
    nd_idis_clock_init(&objects.clock, &port);
    objects.clock.config.time_zone = 60;
}

void tearDown(void) {}

static void test_clock_config_attributes_and_set(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_CLOCK,
                                            &ND_OBIS_CLOCK, 3, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(60, buffer[2]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_CLOCK,
                                            &ND_OBIS_CLOCK, 9, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);

    uint8_t set_buffer[8];
    nd_axdr_writer_t set;
    nd_axdr_writer_init(&set, set_buffer, sizeof(set_buffer));
    nd_axdr_put_tag(&set, ND_AXDR_LONG);
    nd_axdr_put_i16(&set, 120);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, set_buffer, set.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_CLOCK,
                                            &ND_OBIS_CLOCK, 3, &in));
    TEST_ASSERT_EQUAL_INT16(120, objects.clock.config.time_zone);
}

static void test_clock_adjust_methods(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_CLOCK,
                                               &ND_OBIS_CLOCK, 3, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT8(8, now.minute);
    TEST_ASSERT_EQUAL_UINT8(0, now.second);
    now.minute = 7;
    now.second = 31;
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_CLOCK,
                                               &ND_OBIS_CLOCK, 1, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT8(15, now.minute);
    TEST_ASSERT_EQUAL_UINT8(0, now.second);
    uint8_t shift_buffer[4];
    nd_axdr_writer_t shift;
    nd_axdr_writer_init(&shift, shift_buffer, sizeof(shift_buffer));
    nd_axdr_put_tag(&shift, ND_AXDR_INTEGER);
    nd_axdr_put_u8(&shift, (uint8_t)(int8_t)-60);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, shift_buffer, shift.len);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_CLOCK,
                                               &ND_OBIS_CLOCK, 6, &in, NULL));
    TEST_ASSERT_EQUAL_UINT8(14, now.minute);
}

static void test_clock_preset_round_trip(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_CLOCK,
                                               &ND_OBIS_CLOCK, 5, NULL, NULL));
    now.hour = 12;
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_CLOCK,
                                               &ND_OBIS_CLOCK, 4, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT8(10, now.hour);
    TEST_ASSERT_EQUAL_UINT8(7, now.minute);
}


static void test_clock_config_persists(void)
{
    static uint8_t memory[256];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    objects.clock.config.time_zone = 120;
    objects.clock.config.daylight_deviation = 60;
    objects.clock.config.clock_base = 2;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_clock_save(
                                 &objects.clock, nd_byte_store_port(&store), 0));
    nd_idis_clock_state_t restored = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_idis_clock_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_INT16(120, restored.config.time_zone);
    TEST_ASSERT_EQUAL_INT16(60, restored.config.daylight_deviation);
    TEST_ASSERT_EQUAL_UINT8(2, restored.config.clock_base);
    TEST_ASSERT_EQUAL_UINT16(900, restored.measuring_period);
}

static void test_clock_shift_limit_persists(void)
{
    static uint8_t memory[256];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    objects.clock.time_shift_limit = 120;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_clock_save(
                                 &objects.clock, nd_byte_store_port(&store), 0));
    nd_idis_clock_state_t restored = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_idis_clock_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT16(120, restored.time_shift_limit);

    /* Zero was the old reserved value; it keeps the default limit. */
    objects.clock.time_shift_limit = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_clock_save(
                                 &objects.clock, nd_byte_store_port(&store), 0));
    nd_idis_clock_state_t legacy = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_idis_clock_load(
                                 &legacy, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT16(60, legacy.time_shift_limit);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clock_config_attributes_and_set);
    RUN_TEST(test_clock_adjust_methods);
    RUN_TEST(test_clock_preset_round_trip);
    RUN_TEST(test_clock_config_persists);
    RUN_TEST(test_clock_shift_limit_persists);
    return UNITY_END();
}
