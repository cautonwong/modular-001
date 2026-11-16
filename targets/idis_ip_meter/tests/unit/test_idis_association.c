#include "unity.h"

#include <string.h>

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_clock.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_dlms_datetime_t clock_now;
static uint8_t certificate_public_key[64];
static size_t certificate_public_len;

static int fake_clock_read(void *ctx, nd_dlms_datetime_t *out)
{
    (void)ctx;
    *out = clock_now;
    return 0;
}

static int fake_clock_write(void *ctx, const nd_dlms_datetime_t *value)
{
    (void)ctx;
    clock_now = *value;
    return 0;
}

static int fake_sha256(void *ctx, const uint8_t *in, size_t len,
                       uint8_t out[32])
{
    (void)ctx;
    (void)in;
    (void)len;
    for (int index = 0; index < 32; index++) {
        out[index] = 0x5A;
    }
    return 0;
}

static int fake_key_pair(void *ctx, uint8_t *public_key, size_t *public_len)
{
    (void)ctx;
    for (int index = 0; index < 64; index++) {
        public_key[index] = 0x11;
    }
    *public_len = 64;
    return 0;
}

static int fake_certificate_request(void *ctx, const uint8_t *public_key,
                                    size_t public_len, uint8_t *request,
                                    size_t *request_len)
{
    (void)ctx;
    certificate_public_len = public_len;
    memcpy(certificate_public_key, public_key,
           public_len < sizeof(certificate_public_key)
               ? public_len
               : sizeof(certificate_public_key));
    for (int index = 0; index < 32; index++) {
        request[index] = 0x22;
    }
    *request_len = 32;
    return 0;
}

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    nd_idis_association_init(&objects.association);
    nd_clock_port_t port = {.read = fake_clock_read, .write = fake_clock_write};
    nd_idis_clock_init(&objects.clock, &port);
}

void tearDown(void) {}

static void test_hls_reply_uses_crypto_port(void)
{
    nd_crypto_port_t crypto = {.sha256 = fake_sha256};
    nd_idis_association_bind_crypto(&objects.association, &crypto);
    uint8_t challenge[] = {ND_AXDR_OCTET_STRING, 4, 1, 2, 3, 4};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, challenge, sizeof(challenge));
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_ASSOCIATION_LN,
                                               &ND_OBIS_ASSOCIATION, 1, &in,
                                               &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(32, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x5A, buffer[2]);
}

static void test_key_pair_and_certificate_request(void)
{
    nd_crypto_port_t crypto = {
        .key_pair_generate = fake_key_pair,
        .certificate_request = fake_certificate_request,
    };
    nd_idis_association_bind_crypto(&objects.association, &crypto);
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SECURITY_SETUP,
                                               &ND_OBIS_SECURITY_SETUP, 4, &in,
                                               NULL));
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_SECURITY_SETUP,
                                               &ND_OBIS_SECURITY_SETUP, 5, &in,
                                               &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_OCTET_STRING, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(32, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x22, buffer[2]);
    TEST_ASSERT_EQUAL_size_t(64, certificate_public_len);
    TEST_ASSERT_EQUAL_UINT8(0x11, certificate_public_key[0]);
}

static void test_xdlms_context_info_shape(void)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ASSOCIATION_LN,
                                            &ND_OBIS_ASSOCIATION, 5, NULL,
                                            &out));
    const uint8_t expected[] = {
        ND_AXDR_STRUCTURE, 6,
        ND_AXDR_BIT_STRING, 24, 0, 0, 0,
        ND_AXDR_LONG_UNSIGNED, 0, 0x7F,
        ND_AXDR_LONG_UNSIGNED, 0, 0x7F,
        ND_AXDR_UNSIGNED, 6,
        ND_AXDR_INTEGER, 0xFF,
        ND_AXDR_OCTET_STRING, 0,
    };
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), out.len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_object_list_uses_standard_element(void)
{
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_ASSOCIATION_LN,
                                            &ND_OBIS_ASSOCIATION, 2, NULL,
                                            &out));
    const uint8_t expected[] = {
        ND_AXDR_ARRAY, 5,
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_LONG_UNSIGNED, 0, 8,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 1, 0, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 1,
        ND_AXDR_STRUCTURE, 3,
        ND_AXDR_INTEGER, 2,
        ND_AXDR_ENUM, 3,
        ND_AXDR_NULL,
        ND_AXDR_ARRAY, 0,
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_LONG_UNSIGNED, 0, 3,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 1, 0, 1, 8, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 1,
        ND_AXDR_STRUCTURE, 3,
        ND_AXDR_INTEGER, 2,
        ND_AXDR_ENUM, 1,
        ND_AXDR_NULL,
        ND_AXDR_ARRAY, 0,
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_LONG_UNSIGNED, 0, 15,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 40, 0, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 1,
        ND_AXDR_STRUCTURE, 3,
        ND_AXDR_INTEGER, 1,
        ND_AXDR_ENUM, 1,
        ND_AXDR_NULL,
        ND_AXDR_ARRAY, 0,
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_LONG_UNSIGNED, 0, 64,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 43, 0, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 1,
        ND_AXDR_STRUCTURE, 3,
        ND_AXDR_INTEGER, 1,
        ND_AXDR_ENUM, 1,
        ND_AXDR_NULL,
        ND_AXDR_ARRAY, 0,
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_LONG_UNSIGNED, 0, 17,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 41, 0, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 1,
        ND_AXDR_STRUCTURE, 3,
        ND_AXDR_INTEGER, 1,
        ND_AXDR_ENUM, 1,
        ND_AXDR_NULL,
        ND_AXDR_ARRAY, 0,
    };
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), out.len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_object_list_entries_exist_in_dispatch(void)
{
    const nd_assoc_state_t *state = &objects.association.association;
    for (uint16_t index = 0; index < state->base.count; index++) {
        const nd_idis_registry_entry_t *entry = &state->base.entries[index];
        uint8_t buffer[64];
        nd_axdr_writer_t out;
        nd_axdr_writer_init(&out, buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_cosem_port_get(&objects, 16,
                                                entry->class_id, &entry->ln, 1,
                                                NULL, &out));
    }
    for (uint8_t index = 0; index < state->extra_count; index++) {
        const nd_assoc_object_t *entry = &state->extra[index];
        uint8_t buffer[64];
        nd_axdr_writer_t out;
        nd_axdr_writer_init(&out, buffer, sizeof(buffer));
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_cosem_port_get(&objects, 16,
                                                entry->class_id, &entry->ln, 1,
                                                NULL, &out));
    }
}

static void test_security_policy_rejects_integer(void)
{
    uint8_t input[] = {ND_AXDR_INTEGER, 1};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, input, sizeof(input));
    TEST_ASSERT_EQUAL_INT(ND_ERR_TYPE_MISMATCH,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_SECURITY_SETUP,
                                            &ND_OBIS_SECURITY_SETUP, 2, &in));
}

static void test_sap_assignment_rejects_unsigned(void)
{
    uint8_t input[] = {
        ND_AXDR_ARRAY, 1,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_UNSIGNED, 16,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 40, 0, 0, 255,
    };
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, input, sizeof(input));
    TEST_ASSERT_EQUAL_INT(ND_ERR_TYPE_MISMATCH,
                          nd_cosem_port_set(&objects, 16,
                                            ND_IC_SAP_ASSIGNMENT,
                                            &ND_OBIS_SAP_ASSIGNMENT, 2, &in));
}

static void test_add_object_rejects_integer_class_id(void)
{
    uint8_t input[] = {
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_INTEGER, 3,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 40, 0, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 0,
        ND_AXDR_ARRAY, 0,
    };
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, input, sizeof(input));
    TEST_ASSERT_EQUAL_INT(ND_ERR_TYPE_MISMATCH,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_ASSOCIATION_LN,
                                               &ND_OBIS_ASSOCIATION, 3, &in,
                                               NULL));
}

static void test_add_and_remove_object_round_trip(void)
{
    uint8_t element[] = {
        ND_AXDR_STRUCTURE, 4,
        ND_AXDR_LONG_UNSIGNED, 0, 3,
        ND_AXDR_UNSIGNED, 0,
        ND_AXDR_OCTET_STRING, 6, 0, 0, 40, 0, 0, 255,
        ND_AXDR_STRUCTURE, 2,
        ND_AXDR_ARRAY, 0,
        ND_AXDR_ARRAY, 0,
    };
    uint16_t before = (uint16_t)(objects.association.association.base.count
                                 + objects.association.association.extra_count);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, element, sizeof(element));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_ASSOCIATION_LN,
                                               &ND_OBIS_ASSOCIATION, 3, &in,
                                               NULL));
    TEST_ASSERT_EQUAL_UINT16(before + 1,
                             objects.association.association.base.count
                                 + objects.association.association.extra_count);
    nd_axdr_reader_init(&in, element, sizeof(element));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_ASSOCIATION_LN,
                                               &ND_OBIS_ASSOCIATION, 4, &in,
                                               NULL));
    TEST_ASSERT_EQUAL_UINT16(before,
                             objects.association.association.base.count
                                 + objects.association.association.extra_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hls_reply_uses_crypto_port);
    RUN_TEST(test_key_pair_and_certificate_request);
    RUN_TEST(test_xdlms_context_info_shape);
    RUN_TEST(test_object_list_uses_standard_element);
    RUN_TEST(test_object_list_entries_exist_in_dispatch);
    RUN_TEST(test_security_policy_rejects_integer);
    RUN_TEST(test_sap_assignment_rejects_unsigned);
    RUN_TEST(test_add_object_rejects_integer_class_id);
    RUN_TEST(test_add_and_remove_object_round_trip);
    return UNITY_END();
}
