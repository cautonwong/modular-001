#ifndef _CPU_SURPORT_H_
#define _CPU_SURPORT_H_

#include "type_def.h"
#include "nidavellir.h"

//typedef void (*isr_handle)(void);

struct isr_reg_ctx_t
{
    void (*IsrFunctionRegister)(void *);
    void *para;
    uint8_t irq_index;
};

#define   IO_DAT(IOX, PINX)  *((volatile unsigned long *)((((uint32_t)IOX) & 0xF0000000)\
                              +0x2000000+((((uint32_t)IOX) &0xFFFFF)<<5)+(PINX<<2)))

/*MCU¶ÔÍâÌá¹©µÄ·þÎñ½Ó¿Úº¯ÊýÉùÃ÷*/
extern void mcu_init(void* init_type);

extern uint8_t mcu_uart_init(void* init_config, uint8_t uart_index, uint8_t infrared_enable);
extern void mcu_uart_recive_enable(uint8_t ops_enable, size_t uart_index);
extern int mcu_uart_send_byte(uint8_t send_data, size_t uart_index);

extern int mcu_uart_recive_byte(uint8_t* byte , size_t uart_index);
extern void mcu_uart_send_enable(uint8_t ops_enable, size_t uart_index);

extern uint8_t mcu_uart_close(uint8_t uart_index);
extern uint8_t mcu_uart_set_baud(uint32_t baud, uint8_t uart_index);
extern uint8_t mcu_uart_get_status(uint8_t uart_index, uint8_t status);

extern uint8_t mcu_spi_bus_init(uint8_t spi_index , uint8_t mode);
extern uint8_t mcu_spi_send_byte(uint8_t byte , uint8_t spi_index);
extern uint8_t mcu_spi_recive_byte(uint8_t *byte, uint8_t spi_index);

extern void  mcu_clear_watchdog(void);
extern uint8_t mcu_delay_us(volatile uint16_t count);
extern int mcu_delay_ms_power_down_quit(volatile uint16_t count);
extern uint8_t ac_power_off_monitor(void);
extern uint8_t ac_power_on_monitor(void);

extern void mcu_timer_ctrl(uint32_t period_ms, uint8_t index , uint8_t stop_or_start);

extern void mcu_main_frequency_mode(uint8_t clk_mode);
extern void mcu_enter_lowpower_then_sleep(void);
extern void mcu_exit_sleep_then_enter_normal(void);

extern void mcu_main_power_on_config(uint8_t power_mode);
extern void mcu_main_power_off_config(void);
extern void cm0_systick_init(uint8_t cmd);
//extern void ndv_isr_handle_regsit(isr_handle isr_handle_f , uint8_t irq);
extern uint8_t Isr_handle_regsit(struct isr_reg_ctx_t *isrRegctx , uint8_t irq);

extern uint8_t read_mcu_rtc(uint8_t addr, struct instance_t* instance);
extern uint8_t write_mcu_rtc(uint8_t addr, uint8_t *buf, uint16_t len);

extern uint8_t mcu_adc_start_once(uint8_t chl_type);
extern uint32_t read_mcu_adc_measure_value(uint8_t chl_type , int32_t *adc_measure);
extern void opreate_second_pluse(uint8_t mode);
extern void device_power_close(struct instance_t* instance);
extern void device_power_open(struct instance_t* instance, struct pipeline_t* pipeline);
extern void cpu_io_lowpower_cfg_chk(struct instance_t *instance);
extern void cpu_sleep_manage(struct instance_t* instance, struct pipeline_t* pipeline);
extern  void device_power_close1(struct instance_t* instance);

#define  DISABLE_ALL_INT()     __disable_irq();
#define  ENABLE_ALL_INT()      __enable_irq();

#define STOP    0xAA
#define START   0x55
#define ACK_1   0x33
#define ACK_0  0x66

#define AC_POWER_ON 0xAA
#define AC_POWER_OFF 0x55
#define METERCHIP_CHK 0x5A

#define  CTRL_FLS_SECTOR_ERASE  0x01
#define  CTRL_FLS_BLOCK_ERASE   0x02
#define  CTRL_FLS_TEST          0x03

#define  EVENT_SET  0xAA
#define  EVENT_RESET  0x55


enum irq_en
{
   IRQ_SYSTICK,
   IRQ_TMR0,
   IRQ_TMR1,
   IRQ_TMR2,
   IRQ_TMR3,
   IRQ_DMA,
   IRQ_RTC,
   IRQ_PMU,
   IRQ_UART0_TX,
   IRQ_UART0_RX,
   IRQ_UART1_TX,
   IRQ_UART1_RX,   
   IRQ_UART2_TX,
   IRQ_UART2_RX,   
   IRQ_UART3_TX,
   IRQ_UART3_RX,   
   IRQ_UART4_TX,
   IRQ_UART4_RX,
   IRQ_UART5_TX,
   IRQ_UART5_RX,
   IRQ_RS485_RX_TM_OUT,
   IRQ_PLC_RX_TM_OUT,
   IRQ_IR_RX_TM_OUT,
   IRQ_UART_RX_TM_OUT,
   IRQ_NUM
};






enum bus_type
{
    BUS_I2C,
    BUS_SPI,
    BUS_UART,
};











#endif













