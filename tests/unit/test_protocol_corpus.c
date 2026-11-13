#include "unity.h"

#include <string.h>

#include "application/adapters/dl645/fr_645_context.h"
#include "application/adapters/dl645/fr_645_matcher.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_matcher.h"

void setUp(void) {}
void tearDown(void) {}

static const uint8_t frame_645_control_11[] = {
    0x68, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x68,
    0x11, 0x02, 0xAA, 0xBB, 0xFE, 0x16,
};

static const uint8_t frame_645_bad_checksum[] = {
    0x68, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x68,
    0x11, 0x02, 0xAA, 0xBB, 0xFF, 0x16,
};

static const uint8_t frame_698_read_request[] = {
    0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 0x05, 0x01,
    0x00, 0x60, 0x12, 0x02, 0x00, 0x00, 0x10, 0x85,
    0x16,
};

static const uint8_t frame_698_bad_hcs[] = {
    0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x11, 0x26, 0xF6, 0x05, 0x01,
    0x00, 0x60, 0x12, 0x02, 0x00, 0x00, 0x10, 0x85,
    0x16,
};

static int match_645(const uint8_t *frame, size_t length,
                     struct context_645_t *ctx)
{
    int result = 0;

    memset(ctx, 0, sizeof(*ctx));
    for (size_t i = 0; i < length; i++) {
        result = __matcher_645(ctx, &frame[i]);
    }
    return result;
}

static int match_698(const uint8_t *frame, size_t length,
                     struct context_698_t *ctx)
{
    int result = 0;

    memset(ctx, 0, sizeof(*ctx));
    for (size_t i = 0; i < length; i++) {
        result = __matcher_698(ctx, &frame[i]);
        if (result < 0) {
            break;
        }
    }
    return result;
}

static void test_corpus_645_control_11(void)
{
    struct context_645_t ctx;

    TEST_ASSERT_EQUAL_INT(1, match_645(frame_645_control_11,
                                       sizeof(frame_645_control_11), &ctx));
    TEST_ASSERT_EQUAL_UINT8(0x11, ctx.ctrl);
    TEST_ASSERT_EQUAL_UINT8(2, ctx.len);
    TEST_ASSERT_EQUAL_UINT8(0x12, ctx.addr[5]);
}

static void test_corpus_645_bad_checksum(void)
{
    struct context_645_t ctx;

    TEST_ASSERT_EQUAL_INT(-1, match_645(frame_645_bad_checksum,
                                        sizeof(frame_645_bad_checksum), &ctx));
}

static void test_corpus_698_read_request(void)
{
    struct context_698_t ctx;

    TEST_ASSERT_EQUAL_INT(1, match_698(frame_698_read_request,
                                       sizeof(frame_698_read_request), &ctx));
    TEST_ASSERT_EQUAL_UINT16(0x17, ctx.min_len);
    TEST_ASSERT_EQUAL_UINT8(0x43, ctx.ctrl);
    TEST_ASSERT_EQUAL_UINT8(0x05, ctx.SA);
}

static void test_corpus_698_bad_hcs(void)
{
    struct context_698_t ctx;

    TEST_ASSERT_EQUAL_INT(-1, match_698(frame_698_bad_hcs,
                                        sizeof(frame_698_bad_hcs), &ctx));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_corpus_645_control_11);
    RUN_TEST(test_corpus_645_bad_checksum);
    RUN_TEST(test_corpus_698_read_request);
    RUN_TEST(test_corpus_698_bad_hcs);
    return UNITY_END();
}
