
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "pin_function_def.h"








#define IIC_DELAY()     {               \
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
    EEVDD_DOUT_1();
    IIC_DELAY();
    IIC_DELAY();
    IIC_DELAY();
    SDA_OUTPUT();
    SDA_DOUT_1();
    IIC_DELAY();
    SCL_DOUT_1();
    SCL_OUTPUT();
    IIC_DELAY();
    return true;
}

void iic_ctrl(uint8 ctrl , size_t index)
{
    switch(ctrl)
    {
        case START:
          IIC_DELAY();
          SDA_DOUT_1();
          SDA_OUTPUT();
          IIC_DELAY();
          SCL_DOUT_1();
          SCL_OUTPUT();
          IIC_DELAY();
          SDA_DOUT_0();
          IIC_DELAY();
          SCL_DOUT_0();
          IIC_DELAY();
          break;
        
        case STOP:
          SDA_DOUT_0();
          SDA_OUTPUT();
          IIC_DELAY();
          SCL_DOUT_1();
          SCL_OUTPUT();
          IIC_DELAY();
          SDA_DOUT_1();
          IIC_DELAY();
          break;
         case ACK_1:
          SDA_DOUT_0(); 
          SDA_OUTPUT();
          IIC_DELAY();
          SCL_DOUT_1();
          SCL_OUTPUT();
          SCL_DOUT_0();
          break;
         case ACK_0:
          SDA_DOUT_1();
          SDA_OUTPUT();
          IIC_DELAY();
          SCL_DOUT_1();
          SCL_OUTPUT();
          SCL_DOUT_0();
           break;
    default:
          break;
    }
}



int iic_send_byte(uint8 Data , size_t index)
{
    register uint8_t i,ACK;
        
    SCL_DOUT_0();
    SDA_OUTPUT();
    for( i=0;i<8;i++ )
    {
        if( Data & 0x00000080 )
        {
           SDA_DOUT_1();
        }
        else
        {
           SDA_DOUT_0();
        }
        IIC_DELAY();
        SCL_DOUT_1();
        Data=Data<<1;
        SCL_DOUT_0();
        IIC_DELAY();
    }
    
    SDA_INPUT();
    IIC_DELAY();
    SCL_DOUT_1();
    IIC_DELAY();
    
    while (SDA_GET_DIN() != 0)
    {
        if (++i >= 200)
        {
            SCL_DOUT_0();
            return false;
        }
    }
    SCL_DOUT_0();
    IIC_DELAY();
    
    return true;
}


int iic_recive_byte(uint8_t* byte , size_t index)
{
    register uint32 rcv_data = 0;
    register uint32 i;
    
    SDA_INPUT();
    
    for( i=0;i<8;i++ )
    {
        SCL_DOUT_1();
        IIC_DELAY();
        rcv_data = rcv_data<<1;
        if( SDA_GET_DIN() == 1 )
                rcv_data |= 0x01;
        SCL_DOUT_0();
        IIC_DELAY();
    }
    
    *byte = (uint8)(rcv_data&0x000000FF);
    
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