#include "unity.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

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
    (void)public_key;
    (void)public_len;
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
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hls_reply_uses_crypto_port);
    RUN_TEST(test_key_pair_and_certificate_request);
    return UNITY_END();
}
