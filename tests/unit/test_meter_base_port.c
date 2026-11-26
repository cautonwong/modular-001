#include "unity.h"

#include <string.h>

#include "application/ports/meter_base.h"
#include "application/ports/resource_access.h"
#include "targets/stategrid_meter/app/meter_base_port.h"
#include "targets/stategrid_meter/app/meter_wire_status.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"

static stategrid_context_t context;
static struct mbase_control_block_t block;

const nd_resource_port_t *nd_fr698_resource_access(void *instance)
{
    (void)instance;
    return NULL;
}

void setUp(void)
{
    memset(&context, 0, sizeof(context));
    memset(&block, 0, sizeof(block));
    context.mbase = &block;
    stategrid_meter_base_attach(&context);
}

void tearDown(void)
{
}

static void test_status_reads_map_wire_to_semantic(void)
{
    *nd_mb_esam_chip_abnormal_ptr(&context) = ND_WIRE_STATUS_SET;
    TEST_ASSERT_EQUAL_INT(1, nd_mb_get_esam_chip_abnormal(&context));

    *nd_mb_esam_chip_abnormal_ptr(&context) = ND_WIRE_STATUS_RESET;
    TEST_ASSERT_EQUAL_INT(0, nd_mb_get_esam_chip_abnormal(&context));
}

static void test_status_wire_mapping_both_directions(void)
{
    TEST_ASSERT_EQUAL_HEX8(ND_WIRE_STATUS_SET,
                           nd_wire_status_from_semantic(1));
    TEST_ASSERT_EQUAL_HEX8(ND_WIRE_STATUS_RESET,
                           nd_wire_status_from_semantic(0));
    TEST_ASSERT_EQUAL_INT(1, nd_wire_status_to_semantic(ND_WIRE_STATUS_SET));
    TEST_ASSERT_EQUAL_INT(0, nd_wire_status_to_semantic(ND_WIRE_STATUS_RESET));
}

static void test_power_reads_map_wire_codes(void)
{
    context.ac_power_status = ND_WIRE_POWER_ON;
    TEST_ASSERT_EQUAL_INT(1, nd_mb_get_power_state(&context));

    context.ac_power_status = ND_WIRE_POWER_OFF;
    TEST_ASSERT_EQUAL_INT(0, nd_mb_get_power_state(&context));

    TEST_ASSERT_EQUAL_HEX8(ND_WIRE_POWER_ON,
                           nd_wire_power_from_semantic(1));
    TEST_ASSERT_EQUAL_HEX8(ND_WIRE_POWER_OFF,
                           nd_wire_power_from_semantic(0));
}

static void test_array_index_selects_channel_and_client(void)
{
    nd_mb_set_connect_timeout(&context, 1, 0, 77);
    TEST_ASSERT_EQUAL_INT(77, nd_mb_get_connect_timeout(&context, 1, 0));
    TEST_ASSERT_EQUAL_UINT32(77, nd_mb_connect_timeout_ptr(&context)[1][0]);
    TEST_ASSERT_EQUAL_INT(0, nd_mb_get_connect_timeout(&context, 0, 0));

    nd_mb_set_connect_status(&context, 0, 1, 1);
    TEST_ASSERT_EQUAL_INT(1, nd_mb_get_connect_status(&context, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(1, nd_mb_connect_status_ptr(&context)[0][1]);
}

static void test_byte_buffer_round_trip(void)
{
    static const uint8_t tsa[6] = { 1, 2, 3, 4, 5, 6 };
    uint8_t readback[6] = { 0 };

    memcpy(nd_mb_tsa_ptr(&context), tsa, sizeof(tsa));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(tsa, nd_mb_tsa_ptr(&context), sizeof(tsa));

    nd_mb_get_tsa(&context, readback, sizeof(readback));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(tsa, readback, sizeof(tsa));
}

static void test_scalar_round_trip(void)
{
    nd_mb_set_report_piid(&context, 9);
    TEST_ASSERT_EQUAL_INT(9, nd_mb_get_report_piid(&context));
    TEST_ASSERT_EQUAL_UINT8(9, *nd_mb_report_piid_ptr(&context));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_status_reads_map_wire_to_semantic);
    RUN_TEST(test_status_wire_mapping_both_directions);
    RUN_TEST(test_power_reads_map_wire_codes);
    RUN_TEST(test_array_index_selects_channel_and_client);
    RUN_TEST(test_byte_buffer_round_trip);
    RUN_TEST(test_scalar_round_trip);
    return UNITY_END();
}
