/**
 * @ingroup    drivers_spi_bus
 * @brief      spi 总线驱动
 *
 * @{
 * @file
 *
 */
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "pin_function_def.h"




#define SPI_DELAY()        {__NOP(); __NOP(); __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();}//待调整




int spi_bus_init(size_t index)
{  
    //开漏
    SPI_CS_ATT();
    
    SPI_CLK_ATT();
    
    SPI_MOSI_ATT();
  
    SPI_MISO_IN();
    
    SPI_MOSI_OUT();

    DO_MOSI_HIGH();
    
    SPI_SCK_OUT();
    
    DO_SCK_LOW();
    
    
    
    if (DO_MISO_RD())
    {
        return true;
    }
    
    return false;
}



int spi_send_byte(uint8_t byte  , size_t index)
{
    DO_CLK_HIGH();
    if (byte & 0x80)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x40)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x20)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x10)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x08)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }    
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x04)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }    
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x02)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_LOW();
    SPI_DELAY();
    DO_CLK_HIGH();
    if (byte & 0x01)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    } 
    DO_CLK_LOW();    
    SPI_DELAY();

    return true;
}


int spi_recive_byte(uint8_t* byte , size_t index)
{
    register uint32 rcv_byte = 0;

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x80;
    }
    
    
    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x40;
    }
    

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x20;
    }
    

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x10;
    }
    

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x08;
    }
    

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x04;
    }
    

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x02;
    }
    

    DO_CLK_HIGH();
    SPI_DELAY();
    DO_CLK_LOW();
    if(DO_MISO_RD())
    {
        rcv_byte |= 0x01;
    }
    
    *byte =  (uint8)(rcv_byte & 0x000000FF);
    
    return true;
}






int spi_send_byte_mode3(uint8_t byte  , size_t index)
{
    register uint32 send_byte = byte;
    DO_CLK_LOW();
    if (send_byte & 0x80)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x40)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x20)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x10)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x08)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }    
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x04)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }    
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x02)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    }
    DO_CLK_HIGH();
    DO_CLK_LOW();
    if (send_byte & 0x01)
    {
        DO_MOSI_HIGH();
    }
    else
    {
        DO_MOSI_LOW();
    } 
    DO_CLK_HIGH();
    DO_CLK_LOW();

    return true;
}


int spi_recive_byte_mode3(uint8_t* byte , size_t index)
{
    register uint32 rcv_byte = 0;
    
    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 7);
    DO_SCK_LOW();
 
    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 6);
    DO_SCK_LOW();

    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 5);
    DO_SCK_LOW();
 
    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 4);
    DO_SCK_LOW();

    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 3);
    DO_SCK_LOW();
 
    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 2);
    DO_SCK_LOW();

    DO_SCK_HIGH();
    rcv_byte += (DO_MISO_RD() << 1);
    DO_SCK_LOW();
 
    DO_SCK_HIGH();
    rcv_byte += DO_MISO_RD();
    DO_SCK_LOW();
    
    *byte = rcv_byte & 0x000000ff;
    
    return true;
}




void spi_ctrl(uint8 ctrl , size_t index)
{
    switch(ctrl)
    {
        case START:
              DO_SCK_LOW();
              DO_CS_LOW();
              break;
        case STOP:
              DO_CS_HIGH();
              DO_SCK_LOW();
              break;
        default:
          break;
    }
}

const struct bus_ops_t spi_mode1 = 
{
   BUS_SPI,
   1,
   spi_bus_init,
   __NULL,
   spi_send_byte,
   __NULL,
   spi_recive_byte,
   __NULL,
   __NULL,
   spi_ctrl,
};

const struct bus_ops_t spi_mode3 = 
{
   BUS_SPI,
   1,
   spi_bus_init,
   __NULL,
   spi_send_byte_mode3,
   __NULL,
   spi_recive_byte_mode3,
   __NULL,
   __NULL,
   spi_ctrl,
};

/** @} */