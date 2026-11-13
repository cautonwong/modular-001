#include "unity.h"

#include "application/adapters/dl645/fr_645_password.h"

void setUp(void) {}
void tearDown(void) {}

static void test_645_password_matches_last_three_bytes(void)
{
    const uint8_t entered[4] = {0x02, 0x01, 0x02, 0x03};
    const uint8_t stored[4] = {0x00, 0x01, 0x02, 0x03};

    TEST_ASSERT_EQUAL_UINT8(0, fr_645_password_matches(entered, stored));
}

static void test_645_password_ignores_first_byte(void)
{
    const uint8_t entered[4] = {0x04, 0x11, 0x22, 0x33};
    const uint8_t stored[4] = {0x02, 0x11, 0x22, 0x33};

    TEST_ASSERT_EQUAL_UINT8(0, fr_645_password_matches(entered, stored));
}

static void test_645_password_rejects_mismatch(void)
{
    const uint8_t entered[4] = {0x02, 0x01, 0x02, 0x03};
    const uint8_t stored[4] = {0x00, 0x01, 0xFF, 0x03};

    TEST_ASSERT_EQUAL_UINT8(1, fr_645_password_matches(entered, stored));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_645_password_matches_last_three_bytes);
    RUN_TEST(test_645_password_ignores_first_byte);
    RUN_TEST(test_645_password_rejects_mismatch);
    return UNITY_END();
}
