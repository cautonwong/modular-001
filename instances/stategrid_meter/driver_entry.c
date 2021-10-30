#include "cpu_surport.h"
#include "nidavellir.h"
#include "lib_conf.h"
#include "time_link.h"

extern const struct driver_ops_t  CODE eeprom_driver;
extern const struct bus_ops_t iic_bus;
extern const struct bus_ops_t spi_mode3;
extern const struct driver_ops_t  CODE uart_driver;
extern const struct driver_ops_t rn8209_drv;
extern const struct driver_ops_t nor_flash_driver;
extern const struct bus_ops_t mcu_uart_bus_ops;

const struct dev_info_t eeprom_dev=
{
    DEV_EEPROM,
    1,
    0xFFFF,
    0,
    0,
    &iic_bus,
    &eeprom_driver,
    __NULL,
    __NULL,
    __NULL,
    __NULL,
};

const UART_InitType  Uart_InitType = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    2400,
};

const UART_InitType  Rn8209_Uart_InitType = 
{
    UART_MODE_Msk,
    UART_PARITY_EVEN_LIB,
    UART_WORDLEN_9B,
    UART_FIRSTBIT_LSB,
    4800,
};

struct dev_info_t rs485_dev=
{
    DEV_RS4851,
    0,
    0xFFFF,
    0,
    0,
    __NULL,
    &uart_driver,
    __NULL,
    __NULL,
    __NULL,
    &Uart_InitType,
};


struct dev_info_t rn8209_dev=
{
    DEV_EMU,
    0,
    0x0000,
    0,
    0,
    &mcu_uart_bus_ops,
    &rn8209_drv,
    __NULL,
    __NULL,
    __NULL,
    &Rn8209_Uart_InitType,
};


const struct dev_info_t nor_flash_dev=
{
    DEV_FLASH,
    0,
    0x7FFFFF,
    0,
    0,
    &spi_mode3,
    &rn8209_drv,
    __NULL,
    __NULL,
    __NULL,
    __NULL,
};

/*
void main(void)
{    
    mcu_init(__NULL);
    
    mcu_main_power_on_config();
    
    ndv_timer_init();
    
    eeprom_dev.dev_driver_ops->dev_initialize(&eeprom_dev);
    
    rs485_dev.dev_driver_ops->dev_initialize(&rs485_dev);
    
    rn8209_dev.dev_driver_ops->dev_initialize(&rn8209_dev);
    
    nor_flash_dev.dev_driver_ops->dev_initialize(&nor_flash_dev);
    
    while(1)
    {
        mcu_clear_watchdog();
       
        //eeprom_dev.dev_driver_ops->dev_control(&eeprom_dev , 0 , 0);
        
        //rn8209_dev.dev_driver_ops->dev_control(&rn8209_dev , AC_POWER_ON , 0);

       nor_flash_dev.dev_driver_ops->dev_control(&nor_flash_dev , CTRL_FLS_TEST , 0);
        
    }
}
*/