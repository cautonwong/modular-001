#include "unity.h"

#include <string.h>

#include "application/adapters/dl645/fr_645_context.h"
#include "application/adapters/dl645/fr_645_pack.h"
#include "application/ports/fifo.h"

struct pipeline_t {
    uint8_t id;
    char *name;
    void *ctx;
    struct fifo_t *in_queue;
    struct fifo_t *out_queue;
};

static uint8_t meter_addr[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};

void *nd_fr645_pipeline_ctx(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->ctx;
}

void *nd_fr645_pipeline_out_queue(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->out_queue;
}

uint8_t *nd_fr645_tsa(struct instance_t *instance)
{
    (void)instance;
    return meter_addr;
}

void setUp(void) {}
void tearDown(void) {}

static void test_645_head_data_and_tail(void)
{
    struct context_645_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t out;
    uint8_t out_buffer[32];
    nd_dl645_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    fifoBuf_init(&out, out_buffer, sizeof(out_buffer));
    pipeline.ctx = &ctx;
    pipeline.out_queue = &out;
    mesg.ctrl = 0x11;
    mesg.len = 0;

    TEST_ASSERT_EQUAL_INT(1, fr_pack_645_head(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_INT(1, fr_pack_645_data(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_INT(1, fr_pack_645_tail(NULL, &pipeline, &mesg));

    TEST_ASSERT_EQUAL_UINT16(16, fifoBuf_getUsed(&out));
    TEST_ASSERT_EQUAL_UINT8(0xFE, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x68, out_buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x12, out_buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0x12, out_buffer[10]);
    TEST_ASSERT_EQUAL_UINT8(0x68, out_buffer[11]);
    TEST_ASSERT_EQUAL_UINT8(0x11, out_buffer[12]);
    TEST_ASSERT_EQUAL_UINT8(0x00, out_buffer[13]);
    TEST_ASSERT_EQUAL_UINT8(ctx.cs, out_buffer[14]);
    TEST_ASSERT_EQUAL_UINT8(0x16, out_buffer[15]);
}

static void test_645_apdu_bytes_are_scrambled(void)
{
    struct context_645_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t out;
    uint8_t apdu_buffer[8];
    uint8_t out_buffer[8];
    uint8_t payload[2] = {0xAA, 0xBB};
    nd_dl645_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.apdu.buf_ptr = apdu_buffer;
    ctx.apdu.buf_size = sizeof(apdu_buffer);
    ctx.apdu.rd = 0;
    ctx.apdu.wr = 0;
    memcpy(apdu_buffer, payload, sizeof(payload));
    ctx.apdu.wr = sizeof(payload);
    fifoBuf_init(&out, out_buffer, sizeof(out_buffer));
    pipeline.ctx = &ctx;
    pipeline.out_queue = &out;
    mesg.len = sizeof(payload);

    TEST_ASSERT_EQUAL_INT(1, fr_pack_645_apdu(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(2, fifoBuf_getUsed(&out));
    TEST_ASSERT_EQUAL_UINT8(0xDD, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xEE, out_buffer[1]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(0xDD + 0xEE), ctx.cs);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_645_head_data_and_tail);
    RUN_TEST(test_645_apdu_bytes_are_scrambled);
    return UNITY_END();
}
