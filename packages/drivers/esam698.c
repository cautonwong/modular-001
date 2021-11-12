#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "time_link.h"
#include "pin_function_def.h"

#define   ESAM698_MAX_BUF_SIZE    2000

extern const struct bus_ops_t mcu_spi_ops;

extern void power_off_special_hook(void *ptr);


//获取4字节的随机数,用于MAC计算。
const unsigned char  Cmd_Get_ESMA_Rand4[5] = { 0x00, 0x84, 0x00, 0x00, 0x04 };
//表号分散得到临时密钥
const unsigned char  Cmd_ESAM_Key_Scatter[5] = { 0x80, 0xFA, 0x00, 0x03, 0x08 };
//读ESAM 序列号
const unsigned char  Cmd_Get_ESAM_SEQ[5] = { 0x00, 0xB0, 0x99, 0x01, 0x08 };
//ESAM模块加密
const unsigned char  Cmd_Card_ESAM_Encrypt[5] = { 0x80, 0xFA, 0x00, 0x00, 0x08 };

uint8_t esam_test[256];




struct esam_error_apdu_t
{
    uint8  apdu[6];
    uint16 sw;
};

struct esam_error_apdu_t esam_error_apdu;






/*调试阶段暂不进行代码的精简和合并*/

uint8 tx_esam(uint8 *tx_apdu , uint8 *tx_data , uint16 len)
{
    uint8 value = 0;
    uint8 len1 , len2;
    uint8 lrc1 = 0;
    uint16 i;

    
    ESAM_CS_LOW();
    
    len1 = (uint8)(len>>8);
    len2 = (uint8)len;
    mcu_delay_us(120);
    mcu_clear_watchdog();

    DISABLE_ALL_INT();
    
    mcu_spi_ops.tx(0x55 , 0);

    lrc1 = tx_apdu[0] ^ tx_apdu[1] ^ tx_apdu[2] ^ tx_apdu[3];
    for(i=0;i<4;i++)
    {
        value += *tx_apdu;
        mcu_spi_ops.tx(*tx_apdu , 0);
        tx_apdu++;
    }
    lrc1 ^= len1;
    lrc1 ^= len2;

    value += len1;
    value += len2;
    mcu_spi_ops.tx(len1 , 0);
    mcu_spi_ops.tx(len2 , 0);
    
    for(i=0;i<len;i++)
    {
        value += *tx_data;
        lrc1 ^= *tx_data;
        mcu_spi_ops.tx(*tx_data , 0);
        tx_data++;
    }
    mcu_spi_ops.tx(~lrc1 , 0);
    ENABLE_ALL_INT();

    mcu_delay_us(15);
    ESAM_CS_HIGH();
    mcu_delay_us(55);

    return true;
}



uint16 rx_esam(uint8 *rx_data , uint16 *len )
{
    uint8  busy = 0,lrc1 = 0;
    uint8  err,ret[4] = {0},cnt = 0;
    uint16 i,rcv_len;
    int16  timeout;
    uint8  tmp;
    do
    {
        cnt++;
        err     = true;
        timeout = 2000;//1000;
        
        ESAM_CS_LOW();
        
        mcu_delay_us(120);
        //需要循环检查掉电                       int (*rx)(uint8_t* byte , size_t index )
        while((busy != 0x55)&&(timeout--))
        {
            //需要循环检查掉电
            mcu_clear_watchdog();
            mcu_spi_ops.rx(&busy , 0);
            
            if(0x55 == busy)
            {
                DISABLE_ALL_INT();
                for(i=0;i<4;i++)
                {
                    mcu_spi_ops.rx(&ret[i] , 0);
                }
                ENABLE_ALL_INT();
                if ((0x90 == ret[0])&&(0x00 == ret[1]))
                {
                    lrc1 = ret[0] ^ ret[1] ^ ret[2] ^ ret[3];
                    rcv_len = ((uint16)ret[2]<<8) + ret[3];
                    if(rcv_len > ESAM698_MAX_BUF_SIZE)
                    {
                        rcv_len = ESAM698_MAX_BUF_SIZE;
                    }
                    *len = rcv_len;

                    DISABLE_ALL_INT();
                    for(i=0;i<rcv_len;i++)
                    {
                        mcu_spi_ops.rx(rx_data , 0);
                        lrc1 ^= *rx_data++;
                    }
                    ENABLE_ALL_INT();
                    lrc1 = ~lrc1;
                    DISABLE_ALL_INT();
                    mcu_spi_ops.rx(&tmp , 0);
                    if(lrc1 == tmp)
                    {
                        err = false;
                    }
                    else
                    {
                        err = true;
                    }
                    ENABLE_ALL_INT();
                    break;
                }
            }
            mcu_delay_us(120);
        }
        
        ESAM_CS_HIGH();
        mcu_delay_us(20);
    }while((cnt <= 3)&&(err));

    return ((uint16)ret[0]<<8) + ret[1];
}


uint8 read_esam(uint32 addr, uint8 *buf, uint16 len)
{      
    uint16 rx_len, sw = 0;
    
    memset(&esam_error_apdu.apdu[0] , 0xFF , 6);
    
    tx_esam((uint8*)&addr , __NULL , 0);
    
    sw = rx_esam(buf , &rx_len);

    if((0x9000 == sw)&&(rx_len == len))
    {
        return true;
    }
    esam_error_apdu.sw = sw;
    if(sw > 0)
    {
        memcpy(&esam_error_apdu.apdu[0] , (uint8*)&addr , 4);
        memcpy(&esam_error_apdu.apdu[4] , (uint8*)&len , 2);
    }
    
    return false;
}


uint8 write_esam(uint32 addr, uint8 *buf, uint16 len)
{   
    uint16 rx_len, sw = 0;
    
    memset(&esam_error_apdu.apdu[0] , 0xFF , 6);
    
    tx_esam((uint8*)&addr , buf , len);
    
    sw = rx_esam(buf , &rx_len);
    if(0x9000 == sw)
    {
        return true;
    }
    esam_error_apdu.sw = sw;
    if(sw > 0)
    {
        memcpy(&esam_error_apdu.apdu[0] , (uint8*)&addr , 4);
        memcpy(&esam_error_apdu.apdu[4] , (uint8*)&len , 2);
    }
    
    return false;
}




uint8_t read_esam_info(uint8_t *esam_info)
{
    uint8_t apdu[4] = {0x80 , 0x36 , 0x00 , 0xFF };
    
    uint16_t len;
    
    if(true == tx_esam(apdu , __NULL , 0))
    {
        rx_esam(esam_info , &len );
    }
}

void esam_698_power_ctrl(uint8_t power)
{
    if(power)
    {
        ESAM_CS_HIGH();
        ESAM_POWER_CTRL_ON();
        mcu_spi_ops.ctrl(START , 0);
        mcu_spi_ops.init(0);
        
    }
    else
    {
        ESAM_POWER_CTRL_OFF();
        mcu_spi_ops.ctrl(STOP , 0); 
        
    }
}


void esam_698_init(void)
{

    esam_698_power_ctrl(1);
    
    read_esam_info(esam_test);
    
}

