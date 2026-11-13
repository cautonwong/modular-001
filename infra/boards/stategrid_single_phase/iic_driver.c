
#include "target.h"
#include "core_cm0.h"
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "infra/boards/stategrid_single_phase/iic_pins.h"








#define IIC_DELAY()     {               \
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();\
__NOP();__NOP();__NOP();__NOP();__NOP();}




int iic_bus_init(size_t index)
{
    nd_iic_eeprom_power_set(1);
    IIC_DELAY();
    IIC_DELAY();
    IIC_DELAY();
    nd_iic_sda_output();
    nd_iic_sda_set(1);
    IIC_DELAY();
    nd_iic_scl_set(1);
    nd_iic_scl_output();
    IIC_DELAY();
    return true;
}

void iic_ctrl(uint8_t ctrl , size_t index)
{
    switch(ctrl)
    {
        case START:
          IIC_DELAY();
          nd_iic_sda_set(1);
          nd_iic_sda_output();
          IIC_DELAY();
          nd_iic_scl_set(1);
          nd_iic_scl_output();
          IIC_DELAY();
          nd_iic_sda_set(0);
          IIC_DELAY();
          nd_iic_scl_set(0);
          IIC_DELAY();
          break;
        
        case STOP:
          nd_iic_sda_set(0);
          nd_iic_sda_output();
          IIC_DELAY();
          nd_iic_scl_set(1);
          nd_iic_scl_output();
          IIC_DELAY();
          nd_iic_sda_set(1);
          IIC_DELAY();
          break;
         case ACK_1:
          nd_iic_sda_set(0); 
          nd_iic_sda_output();
          IIC_DELAY();
          nd_iic_scl_set(1);
          nd_iic_scl_output();
          nd_iic_scl_set(0);
          break;
         case ACK_0:
          nd_iic_sda_set(1);
          nd_iic_sda_output();
          IIC_DELAY();
          nd_iic_scl_set(1);
          nd_iic_scl_output();
          nd_iic_scl_set(0);
           break;
    default:
          break;
    }
}



int iic_send_byte(uint8_t Data , size_t index)
{
    register uint8_t i;
        
    nd_iic_scl_set(0);
    nd_iic_sda_output();
    for( i=0;i<8;i++ )
    {
        if( Data & 0x00000080 )
        {
           nd_iic_sda_set(1);
        }
        else
        {
           nd_iic_sda_set(0);
        }
        IIC_DELAY();
        nd_iic_scl_set(1);
        Data=Data<<1;
        IIC_DELAY();
        nd_iic_scl_set(0);
        
    }
    IIC_DELAY();
    nd_iic_sda_input();
    IIC_DELAY();
    nd_iic_scl_set(1);
    IIC_DELAY();
    
    while (nd_iic_sda_read() != 0)
    {
        if (++i >= 200)
        {
            nd_iic_scl_set(0);
            return false;
        }
    }
    nd_iic_scl_set(0);
    IIC_DELAY();
    
    return true;
}


int iic_recive_byte(uint8_t* byte , size_t index)
{
    register uint32_t rcv_data = 0;
    register uint32_t i;
    
    nd_iic_sda_input();
    
    for( i=0;i<8;i++ )
    {
        nd_iic_scl_set(1);
        IIC_DELAY();
        rcv_data = rcv_data<<1;
        if( nd_iic_sda_read() == 1 )
                rcv_data |= 0x01;
        nd_iic_scl_set(0);
        IIC_DELAY();
    }
    
    *byte = (uint8_t)(rcv_data&0x000000FF);
    
    return true;
}

const struct bus_ops_t iic_bus = 
{
    BUS_I2C,
    1,
    iic_bus_init,
    __NULL,
    iic_send_byte,
    __NULL,
    iic_recive_byte,
    __NULL,
    __NULL,
    iic_ctrl,
};



/** @} */