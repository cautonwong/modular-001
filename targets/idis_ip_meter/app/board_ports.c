#include "board_ports.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "domain/common/store.h"
#include "components/core/fifo/fifo.h"
#include "components/core/types/measurement_scale.h"
#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_calibration.h"
#include "targets/idis_ip_meter/cosem/nd_measurement_port.h"
#include "targets/idis_ip_meter/board/board_rn8209.h"
#include "targets/idis_ip_meter/board/spi_driver.h"
#include "targets/idis_ip_meter/board/relay_pins.h"
#include "targets/idis_ip_meter/board/uart_driver.h"
#include "targets/idis_ip_meter/board/board_eeprom.h"
#include "targets/idis_ip_meter/board/board_flash.h"
#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc/lib_uart.h"
#include "target.h"
#include "lib_gpio.h"
#include "lib_pmu.h"
#include "cpu_surport.h"

_Static_assert(BOARD_FLASH_SIZE % BOARD_FLASH_SECTOR == 0,
               "flash capacity must be a whole number of sectors");
_Static_assert(BOARD_FLASH_SIZE == ND_MX25L3206E_CAPACITY_BYTES,
               "the board flash must be the MX25L3206E 4 MiB part");

static int board_store_read(void *ctx, uint32_t offset, uint8_t *buffer,
                            size_t length);
static int board_store_write(void *ctx, uint32_t offset,
                             const uint8_t *buffer, size_t length);

void board_bind(nd_idis_board_state_t *state)
{
    if (state == NULL) {
        return;
    }
    state->store.ctx = state;
    state->store.read = board_store_read;
    state->store.write = board_store_write;
    board_rn8209_init(state);
    nd_board_spi_bind(state);
}

int board_rtc_read(nd_dlms_datetime_t *now)
{
    struct tm scratch;

    if (now == NULL || !read_mcu_rtc(0, &scratch)) {
        return -1;
    }
    int year = scratch.tm_year + 1900;
    int month = scratch.tm_mon + 1;
    if (year < 2020 || month < 1 || month > 12) {
        return -1;
    }
    *now = (nd_dlms_datetime_t){
        .year = (uint16_t)year,
        .month = (uint8_t)month,
        .day = (uint8_t)scratch.tm_mday,
        .hour = (uint8_t)scratch.tm_hour,
        .minute = (uint8_t)scratch.tm_min,
        .second = (uint8_t)scratch.tm_sec,
        .status = 0,
    };
    return 0;
}

#define BOARD_UART_FIFO_BYTES 128u
typedef struct {
    uint8_t flash_sector[BOARD_FLASH_SECTOR];
    struct fifo_t uart_fifo[3];
    uint8_t uart_fifo_buffer[3][BOARD_UART_FIFO_BYTES];
} board_buffers_t;

static board_buffers_t board_buffers;

static const UART_InitType uart_485_config = {
    UART_MODE_Msk, UART_PARITY_EVEN_LIB, UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB, 9600,
};
static const UART_InitType uart_module_config = {
    UART_MODE_Msk, UART_PARITY_EVEN_LIB, UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB, 9600,
};
static const UART_InitType uart_ir_config = {
    UART_MODE_Msk, UART_PARITY_EVEN_LIB, UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB, 1200,
};

static const board_measurement_calibration_t default_calibration = {
    .voltage = 18213639,
    .current = 288020000,
    .power = 1791867,
};

static nd_idis_calibration_status_t board_calibration_load(
    nd_idis_board_state_t *state, board_measurement_calibration_t *calibration)
{
    nd_idis_calibration_record_t record;
    nd_idis_calibration_status_t status =
        nd_idis_calibration_load(&record, board_store_port(state));
    switch (status) {
    case ND_IDIS_CALIBRATION_LOADED:
    case ND_IDIS_CALIBRATION_ADOPTED:
        calibration->voltage = record.voltage;
        calibration->current = record.current;
        calibration->power = record.power;
        break;
    case ND_IDIS_CALIBRATION_FAULT:
    case ND_IDIS_CALIBRATION_LOST:
    case ND_IDIS_CALIBRATION_NONE:
        break;
    }
    return status;
}

static int board_store_read(void *ctx, uint32_t offset, uint8_t *buffer,
                            size_t length)
{
    nd_idis_board_state_t *state = (nd_idis_board_state_t *)ctx;

    if (state == NULL || length > 0xFFFFu) {
        return -1;
    }
    return nd_eeprom_read(board_eeprom(state), offset, buffer,
                          (uint16_t)length) == 0
               ? 0
               : -1;
}

static int board_store_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                             size_t length)
{
    nd_idis_board_state_t *state = (nd_idis_board_state_t *)ctx;

    if (state == NULL || length > 0xFFFFu) {
        return -1;
    }
    mcu_clear_watchdog();
    int result = nd_eeprom_write(board_eeprom(state), offset, buffer,
                                 (uint16_t)length) == 0
                     ? 0
                     : -1;
    mcu_clear_watchdog();
    return result;
}

static int flash_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    nd_idis_board_state_t *state = (nd_idis_board_state_t *)ctx;

    if (state == NULL || length > 0xFFFFu) {
        return -1;
    }
    return nd_mx25l3206e_read(board_flash(state), offset, buffer,
                              (uint16_t)length) == 0
               ? 0
               : -1;
}

static int flash_erase(void *ctx, uint32_t offset)
{
    nd_idis_board_state_t *state = (nd_idis_board_state_t *)ctx;

    if (state == NULL) {
        return -1;
    }
    return nd_mx25l3206e_erase(board_flash(state), offset,
                               BOARD_FLASH_SECTOR) == 0
               ? 0
               : -1;
}

static int flash_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    nd_idis_board_state_t *state = (nd_idis_board_state_t *)ctx;

    if (state == NULL || length > 0xFFFFu) {
        return -1;
    }
    return nd_mx25l3206e_write(board_flash(state), offset, buffer,
                               (uint16_t)length) == 0
               ? 0
               : -1;
}

nd_store_t *board_store_port(nd_idis_board_state_t *state)
{
    if (state == NULL) {
        return NULL;
    }
    return &state->store;
}

nd_store_t *board_flash_port(nd_idis_board_state_t *state)
{
    if (state == NULL) {
        return NULL;
    }
    return nd_sector_store_port(&state->flash_store);
}

void board_store_init(nd_idis_board_state_t *state)
{
    (void)board_eeprom(state);
}

void board_flash_init(nd_idis_board_state_t *state)
{
    if (state == NULL) {
        return;
    }
    (void)board_flash(state);
    /* Off until the board bus measurement; FreeRTOS also needs a store lock. */
    nd_sector_store_init(&state->flash_store, state, flash_read, flash_erase,
                         flash_write, BOARD_FLASH_SIZE, BOARD_FLASH_SECTOR,
                         board_buffers.flash_sector, 0);
}

static void uart_receive_timeout(void)
{
}

static void uart_send_timeout(void *ctx)
{
    (void)ctx;
}

static void uart_channel_init(nd_idis_board_state_t *state, uint8_t channel,
                              const UART_InitType *config, uint8_t uart_index,
                              uint8_t channel_type)
{
    struct uart_chl_init_t init = {
        .tx_fifo = &board_buffers.uart_fifo[channel],
        .rx_fifo = &board_buffers.uart_fifo[channel],
        .uart_cfg = config,
        .mcu_uart_index = uart_index,
        .chl_type = channel_type,
        .CallBackReceiveTimeOut = uart_receive_timeout,
        .CallBackSendTimeOut = uart_send_timeout,
    };
    (void)uart_chl_initialize(state, &init);
}

void board_uart_init(nd_idis_board_state_t *state)
{
    fifoBuf_init(&board_buffers.uart_fifo[0],
                 board_buffers.uart_fifo_buffer[0], BOARD_UART_FIFO_BYTES);
    fifoBuf_init(&board_buffers.uart_fifo[1],
                 board_buffers.uart_fifo_buffer[1], BOARD_UART_FIFO_BYTES);
    fifoBuf_init(&board_buffers.uart_fifo[2],
                 board_buffers.uart_fifo_buffer[2], BOARD_UART_FIFO_BYTES);
    uart_channel_init(state, 0, &uart_485_config, ND_UART_RS4851,
                      UART_RS4851_CHL);
    uart_channel_init(state, 1, &uart_module_config, ND_UART_MODULE,
                      UART_MODULE_CHL);
    uart_channel_init(state, 2, &uart_ir_config, ND_UART_IR, UART_IR_CHL);
}

int board_ir_read(uint8_t *byte)
{
    if (byte == NULL || fifoBuf_getUsed(&board_buffers.uart_fifo[UART_IR_CHL]) == 0) {
        return 0;
    }
    *byte = fifoBuf_getByte(&board_buffers.uart_fifo[UART_IR_CHL]);
    return 1;
}

static void board_pmu_isr(void *param)
{
    volatile uint8_t *event = (volatile uint8_t *)param;

    if (event == NULL) {
        return;
    }
    if (PMU_GetIOAINTStatus(GPIO_Pin_2)) {
        PMU_ClearIOAINTStatus(GPIO_Pin_2);
        *event = 1;
    }
}

int board_key_event_take(nd_idis_board_state_t *state)
{
    uint8_t event;

    if (state == NULL) {
        return 0;
    }
    event = state->key_event;
    state->key_event = 0;
    return event != 0;
}

void board_gpio_init(nd_idis_board_state_t *state)
{
    if (state == NULL) {
        return;
    }
    GPIOC->OEN &= ~BIT6;
    GPIOC->IE &= ~BIT6;
    GPIOC->ATT &= ~BIT6;
    GPIOC->DAT |= BIT6;

    GPIOB->OEN &= ~BIT8;
    GPIOB->IE &= ~BIT8;
    GPIOB->ATT &= ~BIT8;
    GPIOB->DAT &= ~BIT8;

    GPIOA->OEN |= BIT15 | BIT14 | BIT2;
    GPIOA->IE |= BIT15 | BIT14 | BIT2;

    GPIOC->OEN &= ~BIT9;
    GPIOC->IE &= ~BIT9;
    GPIOC->ATT |= BIT9 | BIT10;
    GPIOC->DAT |= BIT9;
    GPIOC->OEN |= BIT10;
    GPIOC->IE |= BIT10;

    GPIOF->OEN |= BIT1;
    GPIOF->IE |= BIT1;
    GPIOF->ATT |= BIT1;

    GPIOB->ATT |= BIT0 | BIT6 | BIT9 | BIT10 | BIT11 | BIT12;

    PMU_WakeUpPinConfig(GPIO_Pin_2, IOA_EDGEBOTH);
    PMU_ClearIOAINTStatus(GPIO_Pin_2);
    PMU_INTConfig(PMU_INT_IOAEN, ENABLE);
    PMU_WakeUpPinConfig(GPIO_Pin_8, IOA_DISABLE);

    struct isr_reg_ctx_t registration = {
        .IsrFunctionRegister = board_pmu_isr,
        .para = (void *)&state->key_event,
        .irq_index = IRQ_PMU,
    };
    (void)Isr_handle_regsit(&registration, IRQ_PMU);
}

nd_idis_calibration_status_t
board_measurement_init(nd_idis_board_state_t *state,
                       board_measurement_calibration_t *calibration)
{
    if (state == NULL) {
        return ND_IDIS_CALIBRATION_NONE;
    }
    *calibration = default_calibration;
    nd_idis_calibration_status_t status = board_calibration_load(state, calibration);
    (void)board_rn8209(state);
    return status;
}

int board_adc_read(uint8_t channel, int32_t *value)
{
    if (value == NULL || mcu_adc_start_once(channel) == 0u) {
        return -1;
    }
    if (read_mcu_adc_measure_value(channel, value) == 0u) {
        return -1;
    }
    return 0;
}

int board_measurement_read(void *ctx, nd_measure_quantity_t quantity,
                           uint8_t phase, nd_measurement_value_t *out)
{
    const board_measurement_ctx_t *measurement = ctx;
    const board_measurement_calibration_t *calibration;
    nd_rn8209_t *rn8209;

    if (measurement == NULL || measurement->calibration == NULL) {
        return -1;
    }
    calibration = measurement->calibration;
    rn8209 = board_rn8209(measurement->state);
    (void)phase;
    uint32_t raw = 0;
    int64_t value = 0;
    int8_t scaler = 0;
    nd_measurement_unit_t unit = ND_MEASUREMENT_UNIT_NONE;
    switch (quantity) {
    case ND_MEASURE_VOLTAGE:
        if (nd_rn8209_read(rn8209, ND_RN8209_URMS, &raw, 3) != 0) {
            return -1;
        }
        value = measurement_scale_clamp(((float)calibration->voltage
                                         / 1000000000.0f)
                                        * (float)raw)
                / 10;
        scaler = -1;
        unit = ND_MEASUREMENT_UNIT_VOLT;
        break;
    case ND_MEASURE_CURRENT:
    case ND_MEASURE_CURRENT_SUM:
        if (nd_rn8209_read(rn8209, ND_RN8209_IARMS, &raw, 3) != 0) {
            return -1;
        }
        value = measurement_scale_clamp(((float)calibration->current
                                         / 1000000000.0f)
                                        * (float)raw)
                / 10;
        scaler = -3;
        unit = ND_MEASUREMENT_UNIT_AMPERE;
        break;
    case ND_MEASURE_ACTIVE_POWER:
    case ND_MEASURE_TOTAL_ACTIVE_POWER:
        if (nd_rn8209_read(rn8209, ND_RN8209_POWERPA, &raw, 4) != 0) {
            return -1;
        }
        value = measurement_scale_clamp(((float)calibration->power
                                         / 1000000000.0f)
                                        * (float)(int32_t)raw);
        unit = ND_MEASUREMENT_UNIT_WATT;
        break;
    case ND_MEASURE_FREQUENCY:
        if (nd_rn8209_read(rn8209, ND_RN8209_UFREQ, &raw, 2) != 0) {
            return -1;
        }
        value = raw;
        scaler = -2;
        unit = ND_MEASUREMENT_UNIT_HERTZ;
        break;
    default:
        return -1;
    }
    *out = (nd_measurement_value_t){
        .value = value,
        .scaler = scaler,
        .unit = unit,
        .phase = phase,
    };
    return 0;
}

void board_relay_apply(int connected)
{
    if (connected) {
        nd_relay_on_pulse(1);
    } else {
        nd_relay_off_pulse(1);
    }
    mcu_delay_us(8000);
    nd_relay_on_pulse(0);
    nd_relay_off_pulse(0);
}
