#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/fr_698_context.h"
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

static int rx_end_calls;
static uint16_t rx_end_len;
static int send_calls;

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

int fr_rx_698_end(struct instance_t *unused, struct pipeline_t *unused2,
                  nd_dl698_msg_t *mesg)
{
    (void)unused;
    (void)unused2;
    rx_end_calls++;
    rx_end_len = mesg->len;
    return 0;
}

int __fr_send_handle(struct instance_t *unused, struct pipeline_t *unused2,
                     nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    send_calls++;
    return 1;
}

void setUp(void)
{
    rx_end_calls = 0;
    rx_end_len = 0;
    send_calls = 0;
}

void tearDown(void) {}

static void test_698_apdu_data_len_accounts_for_address(void)
{
    struct context_698_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.min_len = 20;
    ctx.SA = 0x03;

    TEST_ASSERT_EQUAL_INT(7, __get_apdu_data_len(&ctx));
}

static void test_698_scrambler_only_runs_when_flag_set(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[0] = 0x33;
    ctx.apdu_d_rx[1] = 0x66;
    ctx.apdu_rx_len = 2;

    ctx.ctrl = 0;
    TEST_ASSERT_EQUAL_INT(1, handle_scrambler(NULL, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(0x33, ctx.apdu_d_rx[0]);

    ctx.ctrl = 0x08;
    TEST_ASSERT_EQUAL_INT(1, handle_scrambler(NULL, &pipeline, NULL));
    TEST_ASSERT_EQUAL_UINT8(0x00, ctx.apdu_d_rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x33, ctx.apdu_d_rx[1]);
}

static void test_698_first_segment_accumulates_and_confirms(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    struct fifo_t out;
    uint8_t in_buffer[8];
    uint8_t out_buffer[8];
    uint8_t payload[] = {0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.min_len = 17;
    ctx.SA = 0;
    ctx.segment_rx = 0xffff;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_init(&out, out_buffer, sizeof(out_buffer));
    fifoBuf_putData(&in, payload, sizeof(payload));
    ctx.apdu = &out;
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;
    pipeline.id = FR_698_PID_RX;

    TEST_ASSERT_EQUAL_INT(0, fr_segments_handle(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(5, ctx.rx_block.total_len);
    TEST_ASSERT_EQUAL_UINT16(5, ctx.apdu_rx_len);
    TEST_ASSERT_EQUAL_UINT8(1, ctx.segment_no);
    TEST_ASSERT_EQUAL_MEMORY(payload + 2, ctx.apdu_d_rx, 5);
    TEST_ASSERT_EQUAL_UINT16(2, fifoBuf_getUsed(&out));
    TEST_ASSERT_EQUAL_UINT8(0x00, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x80, out_buffer[1]);
    TEST_ASSERT_EQUAL_INT(1, rx_end_calls);
    TEST_ASSERT_EQUAL_UINT16(2, rx_end_len);
}

static void test_698_out_of_order_segment_resets_block(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    struct fifo_t out;
    uint8_t in_buffer[8];
    uint8_t out_buffer[8];
    uint8_t payload[] = {0x02, 0xc0, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.min_len = 17;
    ctx.SA = 0;
    ctx.segment_rx = 0xffff;
    ctx.segment_no = 5;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_init(&out, out_buffer, sizeof(out_buffer));
    fifoBuf_putData(&in, payload, sizeof(payload));
    ctx.apdu = &out;
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;
    pipeline.id = FR_698_PID_RX;

    TEST_ASSERT_EQUAL_INT(0, fr_segments_handle(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(0, ctx.rx_block.total_len);
    TEST_ASSERT_EQUAL_UINT8(0, ctx.segment_no);
    TEST_ASSERT_EQUAL_UINT16(2, fifoBuf_getUsed(&out));
    TEST_ASSERT_EQUAL_UINT8(0x02, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x80, out_buffer[1]);
    TEST_ASSERT_EQUAL_INT(1, rx_end_calls);
}

static void test_698_segment_tx_confirm_advances_and_sends(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;
    struct fifo_t in;
    uint8_t in_buffer[4];
    uint8_t payload[] = {0x03, 0x80};
    nd_dl698_msg_t mesg;

    memset(&ctx, 0, sizeof(ctx));
    memset(&mesg, 0, sizeof(mesg));
    ctx.ctrl = 0x20;
    ctx.segment_tx = 1;
    ctx.segment_tx_no = 3;
    fifoBuf_init(&in, in_buffer, sizeof(in_buffer));
    fifoBuf_putData(&in, payload, sizeof(payload));
    pipeline.ctx = &ctx;
    pipeline.in_queue = &in;

    TEST_ASSERT_EQUAL_INT(1, __fr_segment_tx_handle(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(4, ctx.segment_tx_no);
    TEST_ASSERT_EQUAL_INT(1, send_calls);

    ctx.ctrl = 0;
    ctx.segment_tx = 1;
    ctx.segment_tx_no = 3;
    TEST_ASSERT_EQUAL_INT(0, __fr_segment_tx_handle(NULL, &pipeline, &mesg));
    TEST_ASSERT_EQUAL_UINT16(0, ctx.segment_tx_no);
    TEST_ASSERT_EQUAL_INT(1, send_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_apdu_data_len_accounts_for_address);
    RUN_TEST(test_698_scrambler_only_runs_when_flag_set);
    RUN_TEST(test_698_first_segment_accumulates_and_confirms);
    RUN_TEST(test_698_out_of_order_segment_resets_block);
    RUN_TEST(test_698_segment_tx_confirm_advances_and_sends);
    return UNITY_END();
}
