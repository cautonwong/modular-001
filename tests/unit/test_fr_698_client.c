#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/fr_698_internal.h"
#include "application/ports/fifo.h"

struct instance_t {
    uint8_t unused;
};

struct pipeline_t {
    void *ctx;
    struct fifo_t *in_queue;
    struct fifo_t *out_queue;
    uint8_t id;
};

static int forward_calls;
static uint16_t forward_len;
static uint8_t forward_service;
static uint8_t forward_payload[8];

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

static int capture_forward(struct instance_t *unused, struct pipeline_t *unused2,
                           nd_dl698_msg_t *mesg)
{
    (void)unused;
    (void)unused2;
    forward_calls++;
    forward_len = mesg->len;
    forward_service = mesg->qos;
    memcpy(forward_payload, mesg->payload, mesg->len);
    return 0;
}

void setUp(void)
{
    forward_calls = 0;
    forward_len = 0;
    forward_service = 0;
    memset(forward_payload, 0, sizeof(forward_payload));
}

void tearDown(void) {}

static void test_698_client_rejects_request_frame(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    struct fifo_t apdu;
    uint8_t in_buffer[16] = {0};
    uint8_t apdu_buffer[8] = {0};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.ctrl = 0x00;
    ctx.min_len = 10;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_init(&apdu, apdu_buffer, sizeof(apdu_buffer));
    fifoBuf_putData(&in, in_buffer, 12);
    ctx.apdu = &apdu;
    ctx.forward = capture_forward;
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;

    TEST_ASSERT_EQUAL_INT(0, fr_gatekeeper_698_client(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&in));
    TEST_ASSERT_EQUAL_INT(0, forward_calls);
}

static void test_698_client_ignores_segment_frame(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    uint8_t in_buffer[16] = {0};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.ctrl = 0xA0;
    ctx.min_len = 10;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_putData(&in, in_buffer, 12);
    ctx.forward = capture_forward;
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;

    TEST_ASSERT_EQUAL_INT(0, fr_gatekeeper_698_client(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&in));
    TEST_ASSERT_EQUAL_INT(0, forward_calls);
}

static void test_698_client_forwards_response_payload(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    struct fifo_t apdu;
    uint8_t in_buffer[32];
    uint8_t apdu_buffer[8] = {0};
    uint8_t frame[15] = {0};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.ctrl = 0x80;
    ctx.SA = 0;
    frame[9] = 0x05;
    frame[10] = 0x02;
    frame[11] = 0x11;
    frame[12] = 0xAA;
    frame[13] = 0xBB;
    frame[14] = 0xCC;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_init(&apdu, apdu_buffer, sizeof(apdu_buffer));
    fifoBuf_putData(&in, frame, sizeof(frame));
    ctx.apdu = &apdu;
    ctx.forward = capture_forward;
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;
    pipeline.id = FR_698_PID_RX;

    TEST_ASSERT_EQUAL_INT(0, fr_gatekeeper_698_client(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_INT(1, forward_calls);
    TEST_ASSERT_EQUAL_UINT16(3, forward_len);
    TEST_ASSERT_EQUAL_UINT8(0x02, forward_service);
    TEST_ASSERT_EQUAL_MEMORY(frame + 12, forward_payload, 3);
    TEST_ASSERT_EQUAL_UINT8(0x80, mesg.ctrl);
    TEST_ASSERT_EQUAL_UINT8(FR_698_PID_RX, mesg.from);
}

static void test_698_client_without_callback_drains_payload(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    struct fifo_t apdu;
    uint8_t in_buffer[32];
    uint8_t apdu_buffer[8] = {0};
    uint8_t frame[15] = {0};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.ctrl = 0x80;
    ctx.SA = 0;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_init(&apdu, apdu_buffer, sizeof(apdu_buffer));
    fifoBuf_putData(&in, frame, sizeof(frame));
    ctx.apdu = &apdu;
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;

    TEST_ASSERT_EQUAL_INT(0, fr_gatekeeper_698_client(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&in));
    TEST_ASSERT_EQUAL_INT(0, forward_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_client_rejects_request_frame);
    RUN_TEST(test_698_client_ignores_segment_frame);
    RUN_TEST(test_698_client_forwards_response_payload);
    RUN_TEST(test_698_client_without_callback_drains_payload);
    return UNITY_END();
}
