#include "cpu_surport.h"
#include "nidavellir.h"


extern const struct driver_ops_t  CODE eeprom_driver;
extern const struct bus_ops_t iic_bus;
    
struct dev_info_t eeprom_dev=
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

void main(void)
{    
    mcu_init(__NULL);
    
    eeprom_dev.dev_driver_ops->dev_initialize(&eeprom_dev);
    
    eeprom_dev.dev_driver_ops->dev_control(&eeprom_dev , 0 , 0);
}