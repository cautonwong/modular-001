#include "unity.h"

#include "application/adapters/dl698/security_mode_map.h"

void setUp(void) {}
void tearDown(void) {}

static nd_security_request_t broadcast_request(uint8_t sa_type,
                                               uint8_t security_choice,
                                               uint8_t verification_choice,
                                               uint8_t connected)
{
    nd_security_request_t request = {0};

    request.sa_type = sa_type;
    request.security_choice = security_choice;
    request.verification_choice = verification_choice;
    request.connected = connected;
    return request;
}

static void test_lookup_uses_exact_and_wildcard_tables(void)
{
    TEST_ASSERT_EQUAL_UINT16(0x4180u, nd_security_mode_lookup(0x2015u));
    TEST_ASSERT_EQUAL_UINT16(0x8184u, nd_security_mode_lookup(0x4000u));
    TEST_ASSERT_EQUAL_UINT16(0x4110u, nd_security_mode_lookup(0x5012u));
    TEST_ASSERT_EQUAL_UINT16(0x8004u, nd_security_mode_lookup(0x0123u));
    TEST_ASSERT_EQUAL_UINT16(0x4110u, nd_security_mode_lookup(0x9999u));
}

static void test_plain_read_and_infrared_allowlists(void)
{
    TEST_ASSERT_TRUE(nd_security_plain_read_allowed(0x00010200u));
    TEST_ASSERT_TRUE(nd_security_plain_read_allowed(0xF1000B00u));
    TEST_ASSERT_TRUE(nd_security_plain_read_allowed(0x202C0200u));
    TEST_ASSERT_TRUE(nd_security_plain_read_allowed(0x40000200u));
    TEST_ASSERT_TRUE(nd_security_plain_read_allowed(0x41110201u));
    TEST_ASSERT_FALSE(nd_security_plain_read_allowed(0x99990200u));
    TEST_ASSERT_TRUE(nd_security_infrared_oad_allowed(0x202C0200u));
    TEST_ASSERT_FALSE(nd_security_infrared_oad_allowed(0xF1000B00u));
}

static void test_wire_choices_map_to_semantic_values(void)
{
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_CHOICE_PLAIN,
                            nd_security_choice_from_wire(0u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_CHOICE_CIPHER,
                            nd_security_choice_from_wire(1u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_CHOICE_NONE,
                            nd_security_choice_from_wire(0xFFu));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_AUTH_SID_MAC,
                            nd_security_verification_from_wire(0u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_AUTH_RN,
                            nd_security_verification_from_wire(1u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_AUTH_RN_MAC,
                            nd_security_verification_from_wire(2u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_AUTH_SID,
                            nd_security_verification_from_wire(3u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_SA_WILDCARD,
                            nd_security_sa_from_wire(1u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_SA_GROUP,
                            nd_security_sa_from_wire(2u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_SA_BROADCAST,
                            nd_security_sa_from_wire(3u));
    TEST_ASSERT_EQUAL_UINT8(ND_SECURITY_SA_UNICAST,
                            nd_security_sa_from_wire(0u));
}

static void test_broadcast_rule_accepts_a_connected_cipher_sidmac_unicast(void)
{
    nd_security_request_t request = broadcast_request(ND_SECURITY_SA_UNICAST,
                                                      ND_SECURITY_CHOICE_CIPHER,
                                                      ND_SECURITY_AUTH_SID_MAC,
                                                      1);

    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_broadcast_time_rule(0x40007F00u, &request));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_broadcast_time_rule(0x40000200u, &request));
}

static void test_broadcast_rule_rejects_a_disconnected_unicast(void)
{
    nd_security_request_t request = broadcast_request(ND_SECURITY_SA_UNICAST,
                                                      ND_SECURITY_CHOICE_CIPHER,
                                                      ND_SECURITY_AUTH_SID_MAC,
                                                      0);

    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_REJECTED,
                          nd_security_broadcast_time_rule(0x40007F00u, &request));
}

static void test_broadcast_rule_is_silent_or_rejected_by_address(void)
{
    nd_security_request_t plain = broadcast_request(ND_SECURITY_SA_BROADCAST,
                                                    ND_SECURITY_CHOICE_PLAIN,
                                                    ND_SECURITY_AUTH_SID_MAC,
                                                    1);
    nd_security_request_t cipher = broadcast_request(ND_SECURITY_SA_BROADCAST,
                                                     ND_SECURITY_CHOICE_CIPHER,
                                                     ND_SECURITY_AUTH_SID_MAC,
                                                     1);
    nd_security_request_t wildcard = broadcast_request(ND_SECURITY_SA_WILDCARD,
                                                       ND_SECURITY_CHOICE_CIPHER,
                                                       ND_SECURITY_AUTH_SID_MAC,
                                                       1);
    nd_security_request_t group = broadcast_request(ND_SECURITY_SA_GROUP,
                                                    ND_SECURITY_CHOICE_CIPHER,
                                                    ND_SECURITY_AUTH_SID_MAC,
                                                    1);

    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_SILENT,
                          nd_security_broadcast_time_rule(0x40007F00u, &plain));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_ALLOW,
                          nd_security_broadcast_time_rule(0x40007F00u, &cipher));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_REJECTED,
                          nd_security_broadcast_time_rule(0x40007F00u, &wildcard));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_REQUEST_SILENT,
                          nd_security_broadcast_time_rule(0x40007F00u, &group));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lookup_uses_exact_and_wildcard_tables);
    RUN_TEST(test_plain_read_and_infrared_allowlists);
    RUN_TEST(test_wire_choices_map_to_semantic_values);
    RUN_TEST(test_broadcast_rule_accepts_a_connected_cipher_sidmac_unicast);
    RUN_TEST(test_broadcast_rule_rejects_a_disconnected_unicast);
    RUN_TEST(test_broadcast_rule_is_silent_or_rejected_by_address);
    return UNITY_END();
}
