#include "unity.h"

#include "domain/freeze/storage.h"

void setUp(void) {}
void tearDown(void) {}

static void test_freeze_storage_zero_depth(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, nd_freeze_scheme_units(1, 10, 0));
    TEST_ASSERT_EQUAL_UINT32(0, nd_freeze_scheme_units(0, 10, 0));
}

static void test_freeze_storage_flash_rounds_up_sectors(void)
{
    TEST_ASSERT_EQUAL_UINT32(2, nd_freeze_scheme_units(1, 10, 1));
    TEST_ASSERT_EQUAL_UINT32(2, nd_freeze_scheme_units(1, 10, 409));
    TEST_ASSERT_EQUAL_UINT32(3, nd_freeze_scheme_units(1, 10, 410));
}

static void test_freeze_storage_eeprom_counts_bytes(void)
{
    TEST_ASSERT_EQUAL_UINT32(51, nd_freeze_scheme_units(0, 10, 5));
    TEST_ASSERT_EQUAL_UINT32(11, nd_freeze_scheme_units(0, 2, 5));
}

static void test_freeze_storage_large_record_fits_one_per_sector(void)
{
    TEST_ASSERT_EQUAL_UINT32(4, nd_freeze_scheme_units(1, 8192, 3));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_freeze_storage_zero_depth);
    RUN_TEST(test_freeze_storage_flash_rounds_up_sectors);
    RUN_TEST(test_freeze_storage_eeprom_counts_bytes);
    RUN_TEST(test_freeze_storage_large_record_fits_one_per_sector);
    return UNITY_END();
}
