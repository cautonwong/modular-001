#include "unity.h"

#include <string.h>

#include "application/adapters/dl645/fr_645_addr.h"

void setUp(void) {}
void tearDown(void) {}

static void test_645_address_all_equal_value(void)
{
    uint8_t all99[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
    uint8_t allaa[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

    TEST_ASSERT_EQUAL_UINT8(0, fr_645_addr_all(all99, 0x99));
    TEST_ASSERT_EQUAL_UINT8(0, fr_645_addr_all(allaa, 0xAA));

    all99[3] = 0x98;
    allaa[0] = 0xAB;
    TEST_ASSERT_EQUAL_UINT8(1, fr_645_addr_all(all99, 0x99));
    TEST_ASSERT_EQUAL_UINT8(1, fr_645_addr_all(allaa, 0xAA));
}

static void test_645_address_exact_match(void)
{
    uint8_t addr[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t tsa[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    TEST_ASSERT_EQUAL_UINT8(0, fr_645_addr_match(addr, tsa, 0));

    tsa[5] = 0x07;
    TEST_ASSERT_EQUAL_UINT8(1, fr_645_addr_match(addr, tsa, 0));
}

static void test_645_address_wildcard_fills_trailing_bytes(void)
{
    uint8_t addr[6] = {0x01, 0x02, 0x03, 0xAA, 0xAA, 0xAA};
    uint8_t tsa[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    TEST_ASSERT_EQUAL_UINT8(0, fr_645_addr_match(addr, tsa, 1));
    TEST_ASSERT_EQUAL_MEMORY(tsa, addr, sizeof(tsa));
}

static void test_645_address_wildcard_stops_at_first_non_wildcard(void)
{
    uint8_t addr[6] = {0xAA, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t tsa[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    TEST_ASSERT_EQUAL_UINT8(1, fr_645_addr_match(addr, tsa, 1));
    TEST_ASSERT_EQUAL_UINT8(0xAA, addr[0]);
}

static void test_645_address_without_wildcard_does_not_fill(void)
{
    uint8_t addr[6] = {0x01, 0x02, 0x03, 0xAA, 0xAA, 0xAA};
    uint8_t tsa[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    TEST_ASSERT_EQUAL_UINT8(1, fr_645_addr_match(addr, tsa, 0));
    TEST_ASSERT_EQUAL_UINT8(0xAA, addr[5]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_645_address_all_equal_value);
    RUN_TEST(test_645_address_exact_match);
    RUN_TEST(test_645_address_wildcard_fills_trailing_bytes);
    RUN_TEST(test_645_address_wildcard_stops_at_first_non_wildcard);
    RUN_TEST(test_645_address_without_wildcard_does_not_fill);
    return UNITY_END();
}
