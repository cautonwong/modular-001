#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/fr_698_esam.h"
#include "application/adapters/dl698/fr_698_internal.h"

struct instance_t {
    uint8_t unused;
};

struct pipeline_t {
    void *ctx;
    uint8_t id;
};

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->ctx;
}

uint32_t buf_to_uint32_msb(uint8_t *unused)
{
    (void)unused;
    return 0;
}

int16_t nd_get(struct instance_t *unused, uint32_t unused2, uint8_t *unused3,
               uint16_t unused4, void *unused5)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    return 0;
}

int16_t nd_set(struct instance_t *unused, uint32_t unused2, uint8_t *unused3,
               uint16_t unused4, void *unused5)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    return 0;
}

int16_t nd_action(struct instance_t *unused, uint32_t unused2, uint8_t *unused3,
                  uint16_t unused4, void *unused5)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    return 0;
}

int security_mode_word_check(struct instance_t *unused,
                             struct pipeline_t *unused2, uint8_t unused3,
                             uint32_t unused4)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    return 0;
}

void fr_698_MeterWr_program_judge(struct instance_t *unused,
                                  struct pipeline_t *unused2,
                                  nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
}

void setUp(void) {}

void tearDown(void) {}

static void test_698_set_normal_consumes_oad_and_value(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 1;
    ctx.apdu_d_rx[4] = TYPE_null;

    TEST_ASSERT_EQUAL_INT(5, get_SET_and_ACTION_Request_len(NULL, &pipeline, NULL));
}

static void test_698_set_list_consumes_count_and_values(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 2;
    ctx.apdu_d_rx[0] = 2;
    ctx.apdu_d_rx[5] = TYPE_null;
    ctx.apdu_d_rx[10] = TYPE_null;

    TEST_ASSERT_EQUAL_INT(11, get_SET_and_ACTION_Request_len(NULL, &pipeline, NULL));
}

static void test_698_action_list_consumes_selectors(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 3;
    ctx.apdu_d_rx[0] = 2;
    ctx.apdu_d_rx[5] = TYPE_null;
    ctx.apdu_d_rx[15] = TYPE_null;

    TEST_ASSERT_EQUAL_INT(21, get_SET_and_ACTION_Request_len(NULL, &pipeline, NULL));
}

static void test_698_set_rejects_unknown_choice(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 4;

    TEST_ASSERT_EQUAL_INT(-1, get_SET_and_ACTION_Request_len(NULL, &pipeline, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_set_normal_consumes_oad_and_value);
    RUN_TEST(test_698_set_list_consumes_count_and_values);
    RUN_TEST(test_698_action_list_consumes_selectors);
    RUN_TEST(test_698_set_rejects_unknown_choice);
    return UNITY_END();
}
