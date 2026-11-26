#include "unity.h"

#include <string.h>

#include "components/core/fifo/fifo.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "application/adapters/dl698/fr_698_internal.h"
#include "support/fake_meter_base.h"

struct fake_pipeline_t {
    void *ctx;
    uint8_t id;
};

static struct context_698_t test_ctx;
static struct fifo_t rx_fifo;
static struct fifo_t tx_fifo;
static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];
static int connect_writes;
static int report_calls;
static int sends;

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
    (void)pipeline;
    return &rx_fifo;
}

const nd_record_storage_port_t *nd_fr698_record_storage(void *unused)
{
    (void)unused;
    return NULL;
}

int fr_pack_698_check(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 1;
}

int __fr_segment_tx_handle(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_ctx_clear(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_segments_handle(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 1;
}

int __get_apdu_data_len(struct context_698_t *ctx)
{
    (void)ctx;
    return 8;
}

int handle_scrambler(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int update_plc_baut_tick(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

void update_connect_timeout_cnt(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
}

int fr_security_request(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_timetag_check(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 1;
}

uint32_t buf_to_uint32_msb(uint8_t *unused)
{
    (void)unused;
    return 0;
}

int fr_apdu_effective_check(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 1;
}

void app_handle_mac_suspend(void *unused, uint32_t unused2)
{
    (void)unused;
    (void)unused2;
}

int fr_timeTag_optional(void *unused, void *pipeline, nd_dl698_msg_t *unused3)
{
    struct context_698_t *ctx =
        (struct context_698_t *)nd_fr698_pipeline_ctx(pipeline);

    (void)unused;
    (void)unused3;
    if ((ctx->service == SECURITY_Response) ||
        (ctx->service == ERROR_S_Response)) {
        return 1;
    }
    ctx->apdu->buf_ptr[ctx->apdu->wr++] = 0x00;//跟随上报
    ctx->apdu->buf_ptr[ctx->apdu->wr++] = 0x00;//时间标签缺省
    return 1;
}

int fr_security_encode(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int __fr_send_handle(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    sends++;
    return 0;
}

int fr_handle_connect(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    if (connect_writes) {
        test_ctx.apdu->buf_ptr[test_ctx.apdu->wr++] = 0xAA;
    }
    return 1;
}

int fr_handle_disconnect(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    struct fifo_t *cache = test_ctx.apdu;
    uint16_t offset = cache->wr;
    cache->buf_ptr[offset - 2] = cache->buf_ptr[offset - 1];
    cache->buf_ptr[offset - 1] = 0;
    return 1;
}

int fr_handle_get_request(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_handle_set_action_request(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int fr_handle_report_response(void *unused, void *unused2, nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    report_calls++;
    return 1;
}

static void run_gatekeeper_choice(uint8_t service, uint8_t choice, uint8_t piid)
{
    struct fake_pipeline_t pipeline;
    static nd_dl698_msg_t mesg;

    memset(&mesg, 0, sizeof(mesg));
    memset(&test_ctx, 0, sizeof(test_ctx));
    memset(&rx_fifo, 0, sizeof(rx_fifo));
    memset(&tx_fifo, 0, sizeof(tx_fifo));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(tx_buffer, 0, sizeof(tx_buffer));
    fifoBuf_init(&rx_fifo, rx_buffer, sizeof(rx_buffer));
    fifoBuf_init(&tx_fifo, tx_buffer, sizeof(tx_buffer));
    for (int i = 0; i < 16; i++) {
        fifoBuf_putByte(&rx_fifo, 0);
    }
    rx_fifo.buf_ptr[9] = service;
    if (service == GET_Request) {
        rx_fifo.buf_ptr[10] = choice;
        rx_fifo.buf_ptr[11] = piid;
    } else {
        rx_fifo.buf_ptr[10] = piid;
    }
    test_ctx.apdu = &tx_fifo;
    test_ctx.ctrl = 0x43;
    test_ctx.apdu_rx_len = 16;
    test_ctx.min_len = 0;
    pipeline.ctx = &test_ctx;
    pipeline.id = 0;

    (void)fr_gatekeeper_698_server(NULL, &pipeline, &mesg);
}

static void run_gatekeeper(uint8_t service, uint8_t piid)
{
    run_gatekeeper_choice(service, service == GET_Request ? 1 : 0, piid);
}

void setUp(void)
{
    test_meter_base_reset();
    connect_writes = 0;
    report_calls = 0;
    sends = 0;
}

void tearDown(void)
{
}

static void test_connect_without_response_gets_error_frame(void)
{
    run_gatekeeper(CONNECT_Request, 0x21);

    TEST_ASSERT_EQUAL_UINT8(ERROR_S_Response, test_ctx.service);
    TEST_ASSERT_EQUAL_UINT16(3, tx_fifo.wr);
    TEST_ASSERT_EQUAL_HEX8(ERROR_S_Response, tx_buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x21, tx_buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(DAR_OTHERS_ERROR, tx_buffer[2]);
}

static void test_connect_response_is_not_overwritten(void)
{
    connect_writes = 1;
    run_gatekeeper(CONNECT_Request, 0x22);

    TEST_ASSERT_EQUAL_UINT8(CONNECT_Request, test_ctx.service);
    TEST_ASSERT_EQUAL_UINT16(6, tx_fifo.wr);
    TEST_ASSERT_EQUAL_HEX8(0xAA, tx_buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0, tx_buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0, tx_buffer[5]);
    TEST_ASSERT_NOT_EQUAL(ERROR_S_Response, tx_buffer[0]);
}

static void test_release_in_place_write_is_not_overwritten(void)
{
    run_gatekeeper(RELEASE_Request, 0x23);

    TEST_ASSERT_EQUAL_UINT16(5, tx_fifo.wr);
    TEST_ASSERT_EQUAL_HEX8(RELEASE_Request | 0x80, tx_buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x23, tx_buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0, tx_buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0, tx_buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0, tx_buffer[4]);
}

static void test_report_response_sends_nothing(void)
{
    run_gatekeeper(REPORT_Response, 0x24);

    TEST_ASSERT_EQUAL_INT(1, report_calls);
    TEST_ASSERT_EQUAL_INT(0, sends);
    TEST_ASSERT_EQUAL_UINT16(3, tx_fifo.wr);
}

static void test_get_without_response_gets_error_frame(void)
{
    run_gatekeeper(GET_Request, 0x25);

    TEST_ASSERT_EQUAL_UINT8(ERROR_S_Response, test_ctx.service);
    TEST_ASSERT_EQUAL_UINT16(3, tx_fifo.wr);
    TEST_ASSERT_EQUAL_HEX8(ERROR_S_Response, tx_buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x25, tx_buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(DAR_OTHERS_ERROR, tx_buffer[2]);
}

static void test_get_empty_oad_list_still_gets_error_frame(void)
{
    run_gatekeeper_choice(GET_Request, 2, 0x26);

    TEST_ASSERT_EQUAL_UINT8(ERROR_S_Response, test_ctx.service);
    TEST_ASSERT_EQUAL_UINT16(3, tx_fifo.wr);
    TEST_ASSERT_EQUAL_HEX8(ERROR_S_Response, tx_buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x26, tx_buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(DAR_OTHERS_ERROR, tx_buffer[2]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_connect_without_response_gets_error_frame);
    RUN_TEST(test_connect_response_is_not_overwritten);
    RUN_TEST(test_release_in_place_write_is_not_overwritten);
    RUN_TEST(test_report_response_sends_nothing);
    RUN_TEST(test_get_without_response_gets_error_frame);
    RUN_TEST(test_get_empty_oad_list_still_gets_error_frame);
    return UNITY_END();
}
