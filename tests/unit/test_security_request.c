#include "unity.h"

#include "domain/security/security_request.h"
#include "domain/security/security_mode.h"

void setUp(void) {}
void tearDown(void) {}

static nd_security_request_t request(void)
{
    nd_security_request_t value = {0};
    value.security_choice = ND_SECURITY_CHOICE_PLAIN;
    value.verification_choice = ND_SECURITY_AUTH_RN;
    value.sa_type = 0;
    return value;
}

static void test_report_response_skips_application_link_check(void)
{
    nd_security_request_t value = request();

    value.report_response = 1;
    value.security_choice = ND_SECURITY_CHOICE_CIPHER;
    value.verification_choice = ND_SECURITY_AUTH_SID_MAC;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));
}

static void test_broadcast_and_group_skip_link_check(void)
{
    nd_security_request_t value = request();

    value.sa_type = 3;
    value.security_choice = ND_SECURITY_CHOICE_CIPHER;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));

    value.sa_type = 2;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));
}

static void test_cipher_requests_need_a_link(void)
{
    nd_security_request_t value = request();

    value.security_choice = ND_SECURITY_CHOICE_CIPHER;
    value.verification_choice = ND_SECURITY_AUTH_SID_MAC;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_DENIED,
                          nd_security_request_check(&value));

    value.connected = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));
}

static void test_plain_master_auth_and_ir_rules(void)
{
    nd_security_request_t value = request();

    value.auth_on = 1;
    value.client_index = 0;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_DENIED,
                          nd_security_request_check(&value));

    value.channel_is_ir = 1;
    value.infrared_auth_timeout = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));

    value.infrared_auth_timeout = 0;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_DENIED,
                          nd_security_request_check(&value));
}

static void test_plain_terminal_auth_timeout_bypasses_link(void)
{
    nd_security_request_t value = request();

    value.auth_on = 1;
    value.client_index = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_DENIED,
                          nd_security_request_check(&value));

    value.term_auth_timeout = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));
}

static void test_plain_rn_get_with_plain_read_and_any_link_bypasses_auth(void)
{
    nd_security_request_t value = request();

    value.auth_on = 1;
    value.get_request = 1;
    value.plain_read_allowed = 1;
    value.any_connected = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));
}

static void test_broadcast_time_oad_rules(void)
{
    nd_security_request_t value = request();

    value.oad = 0x40007F00u;
    value.sa_type = 0;
    value.security_choice = ND_SECURITY_CHOICE_CIPHER;
    value.verification_choice = ND_SECURITY_AUTH_SID_MAC;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_DENIED,
                          nd_security_request_check(&value));

    value.connected = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_request_check(&value));

    value.sa_type = 3;
    value.connected = 0;
    value.verification_choice = ND_SECURITY_AUTH_RN;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_SILENT,
                          nd_security_request_check(&value));

    value.sa_type = 1;
    value.connected = 1;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_REJECTED,
                          nd_security_request_check(&value));

    value.sa_type = 2;
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_SILENT,
                          nd_security_request_check(&value));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_report_response_skips_application_link_check);
    RUN_TEST(test_broadcast_and_group_skip_link_check);
    RUN_TEST(test_cipher_requests_need_a_link);
    RUN_TEST(test_plain_master_auth_and_ir_rules);
    RUN_TEST(test_plain_terminal_auth_timeout_bypasses_link);
    RUN_TEST(test_plain_rn_get_with_plain_read_and_any_link_bypasses_auth);
    RUN_TEST(test_broadcast_time_oad_rules);
    return UNITY_END();
}
