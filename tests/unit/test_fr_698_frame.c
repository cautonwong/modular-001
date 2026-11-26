#include "unity.h"

#include <string.h>
#include <time.h>

#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_internal.h"
#include "components/core/fifo/fifo.h"
#include "support/fake_meter_base.h"

struct fake_instance_t {
    int unused;
};

struct fake_pipeline_t {
    void *ctx;
    struct fifo_t *in_queue;
    struct fifo_t *out_queue;
    uint8_t id;
};

static struct fake_instance_t instance;

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct fake_pipeline_t *)pipeline)->ctx;
}

uint32_t nd_fr698_pipeline_id(const void *pipeline)
{
    return ((const struct fake_pipeline_t *)pipeline)->id;
}

void *nd_fr698_pipeline_in_queue(const void *pipeline)
{
    return ((const struct fake_pipeline_t *)pipeline)->in_queue;
}

void *nd_fr698_pipeline_out_queue(const void *pipeline)
{
    return ((const struct fake_pipeline_t *)pipeline)->out_queue;
}

struct tm *nd_fr698_time(void *unused)
{
    static struct tm value;
    (void)unused;
    return &value;
}

uint8_t mem_compare(void *data1, void *data2, uint16_t length)
{
    return memcmp(data1, data2, length) == 0 ? 0 : 1;
}

int get_CONNECT_Request_len(void *unused,
                            void *unused2,
                            nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int get_GET_Request_len(void *unused,
                        void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int get_SET_and_ACTION_Request_len(void *unused,
                                   void *unused2,
                                   nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_follow_report_optional(void *unused,
                              void *unused2,
                              nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

uint8_t dl_data_time_check(uint8_t *unused, uint8_t unused2)
{
    (void)unused;
    (void)unused2;
    return 0;
}

uint32_t dl_timeTag_interval(uint8_t *unused, uint8_t unused2)
{
    (void)unused;
    (void)unused2;
    return 0;
}

uint32_t dl_datetime_to_second(uint8_t *unused, uint8_t unused2)
{
    (void)unused;
    (void)unused2;
    return 0;
}

uint32_t rtc_mktime(struct tm *unused)
{
    (void)unused;
    return 0;
}

void setUp(void)
{
    memset(&instance, 0, sizeof(instance));
    test_meter_base_reset();
    test_meter_base_set_valid(1);
}

void tearDown(void) {}

static void test_698_server_rejects_response_frame(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    struct fifo_t fifo;
    uint8_t buffer[16] = {0};
    uint8_t frame[12] = {0};

    memset(&ctx, 0, sizeof(ctx));
    ctx.ctrl = 0x80;
    ctx.min_len = 10;
    fifoBuf_init(&fifo, buffer, sizeof(buffer));
    fifoBuf_putData(&fifo, frame, sizeof(frame));
    pipeline.ctx = &ctx;
    pipeline.in_queue = &fifo;
    pipeline.id = FR_698_PID_RX;

    TEST_ASSERT_EQUAL_INT(0, fr_pack_698_check(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&fifo));
}

static void test_698_server_accepts_request_and_sets_channel(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    ctx.ctrl = 0x43;
    ctx.SA = 0x01;
    ctx.addr[0] = 0x12;
    ctx.addr[1] = 0x34;
    test_meter_base_tsa()[0] = 0x12;
    test_meter_base_tsa()[1] = 0x34;
    pipeline.ctx = &ctx;
    pipeline.id = FR_698_PID_RX;

    TEST_ASSERT_EQUAL_INT(1, fr_pack_698_check(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(FR_698_CHL_RS485_1, ctx.channel);
    TEST_ASSERT_EQUAL_INT(2, *test_meter_base_commdisp_ir485());
}

static void test_698_wildcard_address_matches_and_canonicalizes(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    ctx.SA = 0x41;
    ctx.addr[0] = 0x1a;
    ctx.addr[1] = 0xa4;
    test_meter_base_tsa()[0] = 0x12;
    test_meter_base_tsa()[1] = 0x34;
    pipeline.ctx = &ctx;

    TEST_ASSERT_EQUAL_INT(1, fr_addr_handle(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(0x12, ctx.addr[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, ctx.addr[1]);
}

static void test_698_rejects_broadcast_for_set_and_get(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.SA = 0xC0;
    ctx.addr[0] = 0xaa;
    ctx.service = ACTION_Request;
    TEST_ASSERT_EQUAL_INT(1, fr_apdu_effective_check(&instance, &pipeline, NULL));

    ctx.service = GET_Request;
    TEST_ASSERT_EQUAL_INT(0, fr_apdu_effective_check(&instance, &pipeline, NULL));

    ctx.SA = 0x41;
    ctx.service = SET_Request;
    TEST_ASSERT_EQUAL_INT(0, fr_apdu_effective_check(&instance, &pipeline, NULL));
}

static void test_698_effective_check_masks_wildcard_address(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.SA = 0x41;
    ctx.service = GET_Request;
    ctx.apdu_len_valid = 3;
    ctx.apdu_rx_len = 3;

    TEST_ASSERT_EQUAL_INT(1, fr_apdu_effective_check(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(0x01, ctx.SA);
}

static void test_698_time_tag_optional_keeps_normal_tail(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    static uint8_t buffer[64];
    struct fifo_t cache;

    memset(&ctx, 0, sizeof(ctx));
    memset(buffer, 0, sizeof(buffer));
    fifoBuf_init(&cache, buffer, sizeof(buffer));
    ctx.apdu = &cache;
    ctx.service = GET_Request;
    ctx.timeTag_valid = 1;
    memset(ctx.timeTag, 0x5a, sizeof(ctx.timeTag));
    pipeline.ctx = &ctx;

    TEST_ASSERT_EQUAL_INT(1, fr_timeTag_optional(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(ctx.timeTag) + 1, cache.wr);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[0]);
    TEST_ASSERT_EACH_EQUAL_HEX8(0x5a, &buffer[1], sizeof(ctx.timeTag));
}

static void test_698_time_tag_optional_drops_tail_at_buffer_end(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    static uint8_t buffer[2000];
    struct fifo_t cache;

    memset(&ctx, 0, sizeof(ctx));
    memset(buffer, 0, sizeof(buffer));
    fifoBuf_init(&cache, buffer, sizeof(buffer));
    cache.wr = sizeof(buffer) - 1;
    ctx.apdu = &cache;
    ctx.service = GET_Request;
    ctx.timeTag_valid = 0;
    pipeline.ctx = &ctx;

    TEST_ASSERT_EQUAL_INT(1, fr_timeTag_optional(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(buffer), cache.wr);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[sizeof(buffer) - 1]);
}

static void test_698_time_tag_optional_writes_nothing_when_full(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    static uint8_t buffer[2000];
    struct fifo_t cache;

    memset(&ctx, 0, sizeof(ctx));
    memset(buffer, 0xAA, sizeof(buffer));
    fifoBuf_init(&cache, buffer, sizeof(buffer));
    cache.wr = sizeof(buffer);
    ctx.apdu = &cache;
    ctx.service = GET_Request;
    ctx.timeTag_valid = 1;
    memset(ctx.timeTag, 0x5a, sizeof(ctx.timeTag));
    pipeline.ctx = &ctx;

    TEST_ASSERT_EQUAL_INT(1, fr_timeTag_optional(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT16(sizeof(buffer), cache.wr);
    TEST_ASSERT_EACH_EQUAL_HEX8(0xAA, &buffer[sizeof(buffer) - 4], 4);
}

static void test_698_time_tag_optional_skips_error_tail(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    static uint8_t buffer[16];
    struct fifo_t cache;

    memset(&ctx, 0, sizeof(ctx));
    memset(buffer, 0, sizeof(buffer));
    fifoBuf_init(&cache, buffer, sizeof(buffer));
    cache.wr = 3;
    ctx.apdu = &cache;
    ctx.service = ERROR_S_Response;
    ctx.timeTag_valid = 1;
    memset(ctx.timeTag, 0x5a, sizeof(ctx.timeTag));
    pipeline.ctx = &ctx;

    TEST_ASSERT_EQUAL_INT(1, fr_timeTag_optional(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT16(3, cache.wr);
    TEST_ASSERT_EACH_EQUAL_HEX8(0x00, buffer, 3);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_server_rejects_response_frame);
    RUN_TEST(test_698_server_accepts_request_and_sets_channel);
    RUN_TEST(test_698_wildcard_address_matches_and_canonicalizes);
    RUN_TEST(test_698_rejects_broadcast_for_set_and_get);
    RUN_TEST(test_698_effective_check_masks_wildcard_address);
    RUN_TEST(test_698_time_tag_optional_keeps_normal_tail);
    RUN_TEST(test_698_time_tag_optional_drops_tail_at_buffer_end);
    RUN_TEST(test_698_time_tag_optional_writes_nothing_when_full);
    RUN_TEST(test_698_time_tag_optional_skips_error_tail);
    return UNITY_END();
}
