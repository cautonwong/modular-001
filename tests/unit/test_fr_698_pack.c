#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_pack.h"
#include "application/ports/fifo.h"

struct pipeline_t {
    uint8_t id;
    char *name;
    void *ctx;
    struct fifo_t *in_queue;
    struct fifo_t *out_queue;
};

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->ctx;
}

void *nd_fr698_pipeline_out_queue(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->out_queue;
}

void setUp(void) {}
void tearDown(void) {}

static void test_698_head_and_tail_bytes(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t out;
    uint8_t out_buffer[64];
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.ctrl = 0x41;
    ctx.SA = 0;
    ctx.addr[0] = 0x05;
    ctx.CA = 0x00;
    mesg.from = FR_698_PID_RX;
    fifoBuf_init(&out, out_buffer, sizeof(out_buffer));
    pipeline.ctx = &ctx;
    pipeline.out_queue = &out;

    TEST_ASSERT_EQUAL_INT(1, fr_pack_698_head(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_INT(1, fr_pack_698_tail(NULL, &pipeline, &mesg));

    TEST_ASSERT_EQUAL_UINT16(16, fifoBuf_getUsed(&out));
    TEST_ASSERT_EQUAL_UINT8(0xFE, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x68, out_buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x0A, out_buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0xC1, out_buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0x05, out_buffer[9]);
    TEST_ASSERT_EQUAL_UINT8(0x16, out_buffer[15]);
}

static void test_698_apdu_bytes_are_written(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    struct fifo_t out;
    uint8_t in_buffer[8];
    uint8_t out_buffer[8];
    uint8_t payload[2] = {0xAA, 0xBB};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_init(&out, out_buffer, sizeof(out_buffer));
    ctx.apdu = &in;
    pipeline.ctx = &ctx;
    pipeline.out_queue = &out;
    fifoBuf_putData(&in, payload, sizeof(payload));
    mesg.len = sizeof(payload);

    TEST_ASSERT_EQUAL_INT(1, fr_pack_698_apdu(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(2, fifoBuf_getUsed(&out));
    TEST_ASSERT_EQUAL_UINT8(0xAA, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, out_buffer[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_head_and_tail_bytes);
    RUN_TEST(test_698_apdu_bytes_are_written);
    return UNITY_END();
}
