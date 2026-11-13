#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "application/adapters/dl698/fr_698_internal.h"
#include "application/adapters/dl698/meter_base_view.h"

struct instance_t {
    uint8_t unused;
};

struct pipeline_t {
    void *ctx;
    uint8_t id;
};

static uint32_t connect_timeout[4][2];
static uint32_t esam_connect_timeout[4][2];
static uint8_t connect_status[4][2];
static uint32_t connect_timeout_cnt[4][2];
static uint8_t current_connect_channel[2];
static uint8_t client_addr[16];
static uint32_t term_auth_timeout;

void *nd_fr698_pipeline_ctx(const void *pipeline)
{
    return ((const struct pipeline_t *)pipeline)->ctx;
}

uint32_t (*nd_mb_connect_timeout_ptr(struct instance_t *unused))[2]
{
    (void)unused;
    return connect_timeout;
}

uint32_t (*nd_mb_esam_connect_timeout_ptr(struct instance_t *unused))[2]
{
    (void)unused;
    return esam_connect_timeout;
}

uint8_t (*nd_mb_connect_status_ptr(struct instance_t *unused))[2]
{
    (void)unused;
    return connect_status;
}

uint32_t (*nd_mb_connect_timeout_cnt_ptr(struct instance_t *unused))[2]
{
    (void)unused;
    return connect_timeout_cnt;
}

uint8_t *nd_mb_current_connect_channel_ptr(struct instance_t *unused)
{
    (void)unused;
    return current_connect_channel;
}

uint8_t *nd_mb_clientaddr_ptr(struct instance_t *unused)
{
    (void)unused;
    return client_addr;
}

uint32_t *nd_mb_term_auth_timeout_ptr(struct instance_t *unused)
{
    (void)unused;
    return &term_auth_timeout;
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
    return 1;
}

void mem_swap_cpy(void *unused, void *unused2, unsigned short unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
}

void ndv_memcopy(void *unused, void *unused2, uint16_t unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
}

int read_esam_info(struct instance_t *unused, uint8_t *unused2,
                   uint8_t unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

int esam_consult_key(struct instance_t *unused, uint8_t unused2, uint8_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
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

int MeterWr_obj_list_clear(struct instance_t *unused, struct pipeline_t *unused2,
                           nd_dl698_msg_t *unused3)
{
    (void)unused;
    (void)unused2;
    (void)unused3;
    return 0;
}

void setUp(void) {}

void tearDown(void) {}

static void test_698_connect_null_security_is_two_bytes(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[37] = 0;

    TEST_ASSERT_EQUAL_INT(39, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_password_security_uses_octet_length(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[37] = 1;
    ctx.apdu_d_rx[38] = 4;

    TEST_ASSERT_EQUAL_INT(43, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_symmetry_security_uses_two_octet_strings(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[37] = 2;
    ctx.apdu_d_rx[38] = 3;
    ctx.apdu_d_rx[42] = 2;

    TEST_ASSERT_EQUAL_INT(45, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

static void test_698_connect_rejects_unknown_security(void)
{
    struct context_698_t ctx;
    struct pipeline_t pipeline;

    memset(&ctx, 0, sizeof(ctx));
    pipeline.ctx = &ctx;
    ctx.apdu_d_rx[37] = 9;

    TEST_ASSERT_EQUAL_INT(-1, get_CONNECT_Request_len(NULL, &pipeline, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_698_connect_null_security_is_two_bytes);
    RUN_TEST(test_698_connect_password_security_uses_octet_length);
    RUN_TEST(test_698_connect_symmetry_security_uses_two_octet_strings);
    RUN_TEST(test_698_connect_rejects_unknown_security);
    return UNITY_END();
}
