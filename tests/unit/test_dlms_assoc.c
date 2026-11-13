#include "unity.h"

#include "components/protocol/dlms/include/nd_dlms_assoc.h"
#include "components/protocol/dlms/include/nd_dlms_security.h"

void setUp(void) {}
void tearDown(void) {}

static nd_dlms_aarq_t aarq(uint8_t auth)
{
    return (nd_dlms_aarq_t){
        .client_sap = 16,
        .auth = auth,
        .security_policy = 0,
        .security_suite = 0,
        .max_rx_pdu = 1224,
        .max_tx_pdu = 1224,
        .client_system_title = {0},
    };
}

static void test_none_auth_associates_directly(void)
{
    nd_dlms_security_ctx_t security = {0};
    nd_dlms_assoc_t assoc;
    nd_dlms_aarq_t request = aarq(ND_DLMS_AUTH_NONE);

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_init(&assoc, &security));
    TEST_ASSERT_EQUAL_INT(ND_DLMS_ASSOC_IDLE, assoc.state);
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_begin(&assoc, &request));
    TEST_ASSERT_EQUAL_INT(ND_DLMS_ASSOC_ASSOCIATED, assoc.state);
    TEST_ASSERT_TRUE(nd_dlms_assoc_can_access(&assoc, 16));
    TEST_ASSERT_FALSE(nd_dlms_assoc_can_access(&assoc, 1));
}

static void test_hls_waits_for_verification(void)
{
    nd_dlms_security_ctx_t security = {0};
    nd_dlms_assoc_t assoc;
    nd_dlms_aarq_t request = aarq(ND_DLMS_AUTH_HLS_GMAC);

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_init(&assoc, &security));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_begin(&assoc, &request));
    TEST_ASSERT_EQUAL_INT(ND_DLMS_ASSOC_HLS_CHALLENGE, assoc.state);
    TEST_ASSERT_FALSE(nd_dlms_assoc_can_access(&assoc, 16));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_hls_verify(&assoc, 1));
    TEST_ASSERT_EQUAL_INT(ND_DLMS_ASSOC_ASSOCIATED, assoc.state);
    TEST_ASSERT_TRUE(nd_dlms_assoc_can_access(&assoc, 16));
}

static void test_hls_rejection_returns_to_idle(void)
{
    nd_dlms_security_ctx_t security = {0};
    nd_dlms_assoc_t assoc;
    nd_dlms_aarq_t request = aarq(ND_DLMS_AUTH_HLS_SHA256);

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_init(&assoc, &security));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_begin(&assoc, &request));
    TEST_ASSERT_EQUAL_INT(-1, nd_dlms_assoc_hls_verify(&assoc, 0));
    TEST_ASSERT_EQUAL_INT(ND_DLMS_ASSOC_IDLE, assoc.state);
}

static void test_release_and_invalid_request(void)
{
    nd_dlms_security_ctx_t security = {0};
    nd_dlms_assoc_t assoc;
    nd_dlms_aarq_t request = aarq(ND_DLMS_AUTH_NONE);

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_init(&assoc, &security));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_begin(&assoc, &request));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_assoc_release(&assoc));
    TEST_ASSERT_EQUAL_INT(ND_DLMS_ASSOC_RELEASED, assoc.state);
    TEST_ASSERT_FALSE(nd_dlms_assoc_can_access(&assoc, 16));

    request.auth = 99;
    TEST_ASSERT_EQUAL_INT(-1, nd_dlms_assoc_begin(&assoc, &request));
}

static void test_frame_counter_rejects_replay(void)
{
    nd_dlms_security_ctx_t security = {0};

    TEST_ASSERT_EQUAL_INT(0, nd_dlms_security_accept_fc(&security, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_dlms_security_accept_fc(&security, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_security_accept_fc(&security, 1, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_security_accept_fc(&security, 5, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_dlms_security_accept_fc(&security, 4, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_dlms_security_accept_fc(&security, 6, 0));
}

static void test_tx_counter_increments(void)
{
    nd_dlms_security_ctx_t security = {0};

    TEST_ASSERT_EQUAL_UINT32(0, nd_dlms_security_next_tx(&security));
    TEST_ASSERT_EQUAL_UINT32(1, nd_dlms_security_next_tx(&security));
    TEST_ASSERT_EQUAL_UINT32(2, security.fc_tx);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_none_auth_associates_directly);
    RUN_TEST(test_hls_waits_for_verification);
    RUN_TEST(test_hls_rejection_returns_to_idle);
    RUN_TEST(test_release_and_invalid_request);
    RUN_TEST(test_frame_counter_rejects_replay);
    RUN_TEST(test_tx_counter_increments);
    return UNITY_END();
}
