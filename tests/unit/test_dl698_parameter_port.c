#include "unity.h"

#include "application/adapters/dl698/parameter_port.h"

#include <string.h>

static int read_calls;
static int write_calls;
static dl698_parameter_t last_parameter;
static uint8_t last_length;
static uint8_t last_write_value;

static int fake_read(void *ctx, dl698_parameter_t parameter, uint8_t *buffer,
                     size_t length)
{
    (void)ctx;
    read_calls++;
    last_parameter = parameter;
    last_length = (uint8_t)length;
    memset(buffer, 0x5a, length);
    return 0;
}

static int fake_write(void *ctx, dl698_parameter_t parameter,
                      const uint8_t *buffer, size_t length)
{
    (void)ctx;
    write_calls++;
    last_parameter = parameter;
    last_length = (uint8_t)length;
    last_write_value = buffer[0];
    return 0;
}

static int failing_read(void *ctx, dl698_parameter_t parameter, uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    (void)parameter;
    (void)buffer;
    (void)length;
    return -1;
}

static const nd_dl698_parameter_port_t fake_port = { NULL, fake_read, fake_write };
static const nd_dl698_parameter_port_t failing_port = { NULL, failing_read, NULL };

void setUp(void)
{
    read_calls = 0;
    write_calls = 0;
    last_parameter = DL698_PARAM_COUNT;
    last_length = 0;
    last_write_value = 0;
    dl698_bind_parameters(NULL);
}

void tearDown(void)
{
}

static void test_unbound_port_rejects_reads(void)
{
    uint8_t buffer[4] = { 0 };

    TEST_ASSERT_EQUAL_INT(-1, dl698_parameter_read(DL698_PARAM_SECURITY_MODE_EXDATA,
                                                   buffer, sizeof(buffer)));
}

static void test_bound_port_dispatches_key_and_length(void)
{
    uint8_t buffer[3] = { 0 };

    dl698_bind_parameters(&fake_port);
    TEST_ASSERT_EQUAL_INT(0, dl698_parameter_read(DL698_PARAM_SECURITY_MODE_EN,
                                                  buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(1, read_calls);
    TEST_ASSERT_EQUAL_INT(DL698_PARAM_SECURITY_MODE_EN, last_parameter);
    TEST_ASSERT_EQUAL_INT(3, last_length);
    TEST_ASSERT_EACH_EQUAL_UINT8(0x5a, buffer, sizeof(buffer));
}

static void test_failing_reader_propagates(void)
{
    uint8_t buffer[1] = { 0 };

    dl698_bind_parameters(&failing_port);
    TEST_ASSERT_EQUAL_INT(-1, dl698_parameter_read(DL698_PARAM_SECURITY_MODE_EXDATA,
                                                   buffer, sizeof(buffer)));
}

static void test_unbound_port_rejects_writes(void)
{
    const uint8_t value = 1;

    TEST_ASSERT_EQUAL_INT(-1, dl698_parameter_write(DL698_PARAM_REPORT_CHANNEL,
                                                    &value, 1));
}

static void test_read_only_port_rejects_writes(void)
{
    const uint8_t value = 1;

    dl698_bind_parameters(&failing_port);
    TEST_ASSERT_EQUAL_INT(-1, dl698_parameter_write(DL698_PARAM_REPORT_CHANNEL,
                                                    &value, 1));
}

static void test_bound_port_dispatches_writes(void)
{
    const uint8_t value = 0x42;

    dl698_bind_parameters(&fake_port);
    TEST_ASSERT_EQUAL_INT(0, dl698_parameter_write(DL698_PARAM_REPORT_ACTIVE_EN,
                                                   &value, 1));
    TEST_ASSERT_EQUAL_INT(1, write_calls);
    TEST_ASSERT_EQUAL_INT(DL698_PARAM_REPORT_ACTIVE_EN, last_parameter);
    TEST_ASSERT_EQUAL_INT(1, last_length);
    TEST_ASSERT_EQUAL_INT(0x42, last_write_value);
}

static void test_rebinding_replaces_the_port(void)
{
    uint8_t buffer[1] = { 0 };

    dl698_bind_parameters(&failing_port);
    dl698_bind_parameters(&fake_port);
    TEST_ASSERT_EQUAL_INT(0, dl698_parameter_read(DL698_PARAM_SECURITY_MODE_EXDATA,
                                                  buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(1, read_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unbound_port_rejects_reads);
    RUN_TEST(test_bound_port_dispatches_key_and_length);
    RUN_TEST(test_failing_reader_propagates);
    RUN_TEST(test_unbound_port_rejects_writes);
    RUN_TEST(test_read_only_port_rejects_writes);
    RUN_TEST(test_bound_port_dispatches_writes);
    RUN_TEST(test_rebinding_replaces_the_port);
    return UNITY_END();
}
