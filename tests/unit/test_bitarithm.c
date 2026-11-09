#include "unity.h"

#include "infra/kernel/bitarithm.h"

void setUp(void) {}
void tearDown(void) {}

static void test_msb_and_lsb(void)
{
    TEST_ASSERT_EQUAL_UINT(0, bitarithm_msb(1));
    TEST_ASSERT_EQUAL_UINT(15, bitarithm_msb(0x8000));
    TEST_ASSERT_EQUAL_UINT(31, bitarithm_msb(0x80000000));

    TEST_ASSERT_EQUAL_UINT(0, bitarithm_lsb(1));
    TEST_ASSERT_EQUAL_UINT(15, bitarithm_lsb(0x8000));
    TEST_ASSERT_EQUAL_UINT(4, bitarithm_lsb(0x30));
}

static void test_bits_set(void)
{
    TEST_ASSERT_EQUAL_UINT(0, bitarithm_bits_set(0));
    TEST_ASSERT_EQUAL_UINT(8, bitarithm_bits_set(0xF0F0));
    TEST_ASSERT_EQUAL_UINT(32, bitarithm_bits_set_u32(0xFFFFFFFFu));
}

static void test_test_and_clear_iterates_set_bits(void)
{
    unsigned state = 0x0A;
    uint8_t index = 0;

    state = bitarithm_test_and_clear(state, &index);
    TEST_ASSERT_EQUAL_UINT8(1, index);

    state = bitarithm_test_and_clear(state, &index);
    TEST_ASSERT_EQUAL_UINT8(3, index);
    TEST_ASSERT_EQUAL_UINT(0, state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_msb_and_lsb);
    RUN_TEST(test_bits_set);
    RUN_TEST(test_test_and_clear_iterates_set_bits);
    return UNITY_END();
}
