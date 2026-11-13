#include "unity.h"

#include "application/adapters/dl698/security_encoder.h"

void setUp(void) {}
void tearDown(void) {}

static void test_mac_tail_writes_type_length_and_four_bytes(void)
{
    uint8_t buffer[16] = {0};
    size_t offset = 3;
    const uint8_t mac[4] = {0x11, 0x22, 0x33, 0x44};

    TEST_ASSERT_EQUAL_INT(0, nd_security_encode_tail(buffer, sizeof(buffer),
                                                     offset, mac, 4,
                                                     &offset));
    TEST_ASSERT_EQUAL_INT(10, offset);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x44, buffer[9]);
}

static void test_no_mac_tail_writes_single_zero(void)
{
    uint8_t buffer[8] = {0};
    size_t offset = 2;

    TEST_ASSERT_EQUAL_INT(0, nd_security_encode_tail(buffer, sizeof(buffer),
                                                     offset, NULL, 0,
                                                     &offset));
    TEST_ASSERT_EQUAL_INT(3, offset);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
}

static void test_tail_rejects_unsupported_or_short_input(void)
{
    uint8_t buffer[4] = {0};
    size_t offset = 0;
    const uint8_t mac[4] = {1, 2, 3, 4};

    TEST_ASSERT_EQUAL_INT(-1, nd_security_encode_tail(buffer, sizeof(buffer),
                                                      offset, mac, 4,
                                                      &offset));
    TEST_ASSERT_EQUAL_INT(-1, nd_security_encode_tail(buffer, sizeof(buffer),
                                                      offset, NULL, 4,
                                                      &offset));
    TEST_ASSERT_EQUAL_INT(-1, nd_security_encode_tail(buffer, sizeof(buffer),
                                                      offset, mac, 3,
                                                      &offset));
    TEST_ASSERT_EQUAL_INT(-1, nd_security_encode_tail(buffer, sizeof(buffer),
                                                      offset, NULL, 0,
                                                      NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mac_tail_writes_type_length_and_four_bytes);
    RUN_TEST(test_no_mac_tail_writes_single_zero);
    RUN_TEST(test_tail_rejects_unsupported_or_short_input);
    return UNITY_END();
}
