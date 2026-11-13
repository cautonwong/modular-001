#include "unity.h"

#include "domain/events/program_list.h"

void setUp(void) {}
void tearDown(void) {}

static void test_program_list_normalizes_count(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, nd_program_list_insert_count(0));
    TEST_ASSERT_EQUAL_UINT8(5, nd_program_list_insert_count(5));
    TEST_ASSERT_EQUAL_UINT8(9, nd_program_list_insert_count(10));
    TEST_ASSERT_EQUAL_UINT8(0, nd_program_list_insert_count(11));
}

static void test_program_list_first_entry_only(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_program_list_is_first(0));
    TEST_ASSERT_EQUAL_UINT8(0, nd_program_list_is_first(1));
    TEST_ASSERT_EQUAL_UINT8(0, nd_program_list_is_first(9));
}

static void test_program_list_698_time_oad(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_program_list_sets_time_698(0x40000200u));
    TEST_ASSERT_EQUAL_UINT8(0, nd_program_list_sets_time_698(0x40000201u));
}

static void test_program_list_645_time_oads(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_program_list_sets_time_645(0x04000101u));
    TEST_ASSERT_EQUAL_UINT8(1, nd_program_list_sets_time_645(0x04000102u));
    TEST_ASSERT_EQUAL_UINT8(1, nd_program_list_sets_time_645(0x0400010Cu));
    TEST_ASSERT_EQUAL_UINT8(0, nd_program_list_sets_time_645(0x04000103u));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_program_list_normalizes_count);
    RUN_TEST(test_program_list_first_entry_only);
    RUN_TEST(test_program_list_698_time_oad);
    RUN_TEST(test_program_list_645_time_oads);
    return UNITY_END();
}
