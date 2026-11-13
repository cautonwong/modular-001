#include "unity.h"

#include "domain/security/esam_result.h"

void setUp(void) {}
void tearDown(void) {}

static void test_esam_status_must_be_ok(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_OK,
                          nd_esam_result(0x9000, 0, 1, 0));
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_FAIL,
                          nd_esam_result(0x6A80, 0, 1, 0));
}

static void test_type_zero_ignores_mac(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_OK,
                          nd_esam_result(0x9000, 0, 0, 1));
}

static void test_type_one_can_enforce_or_skip_mac(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_OK,
                          nd_esam_result(0x9000, 1, 1, 1));
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_FAIL,
                          nd_esam_result(0x9000, 1, 0, 1));
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_OK,
                          nd_esam_result(0x9000, 1, 0, 0));
}

static void test_unknown_verification_type_fails(void)
{
    TEST_ASSERT_EQUAL_INT(ND_ESAM_RESULT_FAIL,
                          nd_esam_result(0x9000, 2, 0, 0));
}

static void test_optional_mac_accepts_absent_or_matching_mac(void)
{
    uint8_t expected[4] = {1, 2, 3, 4};
    uint8_t actual[4] = {1, 2, 3, 4};
    uint8_t wrong[4] = {1, 2, 3, 5};

    TEST_ASSERT_TRUE(nd_esam_mac_optional_matches(NULL, 0, NULL, 0));
    TEST_ASSERT_FALSE(nd_esam_mac_optional_matches(expected, 4, NULL, 0));
    TEST_ASSERT_FALSE(nd_esam_mac_optional_matches(NULL, 0, actual, 4));
    TEST_ASSERT_TRUE(nd_esam_mac_optional_matches(expected, 4, actual, 4));
    TEST_ASSERT_FALSE(nd_esam_mac_optional_matches(expected, 4, wrong, 4));
    TEST_ASSERT_FALSE(nd_esam_mac_optional_matches(expected, 3, actual, 4));
    TEST_ASSERT_FALSE(nd_esam_mac_optional_matches(expected, 4, actual, 3));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_esam_status_must_be_ok);
    RUN_TEST(test_type_zero_ignores_mac);
    RUN_TEST(test_type_one_can_enforce_or_skip_mac);
    RUN_TEST(test_unknown_verification_type_fails);
    RUN_TEST(test_optional_mac_accepts_absent_or_matching_mac);
    return UNITY_END();
}
