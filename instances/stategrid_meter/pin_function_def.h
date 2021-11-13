#ifndef __PIN_H__
#define __PIN_H__


#include "target.h"
#include "lib_gpio.h"

#define  BITBAND_ADDR(addr, bitnum)   ((((uint32_t)addr) & 0xF0000000) + \
                                        0x2000000 + ((((uint32_t)addr) & 0xFFFFF) << 5) + (bitnum << 2))

/*--------------EEPROM 对应的电源控制以及IIC总线配置---------------*/
#define EEVDD_DOUT_1()  {*((uint32*)GPIOC_OEN) &= ~BIT(6); *((uint32*)GPIOC_IE) &= ~BIT(6);\
                         *((uint32*)BITBAND_ADDR(GPIOC_DAT,6)) = 1;}

#define EEVDD_DOUT_0()  {*((uint32*)BITBAND_ADDR(GPIOC_DAT,6)) = 0;}
#define SDA_INPUT()     {*((uint32*)GPIOC_IE) |= BIT(5); *((uint32*)GPIOC_OEN) |= BIT(5);}
#define SDA_OUTPUT()    {*((uint32*)GPIOC_OEN) &= ~BIT(5); *((uint32*)GPIOC_IE) &= ~BIT(5);}
#define SCL_OUTPUT()    {*((uint32*)GPIOC_OEN) &= ~BIT(4); *((uint32*)GPIOC_IE) &= ~BIT(4);}
#define SCL_INPUT()     {*((uint32*)GPIOC_IE) |= BIT(4);  *((uint32*)GPIOC_OEN) |= BIT(4);}
#define SDA_GET_DIN()   *((uint32*)BITBAND_ADDR(GPIOC_STS,5))
#define SCL_DOUT_1()    { *((uint32*)BITBAND_ADDR(GPIOC_DAT,4)) = 1;}
#define SCL_DOUT_0()    { *((uint32*)BITBAND_ADDR(GPIOC_DAT,4)) = 0;}
#define SDA_DOUT_1()    { *((uint32*)BITBAND_ADDR(GPIOC_DAT,5)) = 1;}
#define SDA_DOUT_0()    { *((uint32*)BITBAND_ADDR(GPIOC_DAT,5)) = 0;}


/*--------------FLASH 对应的的电源控制以及SPI总线配置---------------*/
#define  FLS_VDD_OPEN()    {*((uint32*)BITBAND_ADDR(GPIOB_DAT,8)) = 0; *((uint32*)GPIOB_OEN) &= ~BIT(8);}
#define  FLS_VDD_CLOSE()   {*((uint32*)BITBAND_ADDR(GPIOB_DAT,8)) = 1; *((uint32*)GPIOB_OEN) &= ~BIT(8);}
                           
#define  DO_CS_LOW()       {*((uint32*)GPIOB_OEN) &= ~BIT(9); *((uint32*)BITBAND_ADDR(GPIOB_DAT,9)) = 0;}
#define  DO_CS_HIGH()      {*((uint32*)GPIOB_OEN) &= ~BIT(9); *((uint32*)BITBAND_ADDR(GPIOB_DAT,9)) = 1;}
#define  DO_SCK_HIGH()     {*((uint32*)GPIOB_OEN) &= ~BIT(10); *((uint32*)BITBAND_ADDR(GPIOB_DAT,10)) = 1;}
#define  DO_SCK_LOW()      {*((uint32*)GPIOB_OEN) &= ~BIT(10); *((uint32*)BITBAND_ADDR(GPIOB_DAT,10)) = 0;}
#define  DO_CLK_HIGH()     {*((uint32*)GPIOB_OEN) &= ~BIT(10); *((uint32*)BITBAND_ADDR(GPIOB_DAT,10)) = 1;}
#define  DO_CLK_LOW()      {*((uint32*)GPIOB_OEN) &= ~BIT(10); *((uint32*)BITBAND_ADDR(GPIOB_DAT,10)) = 0;}
#define  DO_MOSI_HIGH()    {*((uint32*)GPIOB_OEN) &= ~BIT(12); *((uint32*)BITBAND_ADDR(GPIOB_DAT,12)) = 1;}
#define  DO_MOSI_LOW()     {*((uint32*)GPIOB_OEN) &= ~BIT(12); *((uint32*)BITBAND_ADDR(GPIOB_DAT,12)) = 0;}
#define  DO_MISO_RD()      *((uint32*)BITBAND_ADDR(GPIOB_STS,11))
#define  SPI_MISO_IN()     {*((uint32*)GPIOB_IE) |= BIT(11);}
#define  SPI_MOSI_OUT()    {*((uint32*)GPIOB_OEN) &= ~BIT(12);}
#define  SPI_SCK_OUT()     {*((uint32*)GPIOB_OEN) &= ~BIT(10);}
#define  SPI_CS_ATT()      {*((uint32*)GPIOB_ATT) |= BIT(9);}
#define  SPI_CLK_ATT()     {*((uint32*)GPIOB_ATT) |= BIT(10);}
#define  SPI_MOSI_ATT()    {*((uint32*)GPIOB_ATT) |= BIT(12);}                           

/*--------------------------RS485接收控制-------------------------------*/
#define  RS485_RCV_EN()    {*((uint32*)BITBAND_ADDR(GPIOA_DAT,7)) = 1;  \
                           *((uint32*)GPIOA_OEN) &= ~BIT(7);}
#define  RS485_RCV_DIS()   { *((uint32*)BITBAND_ADDR(GPIOA_DAT,7)) = 0;  \
                           *((uint32*)GPIOA_OEN) &= ~BIT(7);}


#define RS485_RX_INPUT()    {*((uint32*)GPIOA_IE) |= BIT(12); *((uint32*)GPIOA_OEN) |= BIT(12);}
#define RS485_TX_OUTPUT()   {*((uint32*)GPIOB_OEN) &= ~BIT(2); *((uint32*)GPIOB_IE) &= ~BIT(2);}
#define RS485_TX_HIGH()     {*((uint32*)BITBAND_ADDR(GPIOB_DAT,2)) = 1;}


                           
/*--------------------------PLC 模块复位-------------------------------*/                           
#define  PLC_RST_EN()      {*((uint32*)BITBAND_ADDR(GPIOC_DAT,9)) = 0;  \
                           *((uint32*)GPIOC_OEN) &= ~BIT(9);}
#define  PLC_RST_DIS()     {*((uint32*)BITBAND_ADDR(GPIOC_DAT,9)) = 1;  \
                           *((uint32*)GPIOC_OEN) &= ~BIT(9);}
                           
/*--------------------------PLC 模块EVENOUT-------------------------------*/                           
#define  PLC_EVENOUT_EN()    {*((uint32*)BITBAND_ADDR(GPIOC_DAT,10)) = 1;  \
                             *((uint32*)GPIOC_OEN) &= ~BIT(10);}
#define  PLC_EVENOUT_DIS()    {*((uint32*)BITBAND_ADDR(GPIOC_DAT,10)) = 0;  \
                             *((uint32*)GPIOC_OEN) &= ~BIT(10);}
                           
/*--------------------------液晶背光控制-------------------------------*/                           
#define  LCD_BACK_LIGHT_OPEN()    {*((uint32*)BITBAND_ADDR(GPIOB_DAT,4)) = 1;  \
                                  *((uint32*)GPIOB_OEN) &= ~BIT(4);}
#define  LCD_BACK_LIGHT_CLOSE()   {*((uint32*)BITBAND_ADDR(GPIOB_DAT,4)) = 0;  \
                                  *((uint32*)GPIOB_OEN) &= ~BIT(4);}
                           
/*--------------------------磁保持继电器控制-------------------------------*/ 
#define  REALY_ON_ACTIVE()    {*((uint32*)BITBAND_ADDR(GPIOE_DAT,11)) = 1;  \
                              *((uint32*)GPIOE_OEN) &= ~BIT(11);}
#define  REALY_ON_DIS_ACTIVE()    {*((uint32*)BITBAND_ADDR(GPIOE_DAT,11)) = 0;  \
                              *((uint32*)GPIOE_OEN) &= ~BIT(11);}
#define  REALY_OFF_ACTIVE()   {*((uint32*)BITBAND_ADDR(GPIOC_DAT,11)) = 1;  \
                              *((uint32*)GPIOC_OEN) &= ~BIT(11);}
#define  REALY_OFF_DIS_ACTIVE()    {*((uint32*)BITBAND_ADDR(GPIOC_DAT,11)) = 0;  \
                                   *((uint32*)GPIOC_OEN) &= ~BIT(11);}

#define  RELAY_CHECK()       *((uint32*)BITBAND_ADDR(GPIOC_STS,8))

#define  RELAY_LED_OPEN()    {*((uint32*)BITBAND_ADDR(GPIOE_DAT,11)) = 1;  \
                             *((uint32*)GPIOE_OEN) &= ~BIT(11);}
                             

/*---------------------------------上翻页键----------------------------------*/                              
#define DISPLAY_KEY_INPUT()     {*((uint32*)GPIOA_IE) |= BIT(2); *((uint32*)GPIOA_OEN) |= BIT(2);}

#define DISPLAY_KEY_GET_DIN()    *((uint32*)BITBAND_ADDR(GPIOA_STS,2))

/*---------------------------------698 ESAM IO----------------------------------*/ 

#define  ESAM_CS_LOW()    {*((uint32*)GPIOC_OEN) &= ~BIT(0); *((uint32*)GPIOC_IE) |= BIT(0); *((uint32*)BITBAND_ADDR(GPIOC_DAT,0)) = 0;}
#define  ESAM_CS_HIGH()   {*((uint32*)GPIOC_OEN) &= ~BIT(0); *((uint32*)GPIOC_IE) |= BIT(0); *((uint32*)BITBAND_ADDR(GPIOC_DAT,0)) = 1;}


#define  ESAM_POWER_CTRL_ON()      {*((uint32*)GPIOC_OEN) &= ~BIT(7); *((uint32*)GPIOC_IE) |= BIT(7);  *((uint32*)BITBAND_ADDR(GPIOC_DAT,7)) = 0;}
#define  ESAM_POWER_CTRL_OFF()     {*((uint32*)GPIOC_OEN) &= ~BIT(7); *((uint32*)GPIOC_IE) |= BIT(7);  *((uint32*)BITBAND_ADDR(GPIOC_DAT,7)) = 1;}




#define  UART_RS4851_AT_MCU_UART_INDEX   0
#define  UART_MODULE_AT_MCU_UART_INDEX   4
#define  UART_IR_AT_MCU_UART_INDEX       5
    

#endif