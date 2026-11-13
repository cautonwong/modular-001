#include "unity.h"

#include <string.h>

#include "components/protocol/dlms/include/nd_dlms_apdu.h"

void setUp(void) {}
void tearDown(void) {}

static nd_dlms_aarq_t request(uint8_t auth)
{
    nd_dlms_aarq_t aarq = {
        .client_sap = 16,
        .auth = auth,
        .security_policy = 0,
        .security_suite = 0,
        .max_rx_pdu = 1200,
        .max_tx_pdu = 1200,
        .auth_value_len = 0,
    };
    return aarq;
}

static void test_aarq_no_security_matches_green_book(void)
{
    static const uint8_t expected[] = {
        0x60, 0x1D, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74,
        0x05, 0x08, 0x01, 0x01, 0xBE, 0x10, 0x04, 0x0E, 0x01,
        0x00, 0x00, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00,
        0x7E, 0x1F, 0x04, 0xB0,
    };
    nd_dlms_aarq_t aarq = request(ND_DLMS_AUTH_NONE);
    uint8_t buffer[64];
    size_t length = 0;

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aarq(&aarq, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_aarq_lls_matches_green_book(void)
{
    static const uint8_t expected[] = {
        0x60, 0x36, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74,
        0x05, 0x08, 0x01, 0x01, 0x8A, 0x02, 0x07, 0x80, 0x8B,
        0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x02, 0x01, 0xAC,
        0x0A, 0x80, 0x08, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0xBE, 0x10, 0x04, 0x0E, 0x01, 0x00, 0x00,
        0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00, 0x7E, 0x1F,
        0x04, 0xB0,
    };
    nd_dlms_aarq_t aarq = request(ND_DLMS_AUTH_LLS);
    uint8_t buffer[64];
    size_t length = 0;

    memcpy(aarq.auth_value, "12345678", 8);
    aarq.auth_value_len = 8;
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aarq(&aarq, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_aarq_hls_matches_green_book(void)
{
    static const uint8_t expected[] = {
        0x60, 0x36, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74,
        0x05, 0x08, 0x01, 0x01, 0x8A, 0x02, 0x07, 0x80, 0x8B,
        0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x02, 0x05, 0xAC,
        0x0A, 0x80, 0x08, 0x4B, 0x35, 0x36, 0x69, 0x56, 0x61,
        0x67, 0x59, 0xBE, 0x10, 0x04, 0x0E, 0x01, 0x00, 0x00,
        0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00, 0x7E, 0x1F,
        0x04, 0xB0,
    };
    nd_dlms_aarq_t aarq = request(ND_DLMS_AUTH_HLS_GMAC);
    uint8_t buffer[64];
    size_t length = 0;

    memcpy(aarq.auth_value, "K56iVagY", 8);
    aarq.auth_value_len = 8;
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aarq(&aarq, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_aarq_decode_roundtrip(void)
{
    nd_dlms_aarq_t encoded = request(ND_DLMS_AUTH_HLS_GMAC);
    nd_dlms_aarq_t decoded;
    uint8_t buffer[64];
    size_t length = 0;

    memcpy(encoded.auth_value, "K56iVagY", 8);
    encoded.auth_value_len = 8;
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aarq(&encoded, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_decode_aarq(buffer, length, &decoded));
    TEST_ASSERT_EQUAL_UINT8(ND_DLMS_AUTH_HLS_GMAC, decoded.auth);
    TEST_ASSERT_EQUAL_UINT8(8, decoded.auth_value_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(encoded.auth_value, decoded.auth_value, 8);
    TEST_ASSERT_EQUAL_UINT16(1200, decoded.max_rx_pdu);
}

static void test_aarq_decode_rejects_short_or_bad_frame(void)
{
    uint8_t short_frame[] = {0x60, 0x1D, 0xA1};
    uint8_t bad_tag[] = {0x61, 0x00};
    nd_dlms_aarq_t decoded;

    TEST_ASSERT_EQUAL_INT(-1, nd_dlms_apdu_decode_aarq(short_frame,
                                                       sizeof(short_frame),
                                                       &decoded));
    TEST_ASSERT_EQUAL_INT(-1, nd_dlms_apdu_decode_aarq(bad_tag,
                                                       sizeof(bad_tag),
                                                       &decoded));
}


static void test_aare_success_matches_green_book(void)
{
    static const uint8_t expected[] = {
        0x61, 0x29, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74,
        0x05, 0x08, 0x01, 0x01, 0xA2, 0x03, 0x02, 0x01, 0x00,
        0xA3, 0x05, 0xA1, 0x03, 0x02, 0x01, 0x00, 0xBE, 0x10,
        0x04, 0x0E, 0x08, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00,
        0x00, 0x50, 0x1F, 0x01, 0xF4, 0x00, 0x07,
    };
    nd_dlms_aare_t aare = {.result = 0, .diagnostic = 0,
                           .auth = ND_DLMS_AUTH_NONE};
    uint8_t buffer[96];
    size_t length = 0;

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aare(&aare, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_aare_hls_matches_green_book(void)
{
    static const uint8_t expected[] = {
        0x61, 0x42, 0xA1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74,
        0x05, 0x08, 0x01, 0x01, 0xA2, 0x03, 0x02, 0x01, 0x00,
        0xA3, 0x05, 0xA1, 0x03, 0x02, 0x01, 0x0E, 0x88, 0x02,
        0x07, 0x80, 0x89, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08,
        0x02, 0x05, 0xAA, 0x0A, 0x80, 0x08, 0x50, 0x36, 0x77,
        0x52, 0x4A, 0x32, 0x31, 0x46, 0xBE, 0x10, 0x04, 0x0E,
        0x08, 0x00, 0x06, 0x5F, 0x1F, 0x04, 0x00, 0x00, 0x50,
        0x1F, 0x01, 0xF4, 0x00, 0x07,
    };
    nd_dlms_aare_t aare = {.result = 0, .diagnostic = 0x0E,
                           .auth = ND_DLMS_AUTH_HLS_GMAC,
                           .auth_value_len = 8};
    uint8_t buffer[96];
    size_t length = 0;

    memcpy(aare.auth_value, "P6wRJ21F", 8);
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aare(&aare, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(expected));
}

static void test_aare_decode_roundtrip(void)
{
    nd_dlms_aare_t encoded = {.result = 0, .diagnostic = 0x0E,
                              .auth = ND_DLMS_AUTH_HLS_GMAC,
                              .auth_value_len = 8};
    nd_dlms_aare_t decoded;
    uint8_t buffer[96];
    size_t length = 0;

    memcpy(encoded.auth_value, "P6wRJ21F", 8);
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_encode_aare(&encoded, buffer,
                                                      sizeof(buffer), &length));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_apdu_decode_aare(buffer, length, &decoded));
    TEST_ASSERT_EQUAL_UINT8(0, decoded.result);
    TEST_ASSERT_EQUAL_UINT8(0x0E, decoded.diagnostic);
    TEST_ASSERT_EQUAL_UINT8(ND_DLMS_AUTH_HLS_GMAC, decoded.auth);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(encoded.auth_value, decoded.auth_value, 8);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aarq_no_security_matches_green_book);
    RUN_TEST(test_aarq_lls_matches_green_book);
    RUN_TEST(test_aarq_hls_matches_green_book);
    RUN_TEST(test_aarq_decode_roundtrip);
    RUN_TEST(test_aarq_decode_rejects_short_or_bad_frame);
    RUN_TEST(test_aare_success_matches_green_book);
    RUN_TEST(test_aare_hls_matches_green_book);
    RUN_TEST(test_aare_decode_roundtrip);
    return UNITY_END();
}
