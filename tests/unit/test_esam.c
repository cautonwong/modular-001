#include "unity.h"

#include <string.h>

#include "infra/drivers/security/esam/esam.h"

#define TX_MAX 64
#define REPLY_MAX 2000
#define LOG_MAX 8

static uint8_t tx_bytes[TX_MAX];
static uint16_t tx_count;
static uint8_t reply[REPLY_MAX];
static uint16_t reply_len;
static uint16_t reply_pos;
static uint16_t rx_calls;
static int power_down_at;
static int cs_level;
static int power_state;
static char call_log[LOG_MAX];
static uint16_t call_count;
static nd_esam_storage_t storage;
static nd_esam_t *dev;

static void log_call(char value)
{
    if (call_count < LOG_MAX) {
        call_log[call_count] = value;
    }
    call_count++;
}

static int fake_transfer(void *ctx, uint8_t tx, uint8_t *rx)
{
    (void)ctx;
    if (rx == NULL) {
        if (tx_count < TX_MAX) {
            tx_bytes[tx_count] = tx;
        }
        tx_count++;
        return 0;
    }
    if (power_down_at >= 0 && rx_calls == (uint16_t)power_down_at) {
        rx_calls++;
        return ND_ESAM_BUS_POWER_DOWN;
    }
    rx_calls++;
    *rx = reply_pos < reply_len ? reply[reply_pos++] : 0xFFu;
    return 0;
}

static void fake_cs(void *ctx, int high)
{
    (void)ctx;
    cs_level = high;
    log_call(high ? 'C' : 'c');
}

static void fake_power(void *ctx, int on)
{
    (void)ctx;
    power_state = on;
    log_call(on ? 'P' : 'p');
}

static void fake_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    (void)us;
}

static const nd_esam_config_t config = {
    .bus = {
        .ctx = NULL,
        .transfer = fake_transfer,
        .cs = fake_cs,
        .power = fake_power,
        .delay_us = fake_delay,
    },
};

void setUp(void)
{
    memset(tx_bytes, 0, sizeof(tx_bytes));
    memset(reply, 0, sizeof(reply));
    tx_count = 0;
    reply_len = 0;
    reply_pos = 0;
    rx_calls = 0;
    power_down_at = -1;
    cs_level = -1;
    power_state = -1;
    memset(call_log, 0, sizeof(call_log));
    call_count = 0;
    dev = nd_esam_init(&storage, &config);
}

void tearDown(void) {}

static void test_power_on_raises_cs_then_powers_the_rail(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_esam_power_on(dev));
    TEST_ASSERT_EQUAL_INT(1, cs_level);
    TEST_ASSERT_EQUAL_INT(1, power_state);
    TEST_ASSERT_EQUAL_INT(2, call_count);
    TEST_ASSERT_EQUAL_CHAR('C', call_log[0]);
    TEST_ASSERT_EQUAL_CHAR('P', call_log[1]);

    TEST_ASSERT_EQUAL_INT(0, nd_esam_power_off(dev));
    TEST_ASSERT_EQUAL_INT(0, power_state);
    TEST_ASSERT_EQUAL_INT(3, call_count);
    TEST_ASSERT_EQUAL_CHAR('p', call_log[2]);
}

static void test_transmit_encodes_start_header_length_and_lrc(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    static const uint8_t frame[10] = {
        0x55, 0x80, 0x36, 0x00, 0xFF, 0x00, 0x02, 0x01, 0x02, 0xB7,
    };
    uint8_t out[4] = {0};
    uint16_t len = 0;
    uint16_t i;

    reply[0] = 0x55;
    reply[1] = 0x90;
    reply[2] = 0x00;
    reply[3] = 0x00;
    reply[4] = 0x03;
    reply[5] = 0xAA;
    reply[6] = 0xBB;
    reply[7] = 0xCC;
    reply[8] = 0xB1;
    reply_len = 9;

    TEST_ASSERT_EQUAL_INT(0x9000,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_UINT16(10, tx_count);
    for (i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL_HEX8(frame[i], tx_bytes[i]);
    }
}

static void test_reply_payload_and_status_word(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    reply[0] = 0x55;
    reply[1] = 0x90;
    reply[2] = 0x00;
    reply[3] = 0x00;
    reply[4] = 0x03;
    reply[5] = 0xAA;
    reply[6] = 0xBB;
    reply[7] = 0xCC;
    reply[8] = 0xB1;
    reply_len = 9;

    TEST_ASSERT_EQUAL_INT(0x9000,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_UINT16(3, len);
    TEST_ASSERT_EQUAL_HEX8(0xAA, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, out[2]);
    TEST_ASSERT_EQUAL_INT(1, nd_esam_link_ok(dev));
    TEST_ASSERT_EQUAL_UINT16(9, rx_calls);
}

static void test_bad_lrc_still_reports_the_status_word(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    reply[0] = 0x55;
    reply[1] = 0x90;
    reply[2] = 0x00;
    reply[3] = 0x00;
    reply[4] = 0x02;
    reply[5] = 0x01;
    reply[6] = 0x02;
    reply[7] = 0x00;
    reply_len = 8;

    TEST_ASSERT_EQUAL_INT(0x9000,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_UINT16(2, len);
    TEST_ASSERT_EQUAL_INT(0, nd_esam_link_ok(dev));
}

static void test_unknown_header_is_not_accepted(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    reply[0] = 0x55;
    reply[1] = 0x12;
    reply[2] = 0x34;
    reply[3] = 0x00;
    reply[4] = 0x01;
    reply_len = 5;

    TEST_ASSERT_EQUAL_INT(0x1234,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_INT(0, nd_esam_link_ok(dev));
    TEST_ASSERT_EQUAL_UINT16(5, rx_calls);
}

static void test_silence_times_out_three_attempts(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_INT(0, nd_esam_link_ok(dev));
    TEST_ASSERT_EQUAL_UINT16(3 * 2000, rx_calls);
}

static void test_power_down_aborts_without_raising_cs(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    power_down_at = 0;
    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_INT(0, cs_level);
    TEST_ASSERT_EQUAL_INT(1, nd_esam_link_ok(dev));
}

static void test_power_down_during_payload_keeps_link_state(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    reply[0] = 0x55;
    reply[1] = 0x90;
    reply[2] = 0x00;
    reply[3] = 0x00;
    reply[4] = 0x02;
    reply_len = 5;
    power_down_at = 5;

    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_INT(0, cs_level);
    TEST_ASSERT_EQUAL_INT(1, nd_esam_link_ok(dev));
}

static void test_reply_length_is_clamped(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    static const uint8_t data[2] = {0x01, 0x02};
    static uint8_t out[ND_ESAM_MAX_APDU];
    uint16_t len = 0;

    reply[0] = 0x55;
    reply[1] = 0x90;
    reply[2] = 0x00;
    reply[3] = 0xFF;
    reply[4] = 0xFF;
    reply_len = 5;

    TEST_ASSERT_EQUAL_INT(0x9000,
                          nd_esam_transceive(dev, head, data, 2, out, &len));
    TEST_ASSERT_EQUAL_UINT16(ND_ESAM_MAX_APDU, len);
}

static void test_init_rejects_missing_callbacks(void)
{
    nd_esam_config_t broken = config;

    TEST_ASSERT_NULL(nd_esam_init(NULL, &config));
    TEST_ASSERT_NULL(nd_esam_init(&storage, NULL));

    broken.bus.transfer = NULL;
    TEST_ASSERT_NULL(nd_esam_init(&storage, &broken));
    broken = config;
    broken.bus.cs = NULL;
    TEST_ASSERT_NULL(nd_esam_init(&storage, &broken));
    broken = config;
    broken.bus.power = NULL;
    TEST_ASSERT_NULL(nd_esam_init(&storage, &broken));
    broken = config;
    broken.bus.delay_us = NULL;
    TEST_ASSERT_NULL(nd_esam_init(&storage, &broken));
}

static void test_transceive_rejects_bad_arguments(void)
{
    static const uint8_t head[4] = {0x80, 0x36, 0x00, 0xFF};
    uint8_t out[4] = {0};
    uint16_t len = 0;

    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(NULL, head, NULL, 0, out, &len));
    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, NULL, NULL, 0, out, &len));
    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, head, NULL, 2, out, &len));
    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, head, NULL, 0, NULL, &len));
    TEST_ASSERT_EQUAL_INT(0,
                          nd_esam_transceive(dev, head, NULL, 0, out, NULL));
    TEST_ASSERT_EQUAL_UINT16(0, tx_count);
    TEST_ASSERT_EQUAL_INT(-1, nd_esam_power_on(NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_esam_power_off(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_on_raises_cs_then_powers_the_rail);
    RUN_TEST(test_transmit_encodes_start_header_length_and_lrc);
    RUN_TEST(test_reply_payload_and_status_word);
    RUN_TEST(test_bad_lrc_still_reports_the_status_word);
    RUN_TEST(test_unknown_header_is_not_accepted);
    RUN_TEST(test_silence_times_out_three_attempts);
    RUN_TEST(test_power_down_aborts_without_raising_cs);
    RUN_TEST(test_power_down_during_payload_keeps_link_state);
    RUN_TEST(test_reply_length_is_clamped);
    RUN_TEST(test_init_rejects_missing_callbacks);
    RUN_TEST(test_transceive_rejects_bad_arguments);
    return UNITY_END();
}
