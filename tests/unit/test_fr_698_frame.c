#include "unity.h"

#include <string.h>
#include <time.h>

#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_internal.h"
#include "application/ports/fifo.h"

struct instance_t {
    uint8_t tsa[16];
    uint16_t backlight;
    uint16_t keep_sec;
    uint8_t list_num;
    uint8_t commdisp_plc;
    uint8_t commdisp_ir485;
};

struct pipeline_t {
    void *ctx;
    struct fifo_t *in_queue;
    struct fifo_t *out_queue;
    uint8_t id;
};

static struct instance_t instance;

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->ctx;
}

uint32_t nd_fr698_pipeline_id(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->id;
}

void *nd_fr698_pipeline_in_queue(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->in_queue;
}

void *nd_fr698_pipeline_out_queue(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->out_queue;
}

struct tm *nd_fr698_time(struct instance_t *unused)
{
    static struct tm value;
    (void)unused;
    return &value;
}

int nd_mb_valid(struct instance_t *unused)
{
    (void)unused;
    return 1;
}

uint8_t *nd_mb_tsa_ptr(struct instance_t *self)
{
    return self->tsa;
}

uint16_t *nd_mb_backlight_tmr_ptr(struct instance_t *self)
{
    return &self->backlight;
}

uint16_t *nd_mb_lcd_autodis_keep_sec_ptr(struct instance_t *self)
{
    return &self->keep_sec;
}

uint8_t *nd_mb_lcd_autodis_list_num_ptr(struct instance_t *self)
{
    return &self->list_num;
}

uint8_t *nd_mb_commdisp_plc_ptr(struct instance_t *self)
{
    return &self->commdisp_plc;
}

uint8_t *nd_mb_commdisp_ir485_ptr(struct instance_t *self)
{
    return &self->commdisp_ir485;
}

uint8_t mem_compare(void *data1, void *data2, uint16_t length)
{
    return memcmp(data1, data2, length) == 0 ? 0 : 1;
}

int get_CONNECT_Request_len(struct instance_t *unused,
                            struct pipeline_t *unused2,
                            nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int get_GET_Request_len(struct instance_t *unused,
                        struct pipeline_t *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int get_SET_and_ACTION_Request_len(struct instance_t *unused,
                                   struct pipeline_t *unused2,
                                   nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_follow_report_optional(struct instance_t *unused,
                              struct pipeline_t *unused2,
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
}

void tearDown(void) {}

static void test_698_server_rejects_response_frame(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
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
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    ctx.ctrl = 0x43;
    ctx.SA = 0x01;
    ctx.addr[0] = 0x12;
    ctx.addr[1] = 0x34;
    instance.tsa[0] = 0x12;
    instance.tsa[1] = 0x34;
    pipeline.ctx = &ctx;
    pipeline.id = FR_698_PID_RX;

    TEST_ASSERT_EQUAL_INT(1, fr_pack_698_check(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(FR_698_CHL_RS485_1, ctx.channel);
    TEST_ASSERT_EQUAL_UINT8(2, instance.commdisp_ir485);
}

static void test_698_wildcard_address_matches_and_canonicalizes(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    ctx.SA = 0x41;
    ctx.addr[0] = 0x1a;
    ctx.addr[1] = 0xa4;
    instance.tsa[0] = 0x12;
    instance.tsa[1] = 0x34;
    pipeline.ctx = &ctx;

    TEST_ASSERT_EQUAL_INT(1, fr_addr_handle(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(0x12, ctx.addr[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, ctx.addr[1]);
}

static void test_698_rejects_broadcast_for_set_and_get(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

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
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.SA = 0x41;
    ctx.service = GET_Request;
    ctx.apdu_len_valid = 3;
    ctx.apdu_rx_len = 3;

    TEST_ASSERT_EQUAL_INT(1, fr_apdu_effective_check(&instance, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(0x01, ctx.SA);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_server_rejects_response_frame);
    RUN_TEST(test_698_server_accepts_request_and_sets_channel);
    RUN_TEST(test_698_wildcard_address_matches_and_canonicalizes);
    RUN_TEST(test_698_rejects_broadcast_for_set_and_get);
    RUN_TEST(test_698_effective_check_masks_wildcard_address);
    return UNITY_END();
}
