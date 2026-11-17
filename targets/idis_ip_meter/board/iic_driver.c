#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "target.h"
#include "core_cm0.h"
#include "cpu_surport.h"
#include "targets/idis_ip_meter/board/board_transport.h"
#include "targets/idis_ip_meter/board/iic_driver.h"
#include "targets/idis_ip_meter/board/iic_pins.h"


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




#if ND_BOARD_IIC_MODE == ND_BOARD_MODE_POLL


static void iic_ctrl(uint8_t ctrl)
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



static int iic_send_byte(uint8_t data)
{
    register uint8_t i;

    nd_iic_scl_set(0);
    nd_iic_sda_output();
    for( i=0;i<8;i++ )
    {
        if( data & 0x00000080 )
        {
           nd_iic_sda_set(1);
        }
        else
        {
           nd_iic_sda_set(0);
        }
        IIC_DELAY();
        nd_iic_scl_set(1);
        data=data<<1;
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


static void iic_recive_byte(uint8_t *byte)
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
}


void nd_board_iic_init(void)
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
}


int nd_board_iic_write(uint8_t device, const uint8_t *prefix,
                       uint16_t prefix_len, const uint8_t *buf, uint16_t len)
{
    int ok;

    iic_ctrl(START);
    ok = iic_send_byte(device);
    while (ok && prefix_len > 0) {
        ok = iic_send_byte(*prefix);
        prefix++;
        prefix_len--;
    }
    while (ok && len > 0) {
        ok = iic_send_byte(*buf);
        buf++;
        len--;
    }
    iic_ctrl(STOP);
    return ok ? 0 : -1;
}


int nd_board_iic_read(uint8_t device, const uint8_t *prefix,
                      uint16_t prefix_len, uint8_t *buf, uint16_t len)
{
    int ok;

    iic_ctrl(START);
    ok = iic_send_byte(device);
    while (ok && prefix_len > 0) {
        ok = iic_send_byte(*prefix);
        prefix++;
        prefix_len--;
    }
    iic_ctrl(START);
    if (ok) {
        ok = iic_send_byte((uint8_t)(device | 0x01u));
    }
    while (ok && len > 0) {
        uint8_t byte;

        iic_recive_byte(&byte);
        iic_ctrl(len > 1 ? ACK_1 : ACK_0);
        *buf = byte;
        buf++;
        len--;
    }
    iic_ctrl(STOP);
    return ok ? 0 : -1;
}


/** @} */

#else

/*
 * The vendor lib_i2c controller API blocks in a status poll with no timeout
 * and its interrupt entry is not wired. Until that changes there is no IRQ or
 * DMA I2C transport; this build compiles but fails closed.
 */
void nd_board_iic_init(void)
{
}

int nd_board_iic_write(uint8_t device, const uint8_t *prefix,
                       uint16_t prefix_len, const uint8_t *buf, uint16_t len)
{
    (void)device;
    (void)prefix;
    (void)prefix_len;
    (void)buf;
    (void)len;
    return -1;
}

int nd_board_iic_read(uint8_t device, const uint8_t *prefix,
                      uint16_t prefix_len, uint8_t *buf, uint16_t len)
{
    (void)device;
    (void)prefix;
    (void)prefix_len;
    (void)buf;
    (void)len;
    return -1;
}

#endif
