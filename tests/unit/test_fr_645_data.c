#include "unity.h"

#include <string.h>

#include "application/adapters/dl645/fr_645_data.h"

void setUp(void) {}
void tearDown(void) {}

static void test_645_91_data_maps_id_length_and_payload(void)
{
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB};
    nd_dl645_msg_t mesg;

    memset(&mesg, 0, sizeof(mesg));
    fr_645_parse_91_data(data, sizeof(data), &mesg, 12);

    TEST_ASSERT_EQUAL_UINT8(12, mesg.to);
    TEST_ASSERT_EQUAL_UINT8(0x91, mesg.ctrl);
    TEST_ASSERT_EQUAL_UINT32(0x04030201u, mesg.id);
    TEST_ASSERT_EQUAL_UINT16(10, mesg.len);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mesg.payload[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, mesg.payload[1]);
}

static void test_645_91_data_preserves_short_payload_length(void)
{
    const uint8_t data[] = {0x10, 0x20, 0x30, 0x40};
    nd_dl645_msg_t mesg;

    memset(&mesg, 0, sizeof(mesg));
    fr_645_parse_91_data(data, sizeof(data), &mesg, 12);

    TEST_ASSERT_EQUAL_UINT32(0x40302010u, mesg.id);
    TEST_ASSERT_EQUAL_UINT16(8, mesg.len);
    TEST_ASSERT_TRUE(mesg.payload == data + 4);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_645_91_data_maps_id_length_and_payload);
    RUN_TEST(test_645_91_data_preserves_short_payload_length);
    return UNITY_END();
}
