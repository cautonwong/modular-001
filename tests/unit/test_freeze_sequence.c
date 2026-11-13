#include "unity.h"

#include "domain/freeze/sequence.h"

void setUp(void) {}

void tearDown(void) {}

static void test_rollback_resets_when_the_log_is_empty(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, nd_freeze_sequence_rollback(42, 3, 0));
    TEST_ASSERT_EQUAL_UINT32(0, nd_freeze_sequence_rollback(42, 0, 0));
}

static void test_rollback_subtracts_the_removed_records(void)
{
    TEST_ASSERT_EQUAL_UINT32(7, nd_freeze_sequence_rollback(10, 3, 1));
    TEST_ASSERT_EQUAL_UINT32(10, nd_freeze_sequence_rollback(10, 0, 1));
}

static void test_rollback_floors_at_zero(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, nd_freeze_sequence_rollback(2, 5, 1));
    TEST_ASSERT_EQUAL_UINT32(0, nd_freeze_sequence_rollback(0, 1, 1));
}

static void test_recover_takes_the_last_record_when_it_is_ahead(void)
{
    TEST_ASSERT_EQUAL_UINT32(9, nd_freeze_sequence_recover(3, 8));
    TEST_ASSERT_EQUAL_UINT32(9, nd_freeze_sequence_recover(9, 8));
}

static void test_recover_keeps_the_counter_when_it_is_ahead(void)
{
    TEST_ASSERT_EQUAL_UINT32(12, nd_freeze_sequence_recover(12, 8));
}

static void test_recover_ignores_the_erased_sentinel(void)
{
    TEST_ASSERT_EQUAL_UINT32(12, nd_freeze_sequence_recover(12, 0xFFFFFFFFu));
}

static void test_rollback_required_only_for_backwards_clock(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_rollback_required(100, 100));
    TEST_ASSERT_EQUAL_UINT8(0, nd_freeze_rollback_required(100, 101));
    TEST_ASSERT_EQUAL_UINT8(1, nd_freeze_rollback_required(100, 99));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rollback_resets_when_the_log_is_empty);
    RUN_TEST(test_rollback_subtracts_the_removed_records);
    RUN_TEST(test_rollback_floors_at_zero);
    RUN_TEST(test_recover_takes_the_last_record_when_it_is_ahead);
    RUN_TEST(test_recover_keeps_the_counter_when_it_is_ahead);
    RUN_TEST(test_recover_ignores_the_erased_sentinel);
    RUN_TEST(test_rollback_required_only_for_backwards_clock);
    return UNITY_END();
}
