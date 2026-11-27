#include "unity.h"

#include "application/adapters/dl698/fr_axdr_codec.h"

void setUp(void) {}
void tearDown(void) {}

static void test_axdr_decode_short_and_long_forms(void)
{
    uint16_t len = 0;
    const uint8_t short_form[] = {0x05, 0x00};
    const uint8_t form_81[] = {0x81, 0x80, 0x00};
    const uint8_t form_82[] = {0x82, 0x01, 0x20, 0x00};
    const uint8_t invalid[] = {0x83, 0x00};

    TEST_ASSERT_EQUAL_UINT8(1, axdr_decode(TYPE_octet_string, short_form, &len));
    TEST_ASSERT_EQUAL_UINT16(5, len);
    TEST_ASSERT_EQUAL_UINT8(2, axdr_decode(TYPE_octet_string, form_81, &len));
    TEST_ASSERT_EQUAL_UINT16(0x80, len);
    TEST_ASSERT_EQUAL_UINT8(3, axdr_decode(TYPE_octet_string, form_82, &len));
    TEST_ASSERT_EQUAL_UINT16(0x0120, len);
    TEST_ASSERT_EQUAL_UINT8(0, axdr_decode(TYPE_octet_string, invalid, &len));
    TEST_ASSERT_EQUAL_UINT16(0, len);
}

static void test_axdr_len_encode_boundaries(void)
{
    uint8_t buf[4] = {0};

    TEST_ASSERT_EQUAL_UINT8(1, axdr_len_encode(buf, 127));
    TEST_ASSERT_EQUAL_UINT8(127, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(2, axdr_len_encode(buf, 128));
    TEST_ASSERT_EQUAL_UINT8(0x81, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(128, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(2, axdr_len_encode(buf, 255));
    TEST_ASSERT_EQUAL_UINT8(255, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(3, axdr_len_encode(buf, 256));
    TEST_ASSERT_EQUAL_UINT8(0x82, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buf[2]);
}

static void test_axdr_get_len_scalars_strings_and_array(void)
{
    const uint8_t null_value[] = {TYPE_null};
    const uint8_t long_value[] = {TYPE_long, 0x00, 0x01};
    const uint8_t string_value[] = {
        TYPE_octet_string, 5, 1, 2, 3, 4, 5
    };
    const uint8_t bit_string[] = {TYPE_bit_string, 9, 0xaa, 0x80};
    const uint8_t array_value[] = {
        TYPE_array, 2, TYPE_bool, 0, TYPE_long, 0x00, 0x01
    };

    TEST_ASSERT_EQUAL_UINT16(1, axdr_get_len(null_value, sizeof(null_value)));
    TEST_ASSERT_EQUAL_UINT16(3, axdr_get_len(long_value, sizeof(long_value)));
    TEST_ASSERT_EQUAL_UINT16(7, axdr_get_len(string_value, sizeof(string_value)));
    TEST_ASSERT_EQUAL_UINT16(4, axdr_get_len(bit_string, sizeof(bit_string)));
    TEST_ASSERT_EQUAL_UINT16(7, axdr_get_len(array_value, sizeof(array_value)));
}

static void test_axdr_get_len_rejects_truncated_values(void)
{
    const uint8_t short_string[] = {TYPE_octet_string, 5, 1, 2};
    const uint8_t long_form_string[] = {TYPE_octet_string, 0x82, 0x01, 0x00, 1};
    const uint8_t truncated_array[] = {TYPE_array, 2, TYPE_bool, 0};
    const uint8_t truncated_scalar[] = {TYPE_long, 0};

    TEST_ASSERT_EQUAL_UINT16(0,
        axdr_get_len(short_string, sizeof(short_string)));
    TEST_ASSERT_EQUAL_UINT16(0,
        axdr_get_len(long_form_string, sizeof(long_form_string)));
    TEST_ASSERT_EQUAL_UINT16(0,
        axdr_get_len(truncated_array, sizeof(truncated_array)));
    TEST_ASSERT_EQUAL_UINT16(0,
        axdr_get_len(truncated_scalar, sizeof(truncated_scalar)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_axdr_decode_short_and_long_forms);
    RUN_TEST(test_axdr_len_encode_boundaries);
    RUN_TEST(test_axdr_get_len_scalars_strings_and_array);
    RUN_TEST(test_axdr_get_len_rejects_truncated_values);
    return UNITY_END();
}
