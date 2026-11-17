#include "unity.h"

#include "infra/drivers/storage/eeprom/eeprom.h"

static uint8_t memory[256];
static uint8_t last_device;
static uint16_t last_addr;
static int read_failures;
static int read_calls;
static int write_calls;
static uint16_t write_addr[8];
static uint16_t write_len[8];
static nd_eeprom_storage_t storage;
static nd_eeprom_t *dev;

static int fake_write(void *ctx, uint8_t device, uint16_t addr,
                      const uint8_t *buf, uint16_t len)
{
    (void)ctx;
    if (write_calls < 8) {
        write_addr[write_calls] = addr;
        write_len[write_calls] = len;
    }
    write_calls++;
    last_device = device;
    last_addr = addr;
    for (uint16_t i = 0; i < len; i++) {
        memory[addr + i] = buf[i];
    }
    return 0;
}

static int fake_read(void *ctx, uint8_t device, uint16_t addr, uint8_t *buf,
                     uint16_t len)
{
    (void)ctx;
    read_calls++;
    last_device = device;
    last_addr = addr;
    if (read_failures > 0) {
        read_failures--;
        return -1;
    }
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = memory[addr + i];
    }
    return 0;
}

static void fake_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    (void)us;
}

void setUp(void)
{
    static const nd_eeprom_config_t config = {
        .bus = {
            .ctx = NULL,
            .write = fake_write,
            .read = fake_read,
            .delay_us = fake_delay,
        },
        .device_address = 0xA0,
        .page_size = 8,
        .size = sizeof(memory),
    };

    for (size_t i = 0; i < sizeof(memory); i++) {
        memory[i] = 0xFF;
    }
    last_device = 0;
    last_addr = 0;
    read_failures = 0;
    read_calls = 0;
    write_calls = 0;
    dev = nd_eeprom_init(&storage, &config);
}

void tearDown(void) {}

static void test_init_rejects_invalid_configuration(void)
{
    static const nd_eeprom_config_t no_bus = {
        .device_address = 0xA0,
        .page_size = 8,
        .size = sizeof(memory),
    };
    static const nd_eeprom_config_t bad_page = {
        .bus = { .write = fake_write, .read = fake_read },
        .device_address = 0xA0,
        .page_size = 3,
        .size = sizeof(memory),
    };

    TEST_ASSERT_NULL(nd_eeprom_init(NULL, &no_bus));
    TEST_ASSERT_NULL(nd_eeprom_init(&storage, NULL));
    TEST_ASSERT_NULL(nd_eeprom_init(&storage, &no_bus));
    TEST_ASSERT_NULL(nd_eeprom_init(&storage, &bad_page));
}

static void test_read_sends_device_and_address(void)
{
    uint8_t value = 0;

    memory[0x10] = 0x5A;
    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_read(dev, 0x10, &value, 1));
    TEST_ASSERT_EQUAL_UINT8(0x5A, value);
    TEST_ASSERT_EQUAL_HEX8(0xA0, last_device);
    TEST_ASSERT_EQUAL_HEX16(0x10, last_addr);
}

static void test_read_retries_transient_failures(void)
{
    uint8_t value = 0;

    memory[0] = 0x42;
    read_failures = 2;
    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_read(dev, 0, &value, 1));
    TEST_ASSERT_EQUAL_UINT8(0x42, value);
    TEST_ASSERT_EQUAL_INT(3, read_calls);
}

static void test_read_rejects_out_of_range(void)
{
    uint8_t value = 0;

    TEST_ASSERT_EQUAL_INT(-1, nd_eeprom_read(dev, sizeof(memory), &value, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_eeprom_read(dev, 250, &value, 10));
    TEST_ASSERT_EQUAL_INT(-1, nd_eeprom_read(dev, 0, NULL, 1));
}

static void test_write_splits_pages(void)
{
    const uint8_t data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_write(dev, 6, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(2, write_calls);
    TEST_ASSERT_EQUAL_HEX16(6, write_addr[0]);
    TEST_ASSERT_EQUAL_UINT16(2, write_len[0]);
    TEST_ASSERT_EQUAL_HEX16(8, write_addr[1]);
    TEST_ASSERT_EQUAL_UINT16(8, write_len[1]);
    for (uint8_t i = 0; i < sizeof(data); i++) {
        TEST_ASSERT_EQUAL_UINT8(data[i], memory[6 + i]);
    }
}

static void test_write_skips_when_content_matches(void)
{
    const uint8_t data[4] = {9, 8, 7, 6};

    for (uint8_t i = 0; i < sizeof(data); i++) {
        memory[32 + i] = data[i];
    }
    TEST_ASSERT_EQUAL_INT(0, nd_eeprom_write(dev, 32, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(0, write_calls);
}

static void test_write_rejects_out_of_range(void)
{
    const uint8_t data[2] = {1, 2};

    TEST_ASSERT_EQUAL_INT(-1, nd_eeprom_write(dev, 255, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(0, write_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_configuration);
    RUN_TEST(test_read_sends_device_and_address);
    RUN_TEST(test_read_retries_transient_failures);
    RUN_TEST(test_read_rejects_out_of_range);
    RUN_TEST(test_write_splits_pages);
    RUN_TEST(test_write_skips_when_content_matches);
    RUN_TEST(test_write_rejects_out_of_range);
    return UNITY_END();
}
