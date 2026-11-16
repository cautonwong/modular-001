#include "unity.h"

#include "application/ports/record_storage.h"

static uint32_t saved_oad;
static uint32_t saved_index;
static uint32_t saved_oad_req;
static uint32_t saved_oad_src;
static int saved_mode;
static uint32_t saved_di;
static uint8_t saved_tag;

static int fake_save_index(void *ctx, uint32_t oad, uint32_t *index)
{
    (void)ctx;
    saved_oad = oad;
    *index = 42;
    return 0;
}

static int fake_change_format(void *ctx, uint8_t *des, const uint8_t *src,
                              uint32_t oad_req, uint32_t oad_src, int mode)
{
    (void)ctx;
    (void)des;
    (void)src;
    saved_oad_req = oad_req;
    saved_oad_src = oad_src;
    saved_mode = mode;
    return 7;
}

static int fake_clear_program_list(void *ctx, void *pipeline, void *mesg)
{
    (void)ctx;
    (void)pipeline;
    (void)mesg;
    return 0;
}

static int fake_trigger_645_record(void *ctx, uint32_t di, uint8_t tag)
{
    (void)ctx;
    saved_di = di;
    saved_tag = tag;
    return 0;
}

static const nd_record_storage_port_t fake_port = {
    .ctx = NULL,
    .save_index = fake_save_index,
    .change_format = fake_change_format,
    .clear_program_list = fake_clear_program_list,
    .trigger_645_record = fake_trigger_645_record,
};

void setUp(void)
{
    nd_record_storage_bind(NULL);
    saved_oad = saved_index = saved_oad_req = saved_oad_src = 0;
    saved_mode = 0;
    saved_di = 0;
    saved_tag = 0;
}

void tearDown(void) {}

static void test_unbound_port_returns_error(void)
{
    uint32_t index = 0;
    uint8_t buffer[4] = {0};
    TEST_ASSERT_EQUAL_INT(-1, nd_record_storage_save_index(1, &index));
    TEST_ASSERT_EQUAL_INT(-1, nd_record_storage_change_format(
                                 buffer, buffer, 1, 2, ND_RECORD_READ));
    TEST_ASSERT_EQUAL_INT(-1, nd_record_storage_clear_program_list(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_record_storage_trigger_645_record(1, 0));
}

static void test_bound_port_dispatches(void)
{
    uint32_t index = 0;
    uint8_t buffer[4] = {0};
    nd_record_storage_bind(&fake_port);

    TEST_ASSERT_EQUAL_INT(0, nd_record_storage_save_index(0x1234, &index));
    TEST_ASSERT_EQUAL_UINT32(0x1234, saved_oad);
    TEST_ASSERT_EQUAL_UINT32(42, index);

    TEST_ASSERT_EQUAL_INT(7, nd_record_storage_change_format(
                                 buffer, buffer, 0x3000, 0x2000,
                                 ND_RECORD_GET_STORE_LEN));
    TEST_ASSERT_EQUAL_UINT32(0x3000, saved_oad_req);
    TEST_ASSERT_EQUAL_UINT32(0x2000, saved_oad_src);
    TEST_ASSERT_EQUAL_INT(ND_RECORD_GET_STORE_LEN, saved_mode);

    TEST_ASSERT_EQUAL_INT(0,
                          nd_record_storage_clear_program_list(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(0, nd_record_storage_trigger_645_record(0x04000401, 3));
    TEST_ASSERT_EQUAL_UINT32(0x04000401, saved_di);
    TEST_ASSERT_EQUAL_UINT8(3, saved_tag);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_port_returns_error);
    RUN_TEST(test_bound_port_dispatches);
    return UNITY_END();
}
