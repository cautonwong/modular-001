#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "domain/common/store.h"
#include "infra/kernel/fifo.h"
#include "infra/memory/sector_store.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_measurement_port.h"
#include "infra/kernel/resource_blocks.h"
#include "nidavellir/include/kernel/device.h"
#include "infra/boards/stategrid_single_phase/ndvh_rn8209.h"
#include "infra/boards/stategrid_single_phase/relay_pins.h"
#include "infra/boards/stategrid_single_phase/uart_driver.h"
#include "infra/soc/v85xxp/v8500/vendor/Lib_Driver/inc/lib_uart.h"
#include "target.h"
#include "lib_gpio.h"
#include "lib_pmu.h"
#include "cpu_surport.h"

struct instance_t;

extern const struct bus_ops_t iic_bus;
extern const struct bus_ops_t mcu_uart_bus_ops;
extern const struct bus_ops_t spi_mode3;
extern const struct driver_ops_t eeprom_driver;
extern const struct driver_ops_t rn8209_drv;
extern const struct driver_ops_t nor_flash_driver;
extern int eeprom_initialize(struct instance_t *instance,
                             const struct dev_info_t *dev);
extern int eeprom_read_chk(struct instance_t *instance,
                           const struct dev_info_t *dev, uint32_t addr,
                           uint16_t len, void *src);
extern int eeprom_write_chk(struct instance_t *instance,
                            const struct dev_info_t *dev, uint32_t addr,
                            uint16_t len, void *src);

static const struct dev_info_t eeprom_dev = {
    .dev_type = DEV_EEPROM,
    .dev_index = 1,
    .data_size = 0xFFFF,
    .dev_bus_ops = &iic_bus,
    .dev_driver_ops = &eeprom_driver,
};

static const struct dev_info_t nor_flash_dev = {
    .dev_type = DEV_FLASH,
    .dev_index = 0,
    .data_size = 0x7FFFFF,
    .dev_bus_ops = &spi_mode3,
    .dev_driver_ops = &nor_flash_driver,
};

#define BOARD_FLASH_SECTOR 4096u
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
    if (eeprom_read_chk(NULL, &eeprom_dev, BOARD_CALIBRATION_OFFSET,
                        sizeof(record), &record)
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
    return eeprom_read_chk(NULL, &eeprom_dev, offset, (uint16_t)length, buffer)
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
    int result = eeprom_write_chk(NULL, &eeprom_dev, offset,
                                  (uint16_t)length, (void *)buffer)
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
    return nor_flash_driver.dev_read(NULL, &nor_flash_dev, offset,
                                     (uint16_t)length, buffer)
               ? 0
               : -1;
}

static int flash_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    uint32_t address = offset;
    return nor_flash_driver.dev_control(NULL, &nor_flash_dev,
                                        CTRL_FLS_SECTOR_ERASE, &address)
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
    return nor_flash_driver.dev_write(NULL, &nor_flash_dev, offset,
                                      (uint16_t)length, (void *)buffer)
               ? 0
               : -1;
}

static nd_sector_store_t flash_store;

static const UART_InitType rn8209_uart_config = {
    UART_MODE_Msk, UART_PARITY_EVEN_LIB, UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB, 4800 * Fr_K,
};

static const struct dev_info_t rn8209_dev = {
    .dev_type = DEV_EMU,
    .dev_index = 0,
    .dev_bus_ops = &mcu_uart_bus_ops,
    .dev_driver_ops = &rn8209_drv,
    .cfg = &rn8209_uart_config,
};

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
    (void)eeprom_initialize(NULL, &eeprom_dev);
}

void board_flash_init(void)
{
    (void)nor_flash_driver.dev_initialize(NULL, &nor_flash_dev);
    nd_sector_store_init(&flash_store, NULL, flash_read, flash_erase,
                         flash_write, (uint32_t)nor_flash_dev.data_size,
                         BOARD_FLASH_SECTOR, flash_sector);
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
    /* Rn8209_initialize() is a no-op; the UART bus carries the config. */
    if (rn8209_dev.dev_bus_ops != NULL && rn8209_dev.dev_bus_ops->init != NULL) {
        (void)rn8209_dev.dev_bus_ops->init((size_t)rn8209_dev.cfg);
    }
    (void)rn8209_drv.dev_initialize(NULL, &rn8209_dev);
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
        if (!rn8209_drv.dev_read(NULL, &rn8209_dev, URMS, 3, &raw)) {
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
        if (!rn8209_drv.dev_read(NULL, &rn8209_dev, IARMS, 3, &raw)) {
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
        if (!rn8209_drv.dev_read(NULL, &rn8209_dev, POWERPA, 4, &raw)) {
            return -1;
        }
        value = (int64_t)(((float)calibration_power / 1000000000.0f)
                          * (float)(int32_t)raw);
        unit = ND_MEASUREMENT_UNIT_WATT;
        break;
    case ND_MEASURE_FREQUENCY:
        if (!rn8209_drv.dev_read(NULL, &rn8209_dev, UFREQ, 2, &raw)) {
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
