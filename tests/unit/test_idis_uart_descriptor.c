#include "unity.h"

#include <stdbool.h>
#include <string.h>

#include "cpu_surport.h"
#include "components/core/fifo/fifo.h"
#include "infra/soc/v85xxp/v8500/time_link.h"
#include "targets/idis_ip_meter/board/board_state.h"
#include "targets/idis_ip_meter/board/uart_driver.h"

static int timer_created;
static void *timer_para;
static uint8_t (*timer_func)(void *);
static uint8_t timer_id;
static uint32_t timer_timeout;
static uint8_t timer_stoped;

static int isr_calls;
static uint8_t isr_irqs[8];
static void *isr_paras[8];

static int rx_byte_ready;
static uint8_t rx_byte;
static int tx_sent_calls;
static uint8_t tx_sent_byte;
static size_t tx_sent_index;
static int rs485_disable_calls;
static int mcu_init_calls;
static size_t mcu_init_index;
static int send_enable_calls;

uint8_t ndv_timer_create(struct tm_list_init_t *tm)
{
    timer_created++;
    timer_para = tm->tickpara;
    timer_func = tm->tickfunc;
    timer_id = tm->tm_id;
    timer_timeout = tm->timeout;
    timer_stoped = tm->stoped;
    return true;
}

uint8_t Isr_handle_regsit(struct isr_reg_ctx_t *reg, uint8_t irq)
{
    if (isr_calls < 8) {
        isr_irqs[isr_calls] = irq;
        isr_paras[isr_calls] = reg->para;
    }
    isr_calls++;
    return true;
}

int mcu_uart_recive_byte(uint8_t *byte, size_t index)
{
    (void)index;
    if (!rx_byte_ready) {
        return 0;
    }
    *byte = rx_byte;
    return 1;
}

int mcu_uart_send_byte(uint8_t byte, size_t index)
{
    tx_sent_calls++;
    tx_sent_byte = byte;
    tx_sent_index = index;
    return 1;
}

void mcu_uart_send_enable(uint8_t enable, size_t index)
{
    (void)enable;
    (void)index;
    send_enable_calls++;
}

void mcu_uart_recive_enable(uint8_t enable, size_t index)
{
    (void)enable;
    (void)index;
}

uint8_t mcu_uart_init(void *config, uint8_t index, uint8_t infrared)
{
    (void)config;
    (void)infrared;
    mcu_init_calls++;
    mcu_init_index = index;
    return 1;
}

uint8_t mcu_uart_close(uint8_t index)
{
    (void)index;
    return 0;
}

int mcu_uart_get_tx_status(size_t index)
{
    (void)index;
    return 0;
}

int mcu_uart_get_rx_status(size_t index)
{
    (void)index;
    return 0;
}

void mcu_clear_watchdog(void)
{
}

void nd_uart_rs485_receiver_enable(void)
{
}

void nd_uart_rs485_receiver_disable(void)
{
    rs485_disable_calls++;
}

void nd_uart_rs485_rx_input(void)
{
}

void nd_uart_rs485_tx_output(void)
{
}

void nd_uart_rs485_tx_set(int high)
{
    (void)high;
}

void setUp(void)
{
    timer_created = 0;
    timer_para = NULL;
    timer_func = NULL;
    timer_id = 0;
    timer_timeout = 0;
    timer_stoped = 0;
    isr_calls = 0;
    memset(isr_irqs, 0, sizeof(isr_irqs));
    memset(isr_paras, 0, sizeof(isr_paras));
    rx_byte_ready = 0;
    rx_byte = 0;
    tx_sent_calls = 0;
    tx_sent_byte = 0;
    tx_sent_index = 0;
    rs485_disable_calls = 0;
    mcu_init_calls = 0;
    mcu_init_index = 0;
    send_enable_calls = 0;
}

void tearDown(void) {}

static void init_fifos(struct fifo_t *tx, uint8_t *tx_buf,
                       struct fifo_t *rx, uint8_t *rx_buf)
{
    fifoBuf_init(tx, tx_buf, 16);
    fifoBuf_init(rx, rx_buf, 16);
}

static void init_descriptor(nd_idis_board_state_t *state, uint8_t index,
                            struct fifo_t *tx, struct fifo_t *rx)
{
    struct uart_chl_init_t init = {
        .tx_fifo = tx,
        .rx_fifo = rx,
        .uart_cfg = NULL,
        .mcu_uart_index = index,
        .chl_type = UART_MODULE_CHL,
        .CallBackReceiveTimeOut = NULL,
        .CallBackSendTimeOut = NULL,
    };

    TEST_ASSERT_EQUAL_INT(1, uart_chl_initialize(state, &init));
}

static void test_initialize_binds_descriptor_to_timer_and_isr(void)
{
    nd_idis_board_state_t state;
    struct fifo_t tx;
    struct fifo_t rx;
    uint8_t tx_buf[16];
    uint8_t rx_buf[16];

    memset(&state, 0, sizeof(state));
    init_fifos(&tx, tx_buf, &rx, rx_buf);
    init_descriptor(&state, 3, &tx, &rx);

    TEST_ASSERT_EQUAL_UINT8(3, state.uart_dev[3].mcu_uart_index);
    TEST_ASSERT_EQUAL_PTR(&tx, state.uart_dev[3].tx_fifo);
    TEST_ASSERT_EQUAL_PTR(&rx, state.uart_dev[3].rx_fifo);
    TEST_ASSERT_EQUAL_UINT32(0, state.uart_dev[3].rx_time_out);
    TEST_ASSERT_EQUAL_UINT32(0, state.uart_dev[3].tx_time_out);
    TEST_ASSERT_EQUAL_INT(1, timer_created);
    TEST_ASSERT_EQUAL_PTR(&state.uart_dev[3], timer_para);
    TEST_ASSERT_EQUAL_PTR(uart_time_process, timer_func);
    TEST_ASSERT_EQUAL_UINT8(MODULE_TM, timer_id);
    TEST_ASSERT_EQUAL_UINT32(1, timer_timeout);
    TEST_ASSERT_EQUAL_UINT8(0, timer_stoped);
    TEST_ASSERT_EQUAL_INT(2, isr_calls);
    TEST_ASSERT_EQUAL_UINT8(IRQ_UART0_TX + (3 << 1), isr_irqs[0]);
    TEST_ASSERT_EQUAL_UINT8(IRQ_UART0_RX + (3 << 1), isr_irqs[1]);
    TEST_ASSERT_EQUAL_PTR(&state.uart_dev[3], isr_paras[0]);
    TEST_ASSERT_EQUAL_PTR(&state.uart_dev[3], isr_paras[1]);
}

static void test_receive_isr_uses_descriptor(void)
{
    nd_idis_board_state_t state;
    struct fifo_t tx;
    struct fifo_t rx;
    uint8_t tx_buf[16];
    uint8_t rx_buf[16];

    memset(&state, 0, sizeof(state));
    init_fifos(&tx, tx_buf, &rx, rx_buf);
    init_descriptor(&state, 2, &tx, &rx);
    rx_byte_ready = 1;
    rx_byte = 0x5a;

    UartReceiveHandle(&state.uart_dev[2]);
    TEST_ASSERT_EQUAL_UINT16(1, fifoBuf_getUsed(&rx));
    TEST_ASSERT_EQUAL_HEX8(0x5a, fifoBuf_getByte(&rx));
    TEST_ASSERT_EQUAL_UINT32(5, state.uart_dev[2].rx_time_out);

    UartReceiveHandle(NULL);
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&rx));
}

static void test_timer_uses_descriptor(void)
{
    nd_idis_board_state_t state;
    struct fifo_t tx;
    struct fifo_t rx;
    uint8_t tx_buf[16];
    uint8_t rx_buf[16];

    memset(&state, 0, sizeof(state));
    init_fifos(&tx, tx_buf, &rx, rx_buf);
    init_descriptor(&state, 1, &tx, &rx);
    state.uart_dev[1].chl_type = UART_RS4851_CHL;
    state.uart_dev[1].tx_time_out = 0;
    TEST_ASSERT_EQUAL_UINT16(1, fifoBuf_putByte(&tx, 0x41));

    TEST_ASSERT_EQUAL_INT(1, uart_time_process(&state.uart_dev[1]));
    TEST_ASSERT_EQUAL_INT(1, tx_sent_calls);
    TEST_ASSERT_EQUAL_HEX8(0x41, tx_sent_byte);
    TEST_ASSERT_EQUAL_UINT32(1, tx_sent_index);
    TEST_ASSERT_EQUAL_UINT32(3, state.uart_dev[1].tx_time_out);
    TEST_ASSERT_EQUAL_INT(1, rs485_disable_calls);
    TEST_ASSERT_EQUAL_UINT16(0, fifoBuf_getUsed(&tx));
}

static void test_open_uses_root_state(void)
{
    nd_idis_board_state_t state;
    struct fifo_t tx;
    struct fifo_t rx;
    uint8_t tx_buf[16];
    uint8_t rx_buf[16];

    memset(&state, 0, sizeof(state));
    init_fifos(&tx, tx_buf, &rx, rx_buf);
    init_descriptor(&state, 4, &tx, &rx);
    mcu_init_calls = 0;
    TEST_ASSERT_EQUAL_INT(1, uart_chl_open(&state, UART_MODULE_CHL));
    TEST_ASSERT_EQUAL_INT(1, mcu_init_calls);
    TEST_ASSERT_EQUAL_UINT32(4, mcu_init_index);
    TEST_ASSERT_EQUAL_INT(0, uart_chl_open(NULL, UART_MODULE_CHL));
}

static void test_initialize_without_state_fails(void)
{
    struct uart_chl_init_t init;

    memset(&init, 0, sizeof(init));
    TEST_ASSERT_EQUAL_INT(0, uart_chl_initialize(NULL, &init));
    TEST_ASSERT_EQUAL_INT(0, uart_chl_open(NULL, UART_MODULE_CHL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_initialize_binds_descriptor_to_timer_and_isr);
    RUN_TEST(test_receive_isr_uses_descriptor);
    RUN_TEST(test_timer_uses_descriptor);
    RUN_TEST(test_open_uses_root_state);
    RUN_TEST(test_initialize_without_state_fails);
    return UNITY_END();
}
