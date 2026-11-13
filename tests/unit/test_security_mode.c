#include "unity.h"

#include "domain/security/security_mode.h"

void setUp(void) {}
void tearDown(void) {}

static void test_mode_lookup_uses_exact_and_wildcard_tables(void)
{
    TEST_ASSERT_EQUAL_UINT16(0x4180u, nd_security_mode_lookup(0x2015u));
    TEST_ASSERT_EQUAL_UINT16(0x8184u, nd_security_mode_lookup(0x4000u));
    TEST_ASSERT_EQUAL_UINT16(0x4110u, nd_security_mode_lookup(0x5012u));
    TEST_ASSERT_EQUAL_UINT16(0x8004u, nd_security_mode_lookup(0x0123u));
    TEST_ASSERT_EQUAL_UINT16(0x4110u, nd_security_mode_lookup(0x9999u));
}

static void test_plain_read_allowlist(void)
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

static void test_cipher_permissions_use_the_verification_choice(void)
{
    uint16_t mode = 0x0100u;

    TEST_ASSERT_EQUAL_INT(ND_SECURITY_ALLOW,
                          nd_security_mode_check(mode, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_CIPHER,
                                                 ND_SECURITY_AUTH_SID_MAC));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_ALLOW,
                          nd_security_mode_check(0x0000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_CIPHER,
                                                 ND_SECURITY_AUTH_SID));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_DENY_AUTH,
                          nd_security_mode_check(0x1000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_CIPHER,
                                                 ND_SECURITY_AUTH_SID));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_DENY_AUTH,
                          nd_security_mode_check(0x3000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_CIPHER,
                                                 ND_SECURITY_AUTH_RN_MAC));
}

static void test_plain_get_requires_plain_read_permission(void)
{
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_DENY_AUTH,
                          nd_security_mode_check(0x4110u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_NONE,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_NEED_PLAIN_CHECK,
                          nd_security_mode_check(0xC000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_NONE,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_ALLOW,
                          nd_security_mode_check(0x0000u, ND_SECURITY_CMD_SET,
                                                 ND_SECURITY_CHOICE_NONE,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_DENY_AUTH,
                          nd_security_mode_check(0x0100u, ND_SECURITY_CMD_SET,
                                                 ND_SECURITY_CHOICE_NONE,
                                                 ND_SECURITY_AUTH_RN));
}

static void test_rn_get_and_set_permissions(void)
{
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_ALLOW,
                          nd_security_mode_check(0xC000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_PLAIN,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_ALLOW,
                          nd_security_mode_check(0x4000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_PLAIN,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_DENY_AUTH,
                          nd_security_mode_check(0x2000u, ND_SECURITY_CMD_GET,
                                                 ND_SECURITY_CHOICE_PLAIN,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_ALLOW,
                          nd_security_mode_check(0x0000u, ND_SECURITY_CMD_ACTION,
                                                 ND_SECURITY_CHOICE_PLAIN,
                                                 ND_SECURITY_AUTH_RN));
    TEST_ASSERT_EQUAL_INT(ND_SECURITY_DENY_AUTH,
                          nd_security_mode_check(0x0070u, ND_SECURITY_CMD_ACTION,
                                                 ND_SECURITY_CHOICE_PLAIN,
                                                 ND_SECURITY_AUTH_RN));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mode_lookup_uses_exact_and_wildcard_tables);
    RUN_TEST(test_plain_read_allowlist);
    RUN_TEST(test_cipher_permissions_use_the_verification_choice);
    RUN_TEST(test_plain_get_requires_plain_read_permission);
    RUN_TEST(test_rn_get_and_set_permissions);
    return UNITY_END();
}
