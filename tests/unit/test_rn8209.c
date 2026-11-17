#include "unity.h"

#include <string.h>

#include "infra/drivers/metering/rn8209/rn8209.h"

#define MAX_FRAMES 24
#define MAX_FRAME 8

static uint32_t regs[0x80];
static uint8_t pending;
static uint8_t frames[MAX_FRAMES][MAX_FRAME];
static uint16_t frame_lens[MAX_FRAMES];
static int frame_count;
static int read_calls;
static int fail_reads;
static int fail_writes;
static int bus_power_down;
static int corrupt_checksum;
static int reset_calls;
static int reset_power_on;
static uint32_t device_id;
static nd_rn8209_storage_t storage;
static nd_rn8209_t *dev;

static int fake_write(void *ctx, const uint8_t *buf, uint16_t len,
                      int hold_tx)
{
    (void)ctx;
    (void)hold_tx;
    if (frame_count < MAX_FRAMES && len <= MAX_FRAME) {
        memcpy(frames[frame_count], buf, len);
        frame_lens[frame_count] = len;
        frame_count++;
    }
    if (bus_power_down) {
        return ND_RN8209_BUS_POWER_DOWN;
    }
    if (fail_writes > 0) {
        fail_writes--;
        return -1;
    }
    if (len == 1 && (buf[0] & 0x80u) == 0u) {
        pending = (uint8_t)(buf[0] & 0x7Fu);
        return 0;
    }
    if (len < 2) {
        return -1;
    }
    uint8_t sum = 0;
    for (uint16_t i = 0; i + 1 < len; i++) {
        sum = (uint8_t)(sum + buf[i]);
    }
    if ((uint8_t)~sum != buf[len - 1]) {
        return -1;
    }
    uint32_t value = 0;
    for (uint16_t i = 1; i + 1 < len; i++) {
        value = (value << 8) | buf[i];
    }
    regs[buf[0] & 0x7Fu] = value;
    return 0;
}

static int fake_read(void *ctx, uint8_t *buf, uint16_t len)
{
    (void)ctx;
    read_calls++;
    if (bus_power_down) {
        return ND_RN8209_BUS_POWER_DOWN;
    }
    if (fail_reads > 0) {
        fail_reads--;
        return -1;
    }
    uint16_t payload = (uint16_t)(len - 1u);
    if (payload == 0 || payload > 4) {
        return -1;
    }
    uint32_t value = regs[pending];
    if (pending == ND_RN8209_DEVICEID) {
        value = device_id;
    }
    for (uint16_t i = 0; i < payload; i++) {
        buf[i] = (uint8_t)(value >> (8u * (payload - 1u - i)));
    }
    uint8_t sum = pending;
    for (uint16_t i = 0; i < payload; i++) {
        sum = (uint8_t)(sum + buf[i]);
    }
    buf[payload] = corrupt_checksum ? (uint8_t)(~sum ^ 0xFFu) : (uint8_t)~sum;
    return 0;
}

static void fake_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    (void)us;
}

static void fake_reset(void *ctx, int power_on)
{
    (void)ctx;
    reset_calls++;
    reset_power_on = power_on;
}

static int find_frame(uint8_t reg, uint8_t value)
{
    for (int i = 0; i < frame_count; i++) {
        if (frame_lens[i] >= 3 && frames[i][0] == reg
            && frames[i][1] == value) {
            return i;
        }
    }
    return -1;
}

static const nd_rn8209_config_t config = {
    .bus = {
        .ctx = NULL,
        .write = fake_write,
        .read = fake_read,
        .delay_us = fake_delay,
        .reset = fake_reset,
    },
};

void setUp(void)
{
    memset(regs, 0, sizeof(regs));
    regs[ND_RN8209_SYSCON] = 0x1643;
    regs[ND_RN8209_EMUCON] = 0x0001;
    regs[ND_RN8209_HFCONST] = 0x0DB4;
    pending = 0;
    frame_count = 0;
    read_calls = 0;
    fail_reads = 0;
    fail_writes = 0;
    bus_power_down = 0;
    corrupt_checksum = 0;
    reset_calls = 0;
    reset_power_on = -1;
    device_id = ND_RN8209_DEVICE_ID;
    dev = nd_rn8209_init(&storage, &config);
}

void tearDown(void) {}

static void test_read_requests_register_and_decodes_payload(void)
{
    uint8_t value[3] = {0};

    regs[ND_RN8209_URMS] = 0x563412;
    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_read(dev, ND_RN8209_URMS, value, 3));
    TEST_ASSERT_EQUAL_INT(1, frame_count);
    TEST_ASSERT_EQUAL_UINT16(1, frame_lens[0]);
    TEST_ASSERT_EQUAL_HEX8(ND_RN8209_URMS, frames[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, value[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, value[1]);
    TEST_ASSERT_EQUAL_HEX8(0x56, value[2]);
}

static void test_write_encodes_frame_and_checksum(void)
{
    uint16_t value = 0x1234;

    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_write(dev, ND_RN8209_HFCONST,
                                             &value, 2));
    TEST_ASSERT_EQUAL_INT(2, frame_count);
    TEST_ASSERT_EQUAL_UINT16(4, frame_lens[0]);
    TEST_ASSERT_EQUAL_HEX8(0x82, frames[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x12, frames[0][1]);
    TEST_ASSERT_EQUAL_HEX8(0x34, frames[0][2]);
    TEST_ASSERT_EQUAL_HEX8(0x37, frames[0][3]);
    TEST_ASSERT_EQUAL_UINT16(1, frame_lens[1]);
    TEST_ASSERT_EQUAL_HEX8(ND_RN8209_HFCONST, frames[1][0]);
    TEST_ASSERT_EQUAL_HEX32(0x1234, regs[ND_RN8209_HFCONST]);
}

static void test_read_retries_bad_checksum_three_times(void)
{
    uint16_t value = 0;

    corrupt_checksum = 1;
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_read(dev, ND_RN8209_EMUCON,
                                             &value, 2));
    TEST_ASSERT_EQUAL_INT(3, read_calls);
    TEST_ASSERT_EQUAL_INT(3, frame_count);
}

static void test_read_stops_on_bus_power_down(void)
{
    uint16_t value = 0;

    bus_power_down = 1;
    TEST_ASSERT_EQUAL_INT(ND_RN8209_BUS_POWER_DOWN,
                          nd_rn8209_read(dev, ND_RN8209_EMUCON, &value, 2));
    TEST_ASSERT_EQUAL_INT(0, read_calls);
}

static void test_write_retries_ten_times(void)
{
    uint16_t value = 0x1234;

    fail_writes = 100;
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_write(dev, ND_RN8209_HFCONST,
                                              &value, 2));
    TEST_ASSERT_EQUAL_INT(10, frame_count);
    TEST_ASSERT_EQUAL_INT(0, read_calls);
}

static void test_rejects_bad_arguments(void)
{
    uint8_t value[4] = {0};

    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_read(NULL, ND_RN8209_URMS, value, 2));
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_read(dev, ND_RN8209_URMS, NULL, 2));
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_read(dev, ND_RN8209_URMS, value, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_read(dev, ND_RN8209_URMS, value, 5));
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_write(dev, ND_RN8209_GPQA, NULL, 2));
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_write(dev, ND_RN8209_GPQA, value, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_command(NULL, 0x02));
    TEST_ASSERT_NULL(nd_rn8209_init(NULL, &config));
    TEST_ASSERT_NULL(nd_rn8209_init(&storage, NULL));
    TEST_ASSERT_EQUAL_INT(0, frame_count);
}

static void test_power_on_resets_and_configures_chip(void)
{
    nd_rn8209_control_param_t param = {
        .pstart = 0x004C,
        .hfconst = 0x0DB4,
        .gain = 0x1234,
    };

    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_control(dev,
                                               ND_RN8209_CONTROL_POWER_ON,
                                               &param));
    TEST_ASSERT_EQUAL_INT(1, reset_calls);
    TEST_ASSERT_EQUAL_INT(1, reset_power_on);
    TEST_ASSERT_EQUAL_HEX32(0x1643, regs[ND_RN8209_SYSCON]);
    TEST_ASSERT_EQUAL_HEX32(0x0001, regs[ND_RN8209_EMUCON]);
    TEST_ASSERT_EQUAL_HEX32(0x0DB4, regs[ND_RN8209_HFCONST]);
    TEST_ASSERT_EQUAL_HEX32(0x004C, regs[ND_RN8209_PSTART]);
    TEST_ASSERT_EQUAL_HEX32(0x01, regs[ND_RN8209_CIE]);
    TEST_ASSERT_TRUE(find_frame(0xEA, ND_RN8209_CMD_WR_EN) >= 0);
    TEST_ASSERT_TRUE(find_frame(0xEA, ND_RN8209_CMD_WR_DIS) >= 0);
}

static void test_power_on_rejects_missing_chip(void)
{
    nd_rn8209_control_param_t param = {
        .pstart = 0x004C,
        .hfconst = 0x0DB4,
        .gain = 0,
    };

    device_id = 0;
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_control(dev,
                                                ND_RN8209_CONTROL_POWER_ON,
                                                &param));
    TEST_ASSERT_EQUAL_INT(1, reset_calls);
    TEST_ASSERT_EQUAL_HEX32(0, regs[ND_RN8209_PSTART]);
    TEST_ASSERT_TRUE(find_frame(0xEA, ND_RN8209_CMD_WR_EN) < 0);
}

static void test_power_off_only_resets(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_control(dev,
                                               ND_RN8209_CONTROL_POWER_OFF,
                                               NULL));
    TEST_ASSERT_EQUAL_INT(1, reset_calls);
    TEST_ASSERT_EQUAL_INT(0, reset_power_on);
    TEST_ASSERT_EQUAL_INT(0, frame_count);
}

static void test_check_compares_expected_registers(void)
{
    nd_rn8209_control_param_t param = {
        .pstart = 0,
        .hfconst = 0x0DB4,
        .gain = 0x1234,
    };

    regs[ND_RN8209_GPQA] = 0x1234;
    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_control(dev,
                                               ND_RN8209_CONTROL_CHECK,
                                               &param));
    regs[ND_RN8209_GPQA] = 0x0001;
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_control(dev,
                                                ND_RN8209_CONTROL_CHECK,
                                                &param));
}

static void test_check_treats_power_down_as_healthy(void)
{
    nd_rn8209_control_param_t param = {
        .pstart = 0,
        .hfconst = 0x0DB4,
        .gain = 0,
    };

    bus_power_down = 1;
    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_control(dev,
                                               ND_RN8209_CONTROL_CHECK,
                                               &param));
    TEST_ASSERT_EQUAL_INT(0, read_calls);
}

static void test_read_id_validates_device_id(void)
{
    uint32_t id = 0;

    TEST_ASSERT_EQUAL_INT(0, nd_rn8209_read_id(dev, &id));
    TEST_ASSERT_EQUAL_HEX32(ND_RN8209_DEVICE_ID, id);

    device_id = 0x00820901;
    TEST_ASSERT_EQUAL_INT(-1, nd_rn8209_read_id(dev, &id));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_read_requests_register_and_decodes_payload);
    RUN_TEST(test_write_encodes_frame_and_checksum);
    RUN_TEST(test_read_retries_bad_checksum_three_times);
    RUN_TEST(test_read_stops_on_bus_power_down);
    RUN_TEST(test_write_retries_ten_times);
    RUN_TEST(test_rejects_bad_arguments);
    RUN_TEST(test_power_on_resets_and_configures_chip);
    RUN_TEST(test_power_on_rejects_missing_chip);
    RUN_TEST(test_power_off_only_resets);
    RUN_TEST(test_check_compares_expected_registers);
    RUN_TEST(test_check_treats_power_down_as_healthy);
    RUN_TEST(test_read_id_validates_device_id);
    return UNITY_END();
}
