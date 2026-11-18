#include "board_ports.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "domain/common/store.h"
#include "components/core/fifo/fifo.h"
#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_measurement_port.h"
#include "targets/idis_ip_meter_freertos/board/board_rn8209.h"
#include "targets/idis_ip_meter_freertos/board/relay_pins.h"
#include "targets/idis_ip_meter_freertos/board/uart_driver.h"
#include "targets/idis_ip_meter_freertos/board/board_eeprom.h"
#include "targets/idis_ip_meter_freertos/board/board_flash.h"
#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc/lib_uart.h"
#include "target.h"
#include "lib_gpio.h"
#include "lib_pmu.h"
#include "cpu_surport.h"



static struct tm rtc_scratch;

int board_rtc_read(nd_dlms_datetime_t *now)
{
    if (now == NULL || !read_mcu_rtc(0, &rtc_scratch)) {
        return -1;
    }
    int year = rtc_scratch.tm_year + 1900;
    int month = rtc_scratch.tm_mon + 1;
    if (year < 2020 || month < 1 || month > 12) {
        return -1;
    }
    *now = (nd_dlms_datetime_t){
        .year = (uint16_t)year,
        .month = (uint8_t)month,
        .day = (uint8_t)rtc_scratch.tm_mday,
        .hour = (uint8_t)rtc_scratch.tm_hour,
        .minute = (uint8_t)rtc_scratch.tm_min,
        .second = (uint8_t)rtc_scratch.tm_sec,
        .status = 0,
    };
    return 0;
}

static uint8_t flash_sector[BOARD_FLASH_SECTOR];

#define BOARD_UART_FIFO_BYTES 128u
static struct fifo_t uart_fifo[3];
static uint8_t uart_fifo_buffer[3][BOARD_UART_FIFO_BYTES];

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

#define BOARD_CALIBRATION_OFFSET 57344u
#define BOARD_CALIBRATION_MAGIC 0x43414C31u
typedef struct {
    uint32_t magic;
    int32_t voltage;
    int32_t current;
    int32_t power;
} board_calibration_record_t;

static int32_t calibration_voltage = 18213639;
static int32_t calibration_current = 288020000;
static int32_t calibration_power = 1791867;

static void board_calibration_load(void)
{
    board_calibration_record_t record;
    if (nd_eeprom_read(board_eeprom(), BOARD_CALIBRATION_OFFSET,
                       (uint8_t *)&record, sizeof(record)) == 0
        && record.magic == BOARD_CALIBRATION_MAGIC && record.voltage > 0
        && record.current > 0 && record.power > 0) {
        calibration_voltage = record.voltage;
        calibration_current = record.current;
        calibration_power = record.power;
    }
}

static int board_store_read(void *ctx, uint32_t offset, uint8_t *buffer,
                            size_t length)
{
    (void)ctx;
    if (length > 0xFFFFu) {
        return -1;
    }
    return nd_eeprom_read(board_eeprom(), offset, buffer,
                          (uint16_t)length) == 0
               ? 0
               : -1;
}

static int board_store_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                             size_t length)
{
    (void)ctx;
    if (length > 0xFFFFu) {
        return -1;
    }
    mcu_clear_watchdog();
    int result = nd_eeprom_write(board_eeprom(), offset, buffer,
                                 (uint16_t)length) == 0
                     ? 0
                     : -1;
    mcu_clear_watchdog();
    return result;
}

static int flash_read(void *ctx, uint32_t offset, uint8_t *buffer,
                      size_t length)
{
    (void)ctx;
    if (length > 0xFFFFu) {
        return -1;
    }
    return nd_mx25l3206e_read(board_flash(), offset, buffer,
                              (uint16_t)length) == 0
               ? 0
               : -1;
}

static int flash_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    return nd_mx25l3206e_erase(board_flash(), offset, BOARD_FLASH_SECTOR) == 0
               ? 0
               : -1;
}

static int flash_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    if (length > 0xFFFFu) {
        return -1;
    }
    return nd_mx25l3206e_write(board_flash(), offset, buffer,
                               (uint16_t)length) == 0
               ? 0
               : -1;
}

static nd_sector_store_t flash_store;

static const nd_store_t board_store = {
    .ctx = NULL,
    .read = board_store_read,
    .write = board_store_write,
};

nd_store_t *board_store_port(void)
{
    return (nd_store_t *)&board_store;
}

nd_store_t *board_flash_port(void)
{
    return nd_sector_store_port(&flash_store);
}

void board_store_init(void)
{
    (void)board_eeprom();
}

void board_flash_init(void)
{
    (void)board_flash();
    nd_sector_store_init(&flash_store, NULL, flash_read, flash_erase,
                         flash_write, BOARD_FLASH_SIZE, BOARD_FLASH_SECTOR,
                         flash_sector);
}

static void uart_receive_timeout(void)
{
}

static void uart_send_timeout(void *ctx)
{
    (void)ctx;
}

static void uart_channel_init(uint8_t channel, const UART_InitType *config,
                              uint8_t uart_index, uint8_t channel_type)
{
    struct uart_chl_init_t init = {
        .tx_fifo = &uart_fifo[channel],
        .rx_fifo = &uart_fifo[channel],
        .uart_cfg = config,
        .mcu_uart_index = uart_index,
        .chl_type = channel_type,
        .CallBackReceiveTimeOut = uart_receive_timeout,
        .CallBackSendTimeOut = uart_send_timeout,
    };
    (void)uart_chl_initialize(&init);
}

void board_uart_init(void)
{
    fifoBuf_init(&uart_fifo[0], uart_fifo_buffer[0], BOARD_UART_FIFO_BYTES);
    fifoBuf_init(&uart_fifo[1], uart_fifo_buffer[1], BOARD_UART_FIFO_BYTES);
    fifoBuf_init(&uart_fifo[2], uart_fifo_buffer[2], BOARD_UART_FIFO_BYTES);
    uart_channel_init(0, &uart_485_config, ND_UART_RS4851,
                      UART_RS4851_CHL);
    uart_channel_init(1, &uart_module_config, ND_UART_MODULE,
                      UART_MODULE_CHL);
    uart_channel_init(2, &uart_ir_config, ND_UART_IR, UART_IR_CHL);
}

static volatile uint32_t rtc_seconds;

static void board_rtc_isr(void *param)
{
    (void)param;
    rtc_seconds++;
}

void board_rtc_init(void)
{
    struct isr_reg_ctx_t registration = {
        .IsrFunctionRegister = board_rtc_isr,
        .para = NULL,
        .irq_index = IRQ_RTC,
    };
    (void)Isr_handle_regsit(&registration, IRQ_RTC);
}

static volatile uint8_t key_event;

static void board_pmu_isr(void *param)
{
    (void)param;
    if (PMU_GetIOAINTStatus(GPIO_Pin_2)) {
        PMU_ClearIOAINTStatus(GPIO_Pin_2);
        key_event = 1;
    }
}

int board_key_event_take(void)
{
    uint8_t event = key_event;
    key_event = 0;
    return event != 0;
}

void board_gpio_init(void)
{
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
        .para = NULL,
        .irq_index = IRQ_PMU,
    };
    (void)Isr_handle_regsit(&registration, IRQ_PMU);
}

void board_measurement_init(void)
{
    board_calibration_load();
    (void)board_rn8209();
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
    (void)ctx;
    (void)phase;
    uint32_t raw = 0;
    int64_t value = 0;
    int8_t scaler = 0;
    nd_measurement_unit_t unit = ND_MEASUREMENT_UNIT_NONE;
    switch (quantity) {
    case ND_MEASURE_VOLTAGE:
        if (nd_rn8209_read(board_rn8209(), ND_RN8209_URMS, &raw, 3) != 0) {
            return -1;
        }
        value = (int64_t)(((float)calibration_voltage / 1000000000.0f)
                          * (float)raw)
                / 10;
        scaler = -1;
        unit = ND_MEASUREMENT_UNIT_VOLT;
        break;
    case ND_MEASURE_CURRENT:
    case ND_MEASURE_CURRENT_SUM:
        if (nd_rn8209_read(board_rn8209(), ND_RN8209_IARMS, &raw, 3) != 0) {
            return -1;
        }
        value = (int64_t)(((float)calibration_current / 1000000000.0f)
                          * (float)raw)
                / 10;
        scaler = -3;
        unit = ND_MEASUREMENT_UNIT_AMPERE;
        break;
    case ND_MEASURE_ACTIVE_POWER:
    case ND_MEASURE_TOTAL_ACTIVE_POWER:
        if (nd_rn8209_read(board_rn8209(), ND_RN8209_POWERPA, &raw, 4) != 0) {
            return -1;
        }
        value = (int64_t)(((float)calibration_power / 1000000000.0f)
                          * (float)(int32_t)raw);
        unit = ND_MEASUREMENT_UNIT_WATT;
        break;
    case ND_MEASURE_FREQUENCY:
        if (nd_rn8209_read(board_rn8209(), ND_RN8209_UFREQ, &raw, 2) != 0) {
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
