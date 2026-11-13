#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_matcher.h"

void setUp(void) {}
void tearDown(void) {}

static void test_698_frame_matches(void)
{
    struct context_698_t ctx;
    uint8_t frame[] = {
        0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 0x05, 0x01,
        0x00, 0x60, 0x12, 0x02, 0x00, 0x00, 0x10, 0x85,
        0x16
    };
    int result = 0;

    memset(&ctx, 0, sizeof(ctx));
    for (size_t i = 0; i < sizeof(frame); i++) {
        result = __matcher_698(&ctx, &frame[i]);
    }
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_UINT16(0x17, ctx.min_len);
    TEST_ASSERT_EQUAL_UINT8(0x43, ctx.ctrl);
    TEST_ASSERT_EQUAL_UINT8(0x05, ctx.SA);
}

static void test_698_frame_rejects_bad_hcs(void)
{
    struct context_698_t ctx;
    uint8_t frame[] = {
        0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 0x05, 0x01,
        0x00, 0x60, 0x12, 0x02, 0x00, 0x00, 0x10, 0x85,
        0x16
    };
    int result = 0;

    frame[11] ^= 0x01;
    memset(&ctx, 0, sizeof(ctx));
    for (size_t i = 0; i < sizeof(frame); i++) {
        result = __matcher_698(&ctx, &frame[i]);
        if (result < 0) {
            break;
        }
    }
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_EQUAL_UINT8(0, ctx.step);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_frame_matches);
    RUN_TEST(test_698_frame_rejects_bad_hcs);
    return UNITY_END();
}
