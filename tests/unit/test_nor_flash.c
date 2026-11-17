#include "unity.h"

#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"

static uint8_t memory[0x10000];
static uint8_t tx_log[512];
static uint16_t tx_log_len;
static int cs_low;
static int write_enabled;
static int program_pending;
static uint32_t program_addr;
static nd_mx25l3206e_storage_t storage;
static nd_mx25l3206e_t *dev;

static void log_tx(const uint8_t *tx, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len && tx_log_len < sizeof(tx_log); i++) {
        tx_log[tx_log_len++] = tx[i];
    }
}

static uint32_t command_address(const uint8_t *tx)
{
    return ((uint32_t)tx[1] << 16) | ((uint32_t)tx[2] << 8) | tx[3];
}

static int fake_select(void *ctx, int active)
{
    (void)ctx;
    cs_low = active;
    return 0;
}

static int fake_transfer(void *ctx, const uint8_t *tx, uint16_t tx_len,
                         uint8_t *rx, uint16_t rx_len)
{
    (void)ctx;
    if (!cs_low) {
        return -1;
    }
    if (program_pending) {
        uint16_t i;

        log_tx(tx, tx_len);
        for (i = 0; i < tx_len; i++) {
            memory[program_addr + i] = tx[i];
        }
        program_pending = 0;
        return 0;
    }
    if (tx == NULL || tx_len == 0) {
        return -1;
    }
    log_tx(tx, tx_len);
    switch (tx[0]) {
    case 0x06:
        write_enabled = 1;
        break;
    case 0x05:
        if (rx == NULL || rx_len < 1) {
            return -1;
        }
        rx[0] = 0;
        break;
    case 0x9F:
        if (rx == NULL || rx_len < 3) {
            return -1;
        }
        rx[0] = 0xC2;
        rx[1] = 0x20;
        rx[2] = 0x16;
        break;
    case 0x03: {
        uint32_t addr = command_address(tx);
        uint16_t i;

        if (rx == NULL) {
            return -1;
        }
        for (i = 0; i < rx_len; i++) {
            rx[i] = memory[addr + i];
        }
        break;
    }
    case 0x02:
        if (!write_enabled) {
            return -1;
        }
        write_enabled = 0;
        program_pending = 1;
        program_addr = command_address(tx);
        break;
    case 0x20: {
        uint32_t addr = command_address(tx) & ~(ND_MX25L3206E_SECTOR_SIZE - 1u);
        uint32_t i;

        for (i = 0; i < ND_MX25L3206E_SECTOR_SIZE; i++) {
            memory[addr + i] = 0xFF;
        }
        write_enabled = 0;
        break;
    }
    default:
        return -1;
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
    static const nd_mx25l3206e_config_t config = {
        .bus = {
            .ctx = NULL,
            .select = fake_select,
            .transfer = fake_transfer,
            .delay_us = fake_delay,
        },
        .size = sizeof(memory),
    };

    for (size_t i = 0; i < sizeof(memory); i++) {
        memory[i] = 0xFF;
    }
    tx_log_len = 0;
    cs_low = 0;
    write_enabled = 0;
    program_pending = 0;
    dev = nd_mx25l3206e_init(&storage, &config);
}

void tearDown(void) {}

static void assert_log(const uint8_t *expected, size_t len)
{
    TEST_ASSERT_EQUAL_UINT16(len, tx_log_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, tx_log, len);
}

static void test_init_rejects_invalid_configuration(void)
{
    static const nd_mx25l3206e_config_t no_bus = { .size = sizeof(memory) };
    static const nd_mx25l3206e_config_t no_select = {
        .bus = { .transfer = fake_transfer },
        .size = sizeof(memory),
    };
    static const nd_mx25l3206e_config_t no_size = {
        .bus = { .select = fake_select, .transfer = fake_transfer },
    };

    TEST_ASSERT_NULL(nd_mx25l3206e_init(NULL, &no_bus));
    TEST_ASSERT_NULL(nd_mx25l3206e_init(&storage, NULL));
    TEST_ASSERT_NULL(nd_mx25l3206e_init(&storage, &no_bus));
    TEST_ASSERT_NULL(nd_mx25l3206e_init(&storage, &no_select));
    TEST_ASSERT_NULL(nd_mx25l3206e_init(&storage, &no_size));
}

static void test_read_id_sends_identification_command(void)
{
    const uint8_t expected[] = {0x9F};
    uint8_t id[3] = {0};

    TEST_ASSERT_EQUAL_INT(0, nd_mx25l3206e_read_id(dev, id));
    TEST_ASSERT_EQUAL_HEX8(0xC2, id[0]);
    TEST_ASSERT_EQUAL_HEX8(0x20, id[1]);
    TEST_ASSERT_EQUAL_HEX8(0x16, id[2]);
    assert_log(expected, sizeof(expected));
}

static void test_read_sends_read_command_and_address(void)
{
    const uint8_t expected[] = {0x03, 0x00, 0x12, 0x34};
    uint8_t value[2] = {0};

    memory[0x1234] = 0x5A;
    memory[0x1235] = 0xA5;
    TEST_ASSERT_EQUAL_INT(0, nd_mx25l3206e_read(dev, 0x1234, value, 2));
    TEST_ASSERT_EQUAL_HEX8(0x5A, value[0]);
    TEST_ASSERT_EQUAL_HEX8(0xA5, value[1]);
    assert_log(expected, sizeof(expected));
}

static void test_write_splits_page_and_verifies(void)
{
    uint8_t data[16];
    uint8_t expected[38];
    uint16_t at = 0;

    for (uint8_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(0xA0 + i);
    }
    expected[at++] = 0x06;
    expected[at++] = 0x02;
    expected[at++] = 0x00;
    expected[at++] = 0x01;
    expected[at++] = 0xF8;
    for (uint8_t i = 0; i < 8; i++) {
        expected[at++] = data[i];
    }
    expected[at++] = 0x05;
    expected[at++] = 0x05;
    expected[at++] = 0x03;
    expected[at++] = 0x00;
    expected[at++] = 0x01;
    expected[at++] = 0xF8;
    expected[at++] = 0x06;
    expected[at++] = 0x02;
    expected[at++] = 0x00;
    expected[at++] = 0x02;
    expected[at++] = 0x00;
    for (uint8_t i = 8; i < 16; i++) {
        expected[at++] = data[i];
    }
    expected[at++] = 0x05;
    expected[at++] = 0x05;
    expected[at++] = 0x03;
    expected[at++] = 0x00;
    expected[at++] = 0x02;
    expected[at++] = 0x00;

    TEST_ASSERT_EQUAL_INT(0, nd_mx25l3206e_write(dev, 0x1F8, data,
                                                 sizeof(data)));
    assert_log(expected, at);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, memory + 0x1F8, sizeof(data));
}

static void test_erase_sends_write_enable_and_sector_command(void)
{
    const uint8_t expected[] = {0x06, 0x20, 0x00, 0x20, 0x00, 0x05, 0x05};
    size_t i;

    for (i = 0x2000; i < 0x3000; i++) {
        memory[i] = 0x11;
    }
    TEST_ASSERT_EQUAL_INT(0, nd_mx25l3206e_erase(dev, 0x2345, 16));
    assert_log(expected, sizeof(expected));
    for (i = 0x2000; i < 0x3000; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xFF, memory[i]);
    }
}

static void test_operations_reject_out_of_range(void)
{
    uint8_t value = 0;

    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_read(dev, sizeof(memory), &value, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_read(dev, 0xFFF0, &value, 32));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_write(dev, 0xFFF0, &value, 32));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_erase(dev, 0xFFF0, 32));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_erase(dev, 0, 0));
    TEST_ASSERT_EQUAL_UINT16(0, tx_log_len);
}

static void test_operations_reject_null(void)
{
    uint8_t value = 0;

    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_read(NULL, 0, &value, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_read(dev, 0, NULL, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_write(NULL, 0, &value, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_write(dev, 0, NULL, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_erase(NULL, 0, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_mx25l3206e_read_id(NULL, NULL));
    TEST_ASSERT_EQUAL_UINT16(0, tx_log_len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_configuration);
    RUN_TEST(test_read_id_sends_identification_command);
    RUN_TEST(test_read_sends_read_command_and_address);
    RUN_TEST(test_write_splits_page_and_verifies);
    RUN_TEST(test_erase_sends_write_enable_and_sector_command);
    RUN_TEST(test_operations_reject_out_of_range);
    RUN_TEST(test_operations_reject_null);
    return UNITY_END();
}
