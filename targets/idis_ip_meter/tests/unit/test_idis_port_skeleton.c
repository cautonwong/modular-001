#include "unity.h"

#include "components/protocol/dlms/include/nd_dlms_server.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_clock.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
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
        .day_of_week = 3,
        .hour = 8,
        .minute = 30,
        .second = 0,
        .hundredths = 0,
        .deviation = 0,
        .status = 0,
    };
    return 0;
}

static nd_cosem_port_t make_port(nd_idis_objects_t *objects)
{
    return (nd_cosem_port_t){
        .ctx = objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
}

static void test_clock_get_writes_twelve_byte_datetime(void)
{
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_idis_objects_t objects = {.clock = {.port = &clock_port}};
    nd_cosem_port_t port = make_port(&objects);
    nd_dlms_server_t server = {.port = &port};
    uint8_t buffer[32] = {0};
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    int result = nd_dlms_server_get(&server, 16, ND_IC_CLOCK, &ND_OBIS_CLOCK,
                                    2, NULL, &out);

    TEST_ASSERT_EQUAL_INT(ND_OK, result);
    TEST_ASSERT_EQUAL_size_t(13, out.len);
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x07, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xEA, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(11, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(11, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(8, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(30, buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[9]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[10]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[11]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[12]);
}

static void test_clock_logical_name_is_six_bytes(void)
{
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_idis_objects_t objects = {.clock = {.port = &clock_port}};
    uint8_t buffer[8] = {0};
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    int result = nd_cosem_port_get(&objects, 16, ND_IC_CLOCK, &ND_OBIS_CLOCK, 1,
                                   NULL, &out);

    TEST_ASSERT_EQUAL_INT(ND_OK, result);
    TEST_ASSERT_EQUAL_size_t(8, out.len);
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(6, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(255, buffer[7]);
}

static void test_unknown_class_and_attribute_return_dlms_errors(void)
{
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_idis_objects_t objects = {.clock = {.port = &clock_port}};
    uint8_t buffer[8] = {0};
    nd_axdr_writer_t out;

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OBJECT_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, 999, &ND_OBIS_CLOCK,
                                            2, NULL, &out));
    TEST_ASSERT_EQUAL_INT(ND_ERR_ATTRIBUTE_UNDEFINED,
                          nd_cosem_port_get(&objects, 16, ND_IC_CLOCK,
                                            &ND_OBIS_CLOCK, 99, NULL, &out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clock_get_writes_twelve_byte_datetime);
    RUN_TEST(test_clock_logical_name_is_six_bytes);
    RUN_TEST(test_unknown_class_and_attribute_return_dlms_errors);
    return UNITY_END();
}
