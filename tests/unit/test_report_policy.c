#include "unity.h"

#include "domain/report/report_policy.h"

void setUp(void) {}
void tearDown(void) {}

static void put_oad_le(uint8_t *buffer, uint32_t oad)
{
    buffer[0] = (uint8_t)oad;
    buffer[1] = (uint8_t)(oad >> 8);
    buffer[2] = (uint8_t)(oad >> 16);
    buffer[3] = (uint8_t)(oad >> 24);
}

static void test_report_flag_matches_event_scheme(void)
{
    TEST_ASSERT_TRUE(nd_report_flag_matches(ND_REPORT_FLAG_START, 1));
    TEST_ASSERT_TRUE(nd_report_flag_matches(ND_REPORT_FLAG_START, 2));
    TEST_ASSERT_FALSE(nd_report_flag_matches(ND_REPORT_FLAG_START, 3));

    TEST_ASSERT_TRUE(nd_report_flag_matches(ND_REPORT_FLAG_END, 3));
    TEST_ASSERT_TRUE(nd_report_flag_matches(ND_REPORT_FLAG_END, 4));
    TEST_ASSERT_FALSE(nd_report_flag_matches(ND_REPORT_FLAG_END, 1));

    TEST_ASSERT_TRUE(nd_report_flag_matches(ND_REPORT_FLAG_ALL, 1));
    TEST_ASSERT_TRUE(nd_report_flag_matches(ND_REPORT_FLAG_ALL, 4));
    TEST_ASSERT_FALSE(nd_report_flag_matches(ND_REPORT_FLAG_ALL, 5));
    TEST_ASSERT_FALSE(nd_report_flag_matches(0, 1));
}

static void test_channel_enabled_uses_the_configured_oad(void)
{
    uint8_t buffer[13] = {0};

    buffer[0] = 2;
    put_oad_le(buffer + 1, 0xF2010202u);
    put_oad_le(buffer + 5, 0xF2090201u);

    TEST_ASSERT_TRUE(nd_report_channel_enabled(ND_REPORT_CHANNEL_PLC, buffer,
                                               sizeof(buffer)));
    TEST_ASSERT_TRUE(nd_report_channel_enabled(ND_REPORT_CHANNEL_RS485_2,
                                               buffer, sizeof(buffer)));
    TEST_ASSERT_FALSE(nd_report_channel_enabled(ND_REPORT_CHANNEL_IR, buffer,
                                                sizeof(buffer)));
    TEST_ASSERT_FALSE(nd_report_channel_enabled(ND_REPORT_CHANNEL_RS485_1,
                                                buffer, sizeof(buffer)));
}

static void test_channel_enabled_rejects_malformed_lists(void)
{
    uint8_t buffer[13] = {0};

    TEST_ASSERT_FALSE(nd_report_channel_enabled(ND_REPORT_CHANNEL_PLC, NULL, 0));
    TEST_ASSERT_FALSE(nd_report_channel_enabled(ND_REPORT_CHANNEL_PLC, buffer,
                                                sizeof(buffer)));

    buffer[0] = 1;
    put_oad_le(buffer + 1, 0xF2090201u);
    TEST_ASSERT_FALSE(nd_report_channel_enabled(ND_REPORT_CHANNEL_PLC, buffer,
                                                4));

    buffer[0] = 5;
    TEST_ASSERT_FALSE(nd_report_channel_enabled(ND_REPORT_CHANNEL_PLC, buffer,
                                                sizeof(buffer)));
}

static void test_3011_requires_the_start_record_and_active_mode(void)
{
    TEST_ASSERT_TRUE(nd_report_3011_active_allowed(
        0, ND_REPORT_FLAG_START, ND_REPORT_MODE_ACTIVE, 0));
    TEST_ASSERT_FALSE(nd_report_3011_active_allowed(
        0, ND_REPORT_FLAG_START, ND_REPORT_MODE_FOLLOW, 0));
    TEST_ASSERT_FALSE(nd_report_3011_active_allowed(
        0, ND_REPORT_FLAG_END, ND_REPORT_MODE_ACTIVE, 0));

    TEST_ASSERT_TRUE(nd_report_3011_active_allowed(
        1, ND_REPORT_FLAG_END, ND_REPORT_MODE_ACTIVE, 1));
    TEST_ASSERT_TRUE(nd_report_3011_active_allowed(
        1, ND_REPORT_FLAG_ALL, ND_REPORT_MODE_ACTIVE, 1));
    TEST_ASSERT_FALSE(nd_report_3011_active_allowed(
        1, ND_REPORT_FLAG_END, ND_REPORT_MODE_ACTIVE, 0));
    TEST_ASSERT_FALSE(nd_report_3011_active_allowed(
        1, ND_REPORT_FLAG_START, ND_REPORT_MODE_ACTIVE, 1));
}

static void test_2015_mask_and_3320_variants(void)
{
    TEST_ASSERT_TRUE(nd_report_2015_enabled(0x00000100u, 0x00000100u));
    TEST_ASSERT_FALSE(nd_report_2015_enabled(0x00000100u, 0x00000080u));

    TEST_ASSERT_EQUAL_UINT32(0x30110206u,
                             nd_report_3320_variant(0x30110206u, 0));
    TEST_ASSERT_EQUAL_UINT32(0x30110306u,
                             nd_report_3320_variant(0x30110206u, 1));
    TEST_ASSERT_EQUAL_UINT32(0x30110506u,
                             nd_report_3320_variant(0x30110206u, 3));
    TEST_ASSERT_EQUAL_UINT32(0x30110206u,
                             nd_report_3320_variant(0x3011e206u, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_report_flag_matches_event_scheme);
    RUN_TEST(test_channel_enabled_uses_the_configured_oad);
    RUN_TEST(test_channel_enabled_rejects_malformed_lists);
    RUN_TEST(test_3011_requires_the_start_record_and_active_mode);
    RUN_TEST(test_2015_mask_and_3320_variants);
    return UNITY_END();
}
