#include "unity.h"

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_idis_image_init(&objects.image);
}

void tearDown(void) {}

static int initiate(void)
{
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 4);
    nd_axdr_put_octets(&out, (const uint8_t *)"IMG1", 4);
    nd_axdr_put_tag(&out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&out, 512);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    return nd_cosem_port_action(&objects, 16, ND_IC_IMAGE_TRANSFER,
                                &ND_OBIS_IMAGE_TRANSFER, 1, &in, NULL);
}

static int transfer(uint32_t block)
{
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&out, block);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 4);
    nd_axdr_put_octets(&out, (const uint8_t *)"DATA", 4);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    return nd_cosem_port_action(&objects, 16, ND_IC_IMAGE_TRANSFER,
                                &ND_OBIS_IMAGE_TRANSFER, 2, &in, NULL);
}

static int verify_or_activate(uint8_t method)
{
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    return nd_cosem_port_action(&objects, 16, ND_IC_IMAGE_TRANSFER,
                                &ND_OBIS_IMAGE_TRANSFER, method, &in, NULL);
}

static void test_transfer_state_machine(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK, initiate());
    TEST_ASSERT_EQUAL_UINT8(2, objects.image.block_count);
    TEST_ASSERT_EQUAL_UINT32(1, objects.image.first_missing);
    TEST_ASSERT_EQUAL_INT(ND_OK, transfer(1));
    TEST_ASSERT_EQUAL_UINT32(2, objects.image.first_missing);
    TEST_ASSERT_EQUAL_INT(ND_OK, verify_or_activate(3));
    TEST_ASSERT_EQUAL_UINT8(4, objects.image.status);
    TEST_ASSERT_EQUAL_INT(ND_OK, transfer(2));
    TEST_ASSERT_EQUAL_UINT32(3, objects.image.first_missing);
    TEST_ASSERT_EQUAL_INT(ND_OK, verify_or_activate(3));
    TEST_ASSERT_EQUAL_UINT8(3, objects.image.status);
    TEST_ASSERT_EQUAL_INT(ND_OK, verify_or_activate(4));
    TEST_ASSERT_EQUAL_UINT8(6, objects.image.status);
}

static void test_image_attributes(void)
{
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_IMAGE_TRANSFER,
                                            &ND_OBIS_IMAGE_TRANSFER, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT32(256, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    TEST_ASSERT_EQUAL_INT(ND_OK, initiate());
    TEST_ASSERT_EQUAL_INT(ND_OK, transfer(1));
    TEST_ASSERT_EQUAL_INT(ND_OK, transfer(2));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_IMAGE_TRANSFER,
                                            &ND_OBIS_IMAGE_TRANSFER, 3, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_BIT_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]); /* two blocks */
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[2]);
}

static void test_image_persists(void)
{
    static uint8_t memory[256];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    TEST_ASSERT_EQUAL_INT(ND_OK, initiate());
    TEST_ASSERT_EQUAL_INT(0, nd_idis_image_save(&objects.image,
                                                nd_byte_store_port(&store), 0));
    nd_image_state_t restored;
    nd_idis_image_init(&restored);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_image_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(1, restored.status);
    TEST_ASSERT_EQUAL_UINT32(512, restored.image_size);
    TEST_ASSERT_EQUAL_UINT32(256, restored.block_size);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_transfer_state_machine);
    RUN_TEST(test_image_attributes);
    RUN_TEST(test_image_persists);
    return UNITY_END();
}
