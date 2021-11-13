#ifndef _CPU_SURPORT_H_
#define _CPU_SURPORT_H_

#include "type_def.h"
#include "nidavellir.h"

typedef void (*isr_handle)(void);

extern void mcu_init(void* init_type);

extern uint8 mcu_uart_init(void* init_config, uint8 uart_index, uint8 infrared_enable);
extern void mcu_uart_recive_enable(uint8 ops_enable, size_t uart_index);
extern int mcu_uart_send_byte(uint8 send_data, size_t uart_index);

extern int mcu_uart_recive_byte(uint8* byte , size_t uart_index);
extern void mcu_uart_send_enable(uint8 ops_enable, size_t uart_index);

extern uint8 mcu_uart_close(uint8 uart_index);
extern uint8 mcu_uart_set_baud(uint16 baud, uint8 uart_index);
extern uint8 mcu_uart_get_status(uint8 uart_index, uint8 status);

extern uint8 mcu_spi_bus_init(uint8 spi_index , uint8 mode);
extern uint8 mcu_spi_send_byte(uint8 byte , uint8 spi_index);
extern uint8 mcu_spi_recive_byte(uint8 *byte, uint8 spi_index);

extern void  mcu_clear_watchdog(void);
extern uint8 mcu_delay_us(volatile uint16 count);
extern uint8 ac_power_off_monitor(void);
extern uint8 ac_power_on_monitor(void);

extern void mcu_timer_ctrl(uint32 period_ms, uint8 index , uint8 stop_or_start);

extern void mcu_main_frequency_mode(uint8 clk_mode);
extern void mcu_enter_lowpower_then_sleep(void);
extern void mcu_exit_sleep_then_enter_normal(void);

extern void mcu_main_power_on_config(void);
extern void mcu_main_power_off_config(void);
extern void cm0_systick_init(uint8 cmd);
extern void ndv_isr_handle_regsit(isr_handle isr_handle_f , uint8 irq);


#define  DISABLE_ALL_INT()     __disable_irq();
#define  ENABLE_ALL_INT()      __enable_irq();

#define STOP    0xAA
#define START   0x55
#define ACK_1   0x33
#define ACK_0  0x66

#define AC_POWER_ON 0xAA
#define AC_POWER_OFF 0x55

#define  CTRL_FLS_SECTOR_ERASE  0x01
#define  CTRL_FLS_BLOCK_ERASE   0x02
#define  CTRL_FLS_TEST          0x03




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
   
   IRQ_NUM
};






enum bus_type
{
    BUS_I2C,
    BUS_SPI,
    BUS_UART,
};




enum device_id_t
{
    DEV_EEPROM        = 1,
    DEV_FLASH         ,
    DEV_EMU           ,
    DEV_ESAM_698      ,
    DEV_CARD          ,
    DEV_RAM           ,
};






#endif













