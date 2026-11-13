#include "unity.h"

#include "domain/common/profile_status.h"

void setUp(void) {}
void tearDown(void) {}

static void test_set_clear_and_has(void)
{
    uint8_t status = 0;
    nd_profile_status_set(&status, ND_PROFILE_STATUS_PDN | ND_PROFILE_STATUS_DNV);
    TEST_ASSERT_TRUE(nd_profile_status_has(status, ND_PROFILE_STATUS_PDN));
    TEST_ASSERT_TRUE(nd_profile_status_has(status, ND_PROFILE_STATUS_DNV));
    TEST_ASSERT_FALSE(nd_profile_status_has(status, ND_PROFILE_STATUS_CIV));
    nd_profile_status_clear(&status, ND_PROFILE_STATUS_PDN);
    TEST_ASSERT_FALSE(nd_profile_status_has(status, ND_PROFILE_STATUS_PDN));
    TEST_ASSERT_TRUE(nd_profile_status_has(status, ND_PROFILE_STATUS_DNV));
}

static void test_reserved_bits_are_dropped(void)
{
    uint8_t status = 0;
    nd_profile_status_set(&status, 0xFF);
    TEST_ASSERT_EQUAL_UINT8(0xAF, status);
    TEST_ASSERT_EQUAL_UINT8(0xAF, nd_profile_status_normalize(0xFF));
}

static void test_null_is_safe(void)
{
    nd_profile_status_set(NULL, ND_PROFILE_STATUS_ERR);
    nd_profile_status_clear(NULL, ND_PROFILE_STATUS_ERR);
    TEST_ASSERT_FALSE(nd_profile_status_has(0, ND_PROFILE_STATUS_ERR));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_set_clear_and_has);
    RUN_TEST(test_reserved_bits_are_dropped);
    RUN_TEST(test_null_is_safe);
    return UNITY_END();
}
