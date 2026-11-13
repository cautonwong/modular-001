#include "unity.h"

#include <string.h>

#include "components/protocol/dlms/include/nd_axdr.h"

static uint8_t buffer[64];

void setUp(void)
{
    memset(buffer, 0, sizeof(buffer));
}

void tearDown(void) {}

static void test_length_forms(void)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, sizeof(buffer));
    nd_axdr_put_length(&writer, 0x7F);
    nd_axdr_put_length(&writer, 0x80);
    nd_axdr_put_length(&writer, 0x100);
    TEST_ASSERT_EQUAL_INT(0, writer.err);
    TEST_ASSERT_EQUAL_UINT8(0x7F, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x81, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x80, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x82, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);

    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, buffer, 6);
    TEST_ASSERT_EQUAL_UINT16(0x7F, nd_axdr_get_length(&reader));
    TEST_ASSERT_EQUAL_UINT16(0x80, nd_axdr_get_length(&reader));
    TEST_ASSERT_EQUAL_UINT16(0x100, nd_axdr_get_length(&reader));
    TEST_ASSERT_EQUAL_INT(0, reader.err);
}

static void test_scalars_roundtrip(void)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, sizeof(buffer));
    nd_axdr_put_tag(&writer, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&writer, 0xA5);
    nd_axdr_put_tag(&writer, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&writer, 0xBEEF);
    nd_axdr_put_tag(&writer, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&writer, 0x12345678u);
    nd_axdr_put_tag(&writer, ND_AXDR_INTEGER);
    nd_axdr_put_i16(&writer, -2);
    TEST_ASSERT_EQUAL_INT(0, writer.err);

    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, buffer, writer.len);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_UNSIGNED, nd_axdr_get_tag(&reader));
    TEST_ASSERT_EQUAL_UINT8(0xA5, nd_axdr_get_u8(&reader));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG_UNSIGNED, nd_axdr_get_tag(&reader));
    TEST_ASSERT_EQUAL_UINT16(0xBEEF, nd_axdr_get_u16(&reader));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_DOUBLE_LONG_UNSIGNED,
                            nd_axdr_get_tag(&reader));
    TEST_ASSERT_EQUAL_UINT32(0x12345678u, nd_axdr_get_u32(&reader));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_INTEGER, nd_axdr_get_tag(&reader));
    TEST_ASSERT_EQUAL_INT16(-2, nd_axdr_get_i16(&reader));
    TEST_ASSERT_EQUAL_INT(0, reader.err);
    TEST_ASSERT_EQUAL_size_t(0, nd_axdr_remaining(&reader));
}

static void test_nested_skip(void)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_NULL);
    nd_axdr_put_structure_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&writer, 3);
    nd_axdr_put_octets(&writer, (const uint8_t *)"abc", 3);
    nd_axdr_put_tag(&writer, ND_AXDR_BIT_STRING);
    nd_axdr_put_length(&writer, 9);
    nd_axdr_put_octets(&writer, (const uint8_t *)"\xAA\x80", 2);
    TEST_ASSERT_EQUAL_INT(0, writer.err);

    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, buffer, writer.len);
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_skip_value(&reader));
    TEST_ASSERT_EQUAL_size_t(0, nd_axdr_remaining(&reader));
    TEST_ASSERT_EQUAL_INT(0, reader.err);
}

static void test_strings_and_datetime_skip(void)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, sizeof(buffer));
    nd_axdr_put_tag(&writer, ND_AXDR_VISIBLE_STRING);
    nd_axdr_put_length(&writer, 4);
    nd_axdr_put_octets(&writer, (const uint8_t *)"test", 4);
    nd_axdr_put_tag(&writer, ND_AXDR_DATE_TIME);
    for (uint8_t index = 0; index < 12; index++) {
        nd_axdr_put_u8(&writer, index);
    }
    TEST_ASSERT_EQUAL_INT(0, writer.err);

    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, buffer, writer.len);
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_skip_value(&reader));
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_skip_value(&reader));
    TEST_ASSERT_EQUAL_size_t(0, nd_axdr_remaining(&reader));
}

static void test_truncated_input_fails(void)
{
    uint8_t short_buffer[3] = {0x01, 0x02, 0x03};
    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, short_buffer, sizeof(short_buffer));
    (void)nd_axdr_get_u32(&reader);
    TEST_ASSERT_EQUAL_INT(1, reader.err);
    TEST_ASSERT_EQUAL_size_t(0, nd_axdr_remaining(&reader));

    nd_axdr_reader_init(&reader, short_buffer, sizeof(short_buffer));
    TEST_ASSERT_EQUAL_INT(-1, nd_axdr_skip_value(&reader));
}

static void test_writer_overflow(void)
{
    uint8_t small[2];
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, small, sizeof(small));
    nd_axdr_put_u32(&writer, 1);
    TEST_ASSERT_EQUAL_INT(1, writer.err);
    TEST_ASSERT_EQUAL_size_t(0, writer.len);
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_writer_ok(&writer));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_length_forms);
    RUN_TEST(test_scalars_roundtrip);
    RUN_TEST(test_nested_skip);
    RUN_TEST(test_strings_and_datetime_skip);
    RUN_TEST(test_truncated_input_fails);
    RUN_TEST(test_writer_overflow);
    return UNITY_END();
}
