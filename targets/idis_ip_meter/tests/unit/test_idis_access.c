#include "unity.h"

#include <string.h>

#include "components/protocol/dlms/include/nd_axdr.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_ic_function_control_init(&objects.functions);
    objects.access_enforced = 1;
}

void tearDown(void) {}

static uint8_t function_parameter[24];
static size_t function_parameter_len;

static void make_disable_parameter(void)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, function_parameter, sizeof(function_parameter));
    nd_axdr_put_array_header(&writer, 1);
    nd_axdr_put_structure_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&writer, 7);
    nd_axdr_put_octets(&writer, (const uint8_t *)"LPCAP_1", 7);
    nd_axdr_put_tag(&writer, ND_AXDR_BOOLEAN);
    nd_axdr_put_u8(&writer, 0);
    function_parameter_len = writer.len;
}

static int get_function(uint8_t sap)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    return nd_cosem_port_get(&objects, sap, ND_IC_FUNCTION_CONTROL,
                             &ND_OBIS_FUNCTION_CONTROL, 2, NULL, &out);
}

static int action_function(uint8_t sap)
{
    make_disable_parameter();
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, function_parameter, function_parameter_len);
    return nd_cosem_port_action(&objects, sap, ND_IC_FUNCTION_CONTROL,
                                &ND_OBIS_FUNCTION_CONTROL, 1, &in, NULL);
}

static void test_public_sap_is_get_only(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK, get_function(16));
    TEST_ASSERT_EQUAL_INT(ND_ERR_ACCESS_DENIED, action_function(16));
}

static void test_management_sap_can_act(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK, action_function(1));
    TEST_ASSERT_EQUAL_UINT8(0, objects.functions.local.functions[0].enabled);
}

static void test_preestablished_sap_cannot_get(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ERR_ACCESS_DENIED, get_function(102));
}

static void test_cip_sap_has_no_request_access(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ERR_ACCESS_DENIED, get_function(103));
    TEST_ASSERT_EQUAL_INT(ND_ERR_ACCESS_DENIED, action_function(103));
}

static void test_policy_is_opt_in_for_unit_tests(void)
{
    objects.access_enforced = 0;
    TEST_ASSERT_EQUAL_INT(ND_OK, action_function(16));
    TEST_ASSERT_EQUAL_UINT8(0, objects.functions.local.functions[0].enabled);
}


static void test_remote_client_cannot_execute_local_script(void)
{
    nd_idis_script_init(&objects.scripts);
    objects.remote_client = 1;
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_action(&objects, 1, ND_IC_SCRIPT_TABLE,
                             &ND_OBIS_SCRIPT_DISCONNECT, 1, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_action(&objects, 1, ND_IC_SCRIPT_TABLE,
                             &ND_OBIS_SCRIPT_LOAD_MGMT, 1, NULL, NULL));
}


static void test_internal_script_execution_uses_management_sap(void)
{
    nd_idis_script_init(&objects.scripts);
    nd_ic_register_activation_init(&objects.activation);
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_script_bind_port(&objects.scripts, &port);
    int result = nd_idis_script_execute(&objects.scripts, &port, 1,
                                        &ND_OBIS_SCRIPT_TARIFF, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_public_sap_is_get_only);
    RUN_TEST(test_management_sap_can_act);
    RUN_TEST(test_preestablished_sap_cannot_get);
    RUN_TEST(test_cip_sap_has_no_request_access);
    RUN_TEST(test_policy_is_opt_in_for_unit_tests);
    RUN_TEST(test_remote_client_cannot_execute_local_script);
    RUN_TEST(test_internal_script_execution_uses_management_sap);
    return UNITY_END();
}
