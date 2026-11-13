#include "unity.h"

#include <string.h>

#include "application/adapters/dl645/fr_645_context.h"
#include "application/adapters/dl645/fr_645_matcher.h"

void setUp(void) {}
void tearDown(void) {}

static void test_645_frame_matches(void)
{
    struct context_645_t ctx;
    uint8_t frame[] = {
        0x68, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x68,
        0x11, 0x02, 0xAA, 0xBB, 0x00, 0x16
    };
    uint8_t sum = 0;
    int result = 0;

    for (size_t i = 0; i < 12; i++) {
        sum = (uint8_t)(sum + frame[i]);
    }
    frame[12] = sum;

    memset(&ctx, 0, sizeof(ctx));
    for (size_t i = 0; i < sizeof(frame); i++) {
        result = __matcher_645(&ctx, &frame[i]);
    }
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_UINT8(0x11, ctx.ctrl);
    TEST_ASSERT_EQUAL_UINT8(2, ctx.len);
    TEST_ASSERT_EQUAL_UINT8(0x12, ctx.addr[5]);
}

static void test_645_frame_rejects_bad_checksum(void)
{
    struct context_645_t ctx;
    uint8_t frame[] = {
        0x68, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x68,
        0x11, 0x02, 0xAA, 0xBB, 0x00, 0x16
    };
    uint8_t sum = 0;
    int result = 0;

    for (size_t i = 0; i < 12; i++) {
        sum = (uint8_t)(sum + frame[i]);
    }
    frame[12] = (uint8_t)(sum + 1);

    memset(&ctx, 0, sizeof(ctx));
    for (size_t i = 0; i < sizeof(frame); i++) {
        result = __matcher_645(&ctx, &frame[i]);
    }
    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_645_frame_matches);
    RUN_TEST(test_645_frame_rejects_bad_checksum);
    return UNITY_END();
}
