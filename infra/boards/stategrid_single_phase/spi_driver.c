/**
 * @ingroup    drivers_spi_bus
 * @brief      spi 总线驱动
 *
 * @{
 * @file
 *
 */
#include "target.h"
#include "core_cm0.h"
#include "cpu_surport.h"
#include "nidavellir.h"
#include "type_def.h"
#include "infra/boards/stategrid_single_phase/spi_pins.h"




#define SPI_DELAY()        {__NOP(); __NOP(); __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();}




int spi_bus_init(size_t index)
{  
    //开漏
    nd_spi_cs_attention();
    
    nd_spi_clk_attention();
    
    nd_spi_mosi_attention();
  
    nd_spi_miso_input();
    
    nd_spi_mosi_output();

    nd_spi_mosi_set(1);
    
    nd_spi_sck_output();
    
    nd_spi_sck_set(0);
    
    
    
    if (nd_spi_miso_read())
    {
        return true;
    }
    
    return false;
}



int spi_send_byte(uint8_t byte  , size_t index)
{
    nd_spi_sck_set(1);
    if (byte & 0x80)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x40)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x20)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x10)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x08)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }    
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x04)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }    
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x02)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(0);
    SPI_DELAY();
    nd_spi_sck_set(1);
    if (byte & 0x01)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    } 
    nd_spi_sck_set(0);    
    SPI_DELAY();

    return true;
}


int spi_recive_byte(uint8_t* byte , size_t index)
{
    register uint32_t rcv_byte = 0;

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x80;
    }
    
    
    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x40;
    }
    

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x20;
    }
    

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x10;
    }
    

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x08;
    }
    

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x04;
    }
    

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x02;
    }
    

    nd_spi_sck_set(1);
    SPI_DELAY();
    nd_spi_sck_set(0);
    if(nd_spi_miso_read())
    {
        rcv_byte |= 0x01;
    }
    
    *byte =  (uint8_t)(rcv_byte & 0x000000FF);
    
    return true;
}






int spi_send_byte_mode3(uint8_t byte  , size_t index)
{
    register uint32_t send_byte = byte;
    nd_spi_sck_set(0);
    if (send_byte & 0x80)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x40)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x20)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x10)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x08)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }    
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x04)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }    
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x02)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    }
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);
    if (send_byte & 0x01)
    {
        nd_spi_mosi_set(1);
    }
    else
    {
        nd_spi_mosi_set(0);
    } 
    nd_spi_sck_set(1);
    nd_spi_sck_set(0);

    return true;
}


int spi_recive_byte_mode3(uint8_t* byte , size_t index)
{
    register uint32_t rcv_byte = 0;
    
    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 7);
    nd_spi_sck_set(0);
 
    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 6);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 5);
    nd_spi_sck_set(0);
 
    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 4);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 3);
    nd_spi_sck_set(0);
 
    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 2);
    nd_spi_sck_set(0);

    nd_spi_sck_set(1);
    rcv_byte += (nd_spi_miso_read() << 1);
    nd_spi_sck_set(0);
 
    nd_spi_sck_set(1);
    rcv_byte += nd_spi_miso_read();
    nd_spi_sck_set(0);
    
    *byte = rcv_byte & 0x000000ff;
    
    return true;
}




void spi_ctrl(uint8_t ctrl , size_t index)
{
    switch(ctrl)
    {
        case START:
              nd_spi_sck_set(0);
              nd_spi_cs_set(0);
              break;
        case STOP:
              nd_spi_cs_set(1);
              nd_spi_sck_set(0);
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