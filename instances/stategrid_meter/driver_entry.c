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
extern void esam_698_init(void);

static uint8_t fifo_buf[32];
static struct fifo_t irq_fifo;

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

const struct dev_info_t rs485_dev=
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


const struct dev_info_t rn8209_dev=
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

static uint8 sec_flag;

void RTC_Second(void)
{
    fifoBuf_putByte(&irq_fifo, IRQ_RTC);
    sec_flag = 0x55;
}



extern void Electrical_measurement_reset(void);
extern void Electrical_measurement(void);


void communication_chlx_init(struct fifo_t* txfifo , struct fifo_t* rxfifo , const struct dev_info_t * chl_dev)
{
    struct dev_info_t chl_dev_init;

    chl_dev_init.dev_type = chl_dev->dev_type;
    chl_dev_init.dev_index = chl_dev->dev_index;
    chl_dev_init.data_size = chl_dev->data_size;
    chl_dev_init.dev_timer_type = chl_dev->dev_timer_type; 
    chl_dev_init.dev_timer_id = chl_dev->dev_timer_id;
    chl_dev_init.dev_bus_ops = chl_dev->dev_bus_ops;   
    chl_dev_init.dev_driver_ops = chl_dev->dev_driver_ops;
    chl_dev_init.dev_sys_ops = chl_dev->dev_sys_ops;
    chl_dev_init.dev_input_fifo = rxfifo;
    chl_dev_init.dev_output_fifo = txfifo;
    chl_dev_init.cfg = chl_dev->cfg;

    chl_dev_init.dev_driver_ops->dev_initialize(&chl_dev_init);
}

void irq_system_init(void)
{

    fifoBuf_init(&irq_fifo, fifo_buf, 32);
    ndv_isr_handle_regsit(RTC_Second , IRQ_RTC );
    


}

void irq_transform_to_instance(struct instance_t* instance)
{
    uint8_t fifo_irq = 0;
    uint8_t i;
    
    while(fifoBuf_getUsed(&irq_fifo) >0)
    {
       fifo_irq = fifoBuf_getByte(&irq_fifo);
       
       switch(fifo_irq)
       {
           case IRQ_RTC:
                fifoBuf_putByte(&instance->pipe_rtc, IRQ_RTC);
             break;
             
             
           default:
             break;
       }
    }
}

void dev_init(void)
{
    mcu_main_power_on_config();
    
    ndv_timer_init();
    
    eeprom_dev.dev_driver_ops->dev_initialize(&eeprom_dev);
    
    rs485_dev.dev_driver_ops->dev_initialize(&rs485_dev);
    
    rn8209_dev.dev_driver_ops->dev_initialize(&rn8209_dev);
    
    nor_flash_dev.dev_driver_ops->dev_initialize(&nor_flash_dev);
    
    rn8209_dev.dev_driver_ops->dev_control(&rn8209_dev , AC_POWER_ON , 0);
    
    Electrical_measurement_reset();
    
    esam_698_init();

}

/*
void main(void)
{    
    mcu_init(__NULL);
    
    dev_init();
        
    
    while(1)
    {
        mcu_clear_watchdog();
        
        if(sec_flag == 0x55)
        {
             Electrical_measurement();
             
             sec_flag = 0;
        }
    }
}
*/
