#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static uint8_t setup_memory[16384];
static nd_byte_store_t setup_store;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    memset(setup_memory, 0, sizeof(setup_memory));
    nd_byte_store_init(&setup_store, setup_memory, sizeof(setup_memory));
    nd_idis_setup_init(&objects.setup);
    (void)nd_idis_setup_load(&objects.setup,
                             nd_byte_store_port(&setup_store), 0);
    nd_idis_association_init(&objects.association);
}

void tearDown(void) {}

static void test_setup_scalar_round_trip(void)
{
    const nd_obis_t port = {0, 0, 20, 0, 0, 255};
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_IEC_LOCAL_PORT,
                                            &port, 1, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(6, buffer[1]);

    uint8_t parameter[2] = {ND_AXDR_ENUM, 3};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_set(&objects, 16, ND_IC_IEC_LOCAL_PORT,
                                            &port, 3, &in));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_IEC_LOCAL_PORT,
                                            &port, 3, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
}

static void test_setup_persists(void)
{
    const nd_obis_t port = {0, 0, 20, 0, 0, 255};
    uint8_t parameter[2] = {ND_AXDR_ENUM, 3};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_ic_setup_set(&objects.setup, 16, &port, 3, &in));

    nd_idis_setup_t restored;
    nd_idis_setup_init(&restored);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_setup_load(
                                 &restored, nd_byte_store_port(&setup_store), 0));
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_ic_setup_get(&restored, 16, &port, 3, NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
}

static void test_association_and_security_objects(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ASSOCIATION_LN,
                                            &ND_OBIS_ASSOCIATION, 8, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[0]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_SECURITY_SETUP,
                                            &ND_OBIS_SECURITY_SETUP, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[0]);

    uint8_t parameter[2] = {ND_AXDR_ENUM, 1};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SECURITY_SETUP,
                                               &ND_OBIS_SECURITY_SETUP, 1,
                                               &in, NULL));
    TEST_ASSERT_EQUAL_UINT8(1, objects.association.security.activated);
}

static void test_firmware_and_frame_counter_objects(void)
{
    memcpy(objects.firmware_id[0], "FW-1.0.0", 8);
    objects.firmware_id_len[0] = 8;
    objects.security_fc_rx[0] = 7;
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_FIRMWARE_IDS[0], 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(8, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_SECURITY_FC_UNICAST, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(7, ((uint32_t)buffer[1] << 24)
                                   | ((uint32_t)buffer[2] << 16)
                                   | ((uint32_t)buffer[3] << 8) | buffer[4]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_setup_scalar_round_trip);
    RUN_TEST(test_setup_persists);
    RUN_TEST(test_association_and_security_objects);
    RUN_TEST(test_firmware_and_frame_counter_objects);
    return UNITY_END();
}
