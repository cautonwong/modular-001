#include "unity.h"

#include <string.h>

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_ic_function_control_init(&objects.functions);
}

void tearDown(void) {}

static void test_function_control_lists_defaults(void)
{
    uint8_t buffer[128];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_get(&objects, 16, ND_IC_FUNCTION_CONTROL,
                                 &ND_OBIS_FUNCTION_CONTROL, 2, NULL, &out));

    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, buffer, out.len);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, nd_axdr_get_tag(&reader));
    TEST_ASSERT_EQUAL_UINT16(3, nd_axdr_get_length(&reader));
}

static void test_function_control_disables_named_function(void)
{
    uint8_t parameter[32];
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, parameter, sizeof(parameter));
    nd_axdr_put_array_header(&writer, 1);
    nd_axdr_put_structure_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&writer, 7);
    nd_axdr_put_octets(&writer, (const uint8_t *)"LPCAP_1", 7);
    nd_axdr_put_tag(&writer, ND_AXDR_BOOLEAN);
    nd_axdr_put_u8(&writer, 0);
    TEST_ASSERT_EQUAL_INT(0, writer.err);

    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, writer.len);
    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_action(&objects, 16, ND_IC_FUNCTION_CONTROL,
                                    &ND_OBIS_FUNCTION_CONTROL, 1, &in, NULL));
    TEST_ASSERT_EQUAL_UINT8(0, objects.functions.local.functions[0].enabled);
    TEST_ASSERT_EQUAL_UINT8(1, objects.functions.local.functions[1].enabled);
}

static void test_function_control_rejects_unknown_name(void)
{
    uint8_t parameter[32];
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, parameter, sizeof(parameter));
    nd_axdr_put_array_header(&writer, 1);
    nd_axdr_put_structure_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&writer, 7);
    nd_axdr_put_octets(&writer, (const uint8_t *)"NOPE___", 7);
    nd_axdr_put_tag(&writer, ND_AXDR_BOOLEAN);
    nd_axdr_put_u8(&writer, 0);

    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, writer.len);
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_TYPE_MISMATCH,
        nd_cosem_port_action(&objects, 16, ND_IC_FUNCTION_CONTROL,
                             &ND_OBIS_FUNCTION_CONTROL, 1, &in, NULL));
    TEST_ASSERT_EQUAL_UINT8(1, objects.functions.local.functions[0].enabled);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_function_control_lists_defaults);
    RUN_TEST(test_function_control_disables_named_function);
    RUN_TEST(test_function_control_rejects_unknown_name);
    return UNITY_END();
}
