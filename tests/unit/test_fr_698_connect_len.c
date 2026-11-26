#include "unity.h"

#include <string.h>

#include "components/core/fifo/fifo.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "application/adapters/dl698/fr_698_internal.h"
#include "application/ports/record_storage.h"
#include "support/fake_meter_base.h"

struct fake_instance_t {
    uint8_t unused;
};

struct fake_pipeline_t {
    void *ctx;
    uint8_t id;
};

static struct connect_request_t test_cfg;
static int16_t nd_get_result;
static int mem_compare_result;

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct fake_pipeline_t *)pipeline)->ctx;
}

const nd_record_storage_port_t *nd_fr698_record_storage(void *unused)
{
    (void)unused;
    return NULL;
}

struct connect_request_t *nd_fr698_cfg_connect(void *unused)
{
    (void)unused;
    return &test_cfg;
}

uint32_t buf_to_uint32_msb(uint8_t *unused)
{
    (void)unused;
    return 0;
}

uint8_t mem_compare(void *unused, void *unused2, uint16_t unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return (uint8_t)mem_compare_result;
}

void mem_swap_cpy(void *unused, void *unused2, unsigned short unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
}

void ndv_memcopy(void *tb, void *sb, uint16_t length)
{
    memcpy(tb, sb, length);
}

int read_esam_info(void *unused, uint8_t *unused2,
                   uint8_t unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int esam_consult_key(void *unused, uint8_t unused2, uint8_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int16_t nd_get(void *unused, uint32_t unused2, uint8_t *buffer,
               uint16_t length, void *unused5)
{
    (void)unused;
    (void)unused2;
    (void)unused5;
    if (nd_get_result >= 0 && buffer != NULL) {
        memset(buffer, 0, length);
    }
    return nd_get_result;
}

int MeterWr_obj_list_clear(void *unused, void *unused2,
                           nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

void setUp(void)
{
    memset(&test_cfg, 0, sizeof(test_cfg));
    test_meter_base_reset();
    nd_get_result = 0;
    mem_compare_result = 1;
}

void tearDown(void) {}

static void test_698_connect_null_security_is_two_bytes(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_len_after_decode = 64;
    ctx.apdu_d_rx[37] = 0;

    TEST_ASSERT_EQUAL_INT(39, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_password_security_uses_octet_length(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_len_after_decode = 64;
    ctx.apdu_d_rx[37] = 1;
    ctx.apdu_d_rx[38] = 4;

    TEST_ASSERT_EQUAL_INT(43, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_symmetry_security_uses_two_octet_strings(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_len_after_decode = 64;
    ctx.apdu_d_rx[37] = 2;
    ctx.apdu_d_rx[38] = 3;
    ctx.apdu_d_rx[42] = 2;

    TEST_ASSERT_EQUAL_INT(45, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_defers_unknown_security(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[37] = 9;
    ctx.apdu_len_after_decode = (uint16_t)sizeof(struct connect_request_t);

    TEST_ASSERT_EQUAL_INT(FRM_LEN_DEFER,
                          get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_rejects_short_fixed_body(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[37] = 9;
    ctx.apdu_len_after_decode = (uint16_t)(sizeof(struct connect_request_t) - 1);

    TEST_ASSERT_EQUAL_INT(-1, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_failure_restores_connect_mode(void)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    struct fifo_t fifo;
    uint8_t cache_buf[256];

    memset(&ctx, 0, sizeof(ctx));
    memset(&fifo, 0, sizeof(fifo));
    memset(&test_cfg, 0, sizeof(test_cfg));
    test_cfg.protocal_conformance[0] = 0xAA;
    fifo.buf_ptr = cache_buf;
    fifo.wr = 64;
    ctx.apdu = &fifo;
    ctx.apdu_d_rx[37] = 1;  // PasswordSecurity
    pipeline.ctx = &ctx;
    nd_get_result = -1;

    TEST_ASSERT_EQUAL_INT(1, fr_handle_connect(NULL, &pipeline, NULL));
    TEST_ASSERT_EQUAL_INT(0, test_cfg.connect_mode);
    TEST_ASSERT_EQUAL_UINT8(0xAA, test_cfg.protocal_conformance[0]);
}

struct connect_rejection {
    uint16_t wr;
    uint8_t first;
    uint8_t result;
    uint8_t extra;
    uint8_t cfg_mode;
    uint8_t canary;
    uint8_t client0;
    uint8_t status;
};

static struct connect_rejection run_connect_rejection(uint8_t mode,
                                                      uint8_t sa_type_raw,
                                                      int16_t get_result,
                                                      int compare_result)
{
    struct context_698_t ctx;
    struct fake_pipeline_t pipeline;
    struct fifo_t fifo;
    uint8_t cache_buf[256] = {0};
    struct connect_rejection out = {0};

    memset(&ctx, 0, sizeof(ctx));
    memset(&fifo, 0, sizeof(fifo));
    memset(&test_cfg, 0, sizeof(test_cfg));
    test_meter_base_reset();
    test_meter_base_clientaddr()[0] = 0xEE;
    test_cfg.protocal_conformance[0] = 0xAA;
    fifo.buf_ptr = cache_buf;
    fifo.wr = 64;
    ctx.apdu = &fifo;
    ctx.apdu_d_rx[37] = mode;
    ctx.SA = 0; // fr_apdu_effective_check 已清掉类型位
    ctx.sa_type_raw = sa_type_raw;
    pipeline.ctx = &ctx;
    nd_get_result = get_result;
    mem_compare_result = compare_result;

    (void)fr_handle_connect(NULL, &pipeline, NULL);
    out.wr = fifo.wr;
    out.first = cache_buf[61];
    if (fifo.wr >= 2 && fifo.wr <= sizeof(cache_buf)) {
        out.result = cache_buf[fifo.wr - 2];
        out.extra = cache_buf[fifo.wr - 1];
    }
    out.cfg_mode = test_cfg.connect_mode;
    out.canary = test_cfg.protocal_conformance[0];
    out.client0 = test_meter_base_clientaddr()[0];
    out.status = test_meter_base_connect_status()[0];
    return out;
}

static void test_698_connect_rejects_mechanism_four(void)
{
    struct connect_rejection out = run_connect_rejection(4, 0, -1, 1);

    TEST_ASSERT_GREATER_THAN_UINT16(64, out.wr);
    TEST_ASSERT_EQUAL_HEX8(0x82, out.first);
    TEST_ASSERT_EQUAL_HEX8(255, out.result);
    TEST_ASSERT_EQUAL_HEX8(0, out.extra);
    TEST_ASSERT_EQUAL_INT(0, out.cfg_mode);
    TEST_ASSERT_EQUAL_HEX8(0xAA, out.canary);
    TEST_ASSERT_EQUAL_HEX8(0xEE, out.client0);
    TEST_ASSERT_EQUAL_UINT8(0, out.status);
}

static void test_698_connect_rejects_mechanism_five(void)
{
    struct connect_rejection out = run_connect_rejection(5, 0, -1, 1);

    TEST_ASSERT_GREATER_THAN_UINT16(64, out.wr);
    TEST_ASSERT_EQUAL_HEX8(0x82, out.first);
    TEST_ASSERT_EQUAL_HEX8(255, out.result);
    TEST_ASSERT_EQUAL_HEX8(0, out.extra);
    TEST_ASSERT_EQUAL_INT(0, out.cfg_mode);
    TEST_ASSERT_EQUAL_HEX8(0xAA, out.canary);
    TEST_ASSERT_EQUAL_HEX8(0xEE, out.client0);
    TEST_ASSERT_EQUAL_UINT8(0, out.status);
}

static void test_698_connect_rejects_general_address(void)
{
    struct connect_rejection out = run_connect_rejection(1, 1, 0, 0);

    TEST_ASSERT_GREATER_THAN_UINT16(64, out.wr);
    TEST_ASSERT_EQUAL_HEX8(0x82, out.first);
    TEST_ASSERT_EQUAL_HEX8(255, out.result);
    TEST_ASSERT_EQUAL_HEX8(0, out.extra);
    TEST_ASSERT_EQUAL_INT(0, out.cfg_mode);
    TEST_ASSERT_EQUAL_HEX8(0xAA, out.canary);
    TEST_ASSERT_EQUAL_HEX8(0xEE, out.client0);
    TEST_ASSERT_EQUAL_UINT8(0, out.status);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_connect_null_security_is_two_bytes);
    RUN_TEST(test_698_connect_password_security_uses_octet_length);
    RUN_TEST(test_698_connect_symmetry_security_uses_two_octet_strings);
    RUN_TEST(test_698_connect_defers_unknown_security);
    RUN_TEST(test_698_connect_rejects_short_fixed_body);
    RUN_TEST(test_698_connect_failure_restores_connect_mode);
    RUN_TEST(test_698_connect_rejects_mechanism_four);
    RUN_TEST(test_698_connect_rejects_mechanism_five);
    RUN_TEST(test_698_connect_rejects_general_address);
    return UNITY_END();
}
