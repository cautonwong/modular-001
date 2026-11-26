#include "unity.h"

#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "cpu_surport.h"
#include "lib_pmu.h"
#include "targets/idis_ip_meter/app/board_ports.h"
#include "targets/idis_ip_meter/board/board_eeprom.h"
#include "targets/idis_ip_meter/board/board_flash.h"
#include "targets/idis_ip_meter/board/board_rn8209.h"
#include "targets/idis_ip_meter/board/relay_pins.h"
#include "targets/idis_ip_meter/board/spi_driver.h"
#include "targets/idis_ip_meter/board/uart_driver.h"

#define FLASH_BYTES 4096u
#define REGISTER_WINDOW_BASE ((void *)0x40000000u)
#define REGISTER_WINDOW_BYTES (1024u * 1024u)

static uint8_t flash_mem[FLASH_BYTES];
static uint8_t flash_opcode;
static uint32_t flash_addr;
static int flash_pp_active;
static int chip_selected;

static void reset_device(void)
{
    memset(flash_mem, 0xFF, sizeof(flash_mem));
    flash_opcode = 0;
    flash_addr = 0;
    flash_pp_active = 0;
    chip_selected = 0;
}

void nd_board_spi_bind(nd_idis_board_state_t *state)
{
    (void)state;
}

void nd_board_spi_init(nd_board_spi_t bus)
{
    (void)bus;
}

void nd_board_spi_select(nd_idis_board_state_t *state, nd_board_spi_t bus,
                         int active)
{
    (void)state;
    (void)bus;
    chip_selected = active;
    if (!active) {
        flash_pp_active = 0;
    }
}

int nd_board_spi_transfer(nd_idis_board_state_t *state, nd_board_spi_t bus,
                          const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    (void)state;
    (void)bus;
    if (tx != NULL) {
        if (flash_pp_active) {
            for (uint16_t i = 0; i < len; i++) {
                if (flash_addr < FLASH_BYTES) {
                    flash_mem[flash_addr] = tx[i];
                }
                flash_addr++;
            }
            return 0;
        }
        flash_opcode = tx[0];
        flash_pp_active = 0;
        if (len >= 4
            && (flash_opcode == 0x03 || flash_opcode == 0x02
                || flash_opcode == 0x20)) {
            flash_addr = ((uint32_t)tx[1] << 16) | ((uint32_t)tx[2] << 8)
                         | (uint32_t)tx[3];
            if (flash_opcode == 0x02) {
                flash_pp_active = 1;
            } else if (flash_opcode == 0x20) {
                for (uint32_t i = 0; i < FLASH_BYTES
                                     && flash_addr + i < FLASH_BYTES; i++) {
                    flash_mem[flash_addr + i] = 0xFFu;
                }
            }
        }
        return 0;
    }
    if (rx != NULL) {
        if (flash_opcode == 0x03) {
            for (uint16_t i = 0; i < len; i++) {
                rx[i] = flash_addr < FLASH_BYTES ? flash_mem[flash_addr]
                                                 : 0xFFu;
                flash_addr++;
            }
        } else if (flash_opcode == 0x9F && len >= 3) {
            rx[0] = 0xC2;
            rx[1] = 0x20;
            rx[2] = 0x16;
        } else {
            memset(rx, 0, len);
        }
    }
    return 0;
}

void nd_spi_flash_power_set(int on)
{
    (void)on;
}

nd_eeprom_t *board_eeprom(struct nd_idis_board_state *state)
{
    (void)state;
    return NULL;
}

static nd_rn8209_storage_t rn8209_storage;
static nd_rn8209_t *rn8209_device;
static nd_idis_board_state_t *rn8209_state_seen;
static char rn8209_bus_marker;
static uint8_t rn8209_register;

static int rn8209_bus_write(void *ctx, const uint8_t *buf, uint16_t len,
                            int hold_tx)
{
    (void)ctx;
    (void)hold_tx;
    if (len > 0) {
        rn8209_register = buf[0];
    }
    return 0;
}

static int rn8209_bus_read(void *ctx, uint8_t *buf, uint16_t len)
{
    uint8_t sum = rn8209_register;
    uint16_t i;

    (void)ctx;
    for (i = 0; i + 1u < len; i++) {
        buf[i] = 0;
    }
    buf[len - 1u] = (uint8_t)~sum;
    return 0;
}

static void rn8209_bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    (void)us;
}

static void rn8209_bus_reset(void *ctx, int power_on)
{
    (void)ctx;
    (void)power_on;
}

void board_rn8209_init(nd_idis_board_state_t *state)
{
    (void)state;
}

nd_rn8209_t *board_rn8209(nd_idis_board_state_t *state)
{
    rn8209_state_seen = state;
    return rn8209_device;
}

void nd_relay_on_pulse(int active)
{
    (void)active;
}

void nd_relay_off_pulse(int active)
{
    (void)active;
}

int uart_chl_initialize(nd_idis_board_state_t *state,
                        struct uart_chl_init_t *uart_chl)
{
    (void)state;
    (void)uart_chl;
    return 0;
}

nd_idis_calibration_status_t
nd_idis_calibration_load(nd_idis_calibration_record_t *record,
                         const nd_store_t *store)
{
    (void)record;
    (void)store;
    return ND_IDIS_CALIBRATION_NONE;
}

static struct isr_reg_ctx_t registered_isr;
static int register_calls;

uint8_t Isr_handle_regsit(struct isr_reg_ctx_t *isrRegctx, uint8_t irq)
{
    (void)irq;
    if (isrRegctx != NULL) {
        registered_isr = *isrRegctx;
    }
    register_calls++;
    return 0;
}

void PMU_INTConfig(uint32_t INTMask, uint32_t NewState)
{
    (void)INTMask;
    (void)NewState;
}

static uint16_t pmu_int_status;

uint16_t PMU_GetIOAINTStatus(uint16_t INTMask)
{
    (void)INTMask;
    return pmu_int_status;
}

void PMU_ClearIOAINTStatus(uint16_t INTMask)
{
    (void)INTMask;
}

void PMU_WakeUpPinConfig(uint32_t IOAx, uint32_t Wakeup_Event)
{
    (void)IOAx;
    (void)Wakeup_Event;
}

uint8_t read_mcu_rtc(uint8_t addr, struct tm *time)
{
    (void)addr;
    (void)time;
    return 0;
}

uint8_t mcu_adc_start_once(uint8_t chl_type)
{
    (void)chl_type;
    return 0;
}

uint32_t read_mcu_adc_measure_value(uint8_t chl_type, int32_t *adc_measure)
{
    (void)chl_type;
    (void)adc_measure;
    return 0;
}

void mcu_clear_watchdog(void)
{
}

uint8_t mcu_delay_us(volatile uint16_t count)
{
    (void)count;
    return 0;
}

static void reset_measurement_device(void)
{
    nd_rn8209_config_t config = {
        .bus = {
            .ctx = &rn8209_bus_marker,
            .write = rn8209_bus_write,
            .read = rn8209_bus_read,
            .delay_us = rn8209_bus_delay,
            .reset = rn8209_bus_reset,
        },
    };

    rn8209_register = 0;
    rn8209_state_seen = NULL;
    rn8209_device = nd_rn8209_init(&rn8209_storage, &config);
}

static int map_registers(void)
{
    static int mapped;
    void *base;

    if (mapped) {
        return 0;
    }
    base = mmap(REGISTER_WINDOW_BASE, REGISTER_WINDOW_BYTES,
                PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        return -1;
    }
    if (base != REGISTER_WINDOW_BASE) {
        munmap(base, REGISTER_WINDOW_BYTES);
        return -1;
    }
    mapped = 1;
    return 0;
}

void setUp(void)
{
    reset_device();
    reset_measurement_device();
    register_calls = 0;
    pmu_int_status = 0;
    memset(&registered_isr, 0, sizeof(registered_isr));
}

void tearDown(void) {}

static void test_null_state_is_rejected(void)
{
    TEST_ASSERT_NULL(board_flash(NULL));
    TEST_ASSERT_NULL(board_flash_port(NULL));
    board_flash_init(NULL);
}

static void test_lazy_init_binds_state_storage(void)
{
    nd_idis_board_state_t state;
    nd_mx25l3206e_t *flash;

    memset(&state, 0, sizeof(state));
    flash = board_flash(&state);
    TEST_ASSERT_NOT_NULL(flash);
    TEST_ASSERT_EQUAL_PTR(flash, state.flash);
    TEST_ASSERT_EQUAL_PTR(&state.flash_storage, (void *)flash);
    TEST_ASSERT_EQUAL_PTR(flash, board_flash(&state));
}

static void test_two_states_have_independent_flash(void)
{
    nd_idis_board_state_t first;
    nd_idis_board_state_t second;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    TEST_ASSERT_TRUE(board_flash(&first) != board_flash(&second));
    TEST_ASSERT_EQUAL_PTR(&first.flash_storage, (void *)first.flash);
    TEST_ASSERT_EQUAL_PTR(&second.flash_storage, (void *)second.flash);
}

static void test_store_port_is_bound_to_state(void)
{
    nd_idis_board_state_t state;

    memset(&state, 0, sizeof(state));
    board_flash_init(&state);
    TEST_ASSERT_NOT_NULL(state.flash);
    TEST_ASSERT_EQUAL_PTR(nd_sector_store_port(&state.flash_store),
                          board_flash_port(&state));
}

static void test_store_read_write_traverse_the_bound_state(void)
{
    nd_idis_board_state_t state;
    nd_store_t *port;
    uint8_t first[4] = { 1, 2, 3, 4 };
    uint8_t second[4] = { 9, 8, 7, 6 };
    uint8_t readback[4] = { 0 };

    memset(&state, 0, sizeof(state));
    board_flash_init(&state);
    port = board_flash_port(&state);
    TEST_ASSERT_NOT_NULL(port);

    TEST_ASSERT_EQUAL_INT(0, nd_store_write(port, 0, first, sizeof(first)));
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(port, 0, readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(first, readback, sizeof(first));

    TEST_ASSERT_EQUAL_INT(0, nd_store_write(port, 0, second, sizeof(second)));
    memset(readback, 0, sizeof(readback));
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(port, 0, readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_MEMORY(second, readback, sizeof(second));
}

static void test_measurement_null_ctx_is_rejected(void)
{
    nd_measurement_value_t value;
    board_measurement_ctx_t ctx;
    nd_idis_board_state_t state;

    memset(&state, 0, sizeof(state));
    memset(&value, 0, sizeof(value));
    TEST_ASSERT_EQUAL_INT(-1, board_measurement_read(NULL, ND_MEASURE_VOLTAGE,
                                                     0, &value));
    ctx.calibration = NULL;
    ctx.state = &state;
    TEST_ASSERT_EQUAL_INT(-1, board_measurement_read(&ctx, ND_MEASURE_VOLTAGE,
                                                     0, &value));
}

static void test_measurement_read_uses_the_ctx_state(void)
{
    nd_idis_board_state_t state;
    board_measurement_calibration_t calibration = {
        .voltage = 1000000000,
        .current = 1000000000,
        .power = 1000000000,
    };
    board_measurement_ctx_t ctx;
    nd_measurement_value_t value;

    memset(&state, 0, sizeof(state));
    memset(&value, 0, sizeof(value));
    ctx.calibration = &calibration;
    ctx.state = &state;
    TEST_ASSERT_EQUAL_INT(0, board_measurement_read(&ctx, ND_MEASURE_VOLTAGE,
                                                    0, &value));
    TEST_ASSERT_EQUAL_PTR(&state, rn8209_state_seen);
    TEST_ASSERT_EQUAL_INT64(0, value.value);
    TEST_ASSERT_EQUAL_INT(-1, value.scaler);
    TEST_ASSERT_EQUAL_INT(ND_MEASUREMENT_UNIT_VOLT, value.unit);
}

static void test_key_event_null_state_is_rejected(void)
{
    TEST_ASSERT_EQUAL_INT(0, board_key_event_take(NULL));
    board_gpio_init(NULL);
    TEST_ASSERT_EQUAL_INT(0, register_calls);
}

static void test_registered_isr_sets_the_bound_state(void)
{
    nd_idis_board_state_t state;

    memset(&state, 0, sizeof(state));
    if (map_registers() != 0) {
        TEST_IGNORE_MESSAGE("peripheral register window unavailable");
    }
    pmu_int_status = 1;
    board_gpio_init(&state);
    TEST_ASSERT_EQUAL_INT(1, register_calls);
    TEST_ASSERT_NOT_NULL(registered_isr.IsrFunctionRegister);
    TEST_ASSERT_TRUE((void *)&state.key_event == registered_isr.para);
    registered_isr.IsrFunctionRegister(registered_isr.para);
    TEST_ASSERT_EQUAL_INT(1, board_key_event_take(&state));
    TEST_ASSERT_EQUAL_INT(0, board_key_event_take(&state));
}

static void test_key_events_are_per_state(void)
{
    nd_idis_board_state_t first;
    nd_idis_board_state_t second;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    if (map_registers() != 0) {
        TEST_IGNORE_MESSAGE("peripheral register window unavailable");
    }
    pmu_int_status = 1;
    board_gpio_init(&first);
    board_gpio_init(&second);
    registered_isr.IsrFunctionRegister(registered_isr.para);
    TEST_ASSERT_EQUAL_INT(1, board_key_event_take(&second));
    TEST_ASSERT_EQUAL_INT(0, board_key_event_take(&first));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_key_event_null_state_is_rejected);
    RUN_TEST(test_registered_isr_sets_the_bound_state);
    RUN_TEST(test_key_events_are_per_state);
    RUN_TEST(test_null_state_is_rejected);
    RUN_TEST(test_lazy_init_binds_state_storage);
    RUN_TEST(test_two_states_have_independent_flash);
    RUN_TEST(test_store_port_is_bound_to_state);
    RUN_TEST(test_store_read_write_traverse_the_bound_state);
    RUN_TEST(test_measurement_null_ctx_is_rejected);
    RUN_TEST(test_measurement_read_uses_the_ctx_state);
    return UNITY_END();
}
