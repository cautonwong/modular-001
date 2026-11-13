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

static uint16_t record_len_return;
static uint16_t record_rsd;
static uint16_t record_rcsd;

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->ctx;
}

uint8_t *nd_fr698_apdu_cache(struct instance_t *unused)
{
    static uint8_t cache[16];
    (void)unused;
    return cache;
}

uint16_t axdr_record_len_get(uint8_t *unused, uint16_t *rsd_len,
                             uint16_t *rcsd_len)
{
    (void)unused;
    *rsd_len = record_rsd;
    *rcsd_len = record_rcsd;
    return record_len_return;
}

int GET_record_sel_1(struct instance_t *unused, uint32_t unused2,
                     uint8_t *unused3, struct rsd_info_t *unused4,
                     uint32_t *unused5, uint16_t unused6, void *unused7)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    (void)unused6;
    (void)unused7;
    return 0;
}

int GET_record_sel_2(struct instance_t *unused, uint32_t unused2,
                     uint8_t *unused3, struct rsd_info_t *unused4,
                     uint32_t *unused5, uint16_t unused6, void *unused7)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    (void)unused6;
    (void)unused7;
    return 0;
}

int GET_record_sel_9(struct instance_t *unused, uint32_t unused2,
                     uint8_t *unused3, struct rsd_info_t *unused4,
                     uint32_t *unused5, uint16_t unused6, void *unused7)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    (void)unused5;
    (void)unused6;
    (void)unused7;
    return 0;
}

uint32_t buf_to_uint32_msb(uint8_t *unused)
{
    (void)unused;
    return 0;
}

void ndv_memcopy(void *unused, void *unused2, uint16_t unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
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

int rsd_decode(struct instance_t *unused, struct pipeline_t *unused2,
               uint32_t unused3, struct rsd_info_t *unused4)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    return 0;
}

int rcsd_decode(struct instance_t *unused, struct pipeline_t *unused2,
                uint32_t unused3, uint32_t *unused4)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    (void)unused4;
    return 0;
}

void setUp(void)
{
    record_len_return = 5;
    record_rsd = 1;
    record_rcsd = 1;
}

void tearDown(void) {}

static void test_698_get_normal_consumes_one_oad(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 1;

    TEST_ASSERT_EQUAL_INT(4, get_GET_Request_len(NULL, &pipeline, NULL));
}

static void test_698_get_normal_list_consumes_count_and_oads(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 2;
    ctx.apdu_d_rx[0] = 2;

    TEST_ASSERT_EQUAL_INT(9, get_GET_Request_len(NULL, &pipeline, NULL));
}

static void test_698_get_record_consumes_oad_and_selectors(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 3;

    TEST_ASSERT_EQUAL_INT(9, get_GET_Request_len(NULL, &pipeline, NULL));
}

static void test_698_get_record_list_consumes_each_record(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 4;
    ctx.apdu_d_rx[0] = 2;

    TEST_ASSERT_EQUAL_INT(19, get_GET_Request_len(NULL, &pipeline, NULL));
}

static void test_698_get_record_rejects_empty_selectors(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.choice = 3;
    record_rsd = 0;

    TEST_ASSERT_EQUAL_INT(-1, get_GET_Request_len(NULL, &pipeline, NULL));
}

static void test_698_get_next_and_unknown_choice(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;

    ctx.choice = 5;
    TEST_ASSERT_EQUAL_INT(2, get_GET_Request_len(NULL, &pipeline, NULL));

    ctx.choice = 6;
    TEST_ASSERT_EQUAL_INT(-1, get_GET_Request_len(NULL, &pipeline, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_get_normal_consumes_one_oad);
    RUN_TEST(test_698_get_normal_list_consumes_count_and_oads);
    RUN_TEST(test_698_get_record_consumes_oad_and_selectors);
    RUN_TEST(test_698_get_record_list_consumes_each_record);
    RUN_TEST(test_698_get_record_rejects_empty_selectors);
    RUN_TEST(test_698_get_next_and_unknown_choice);
    return UNITY_END();
}
