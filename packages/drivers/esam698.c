#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "time_link.h"
#include "pin_function_def.h"
#include "fr_698_esam.h"

#define   ESAM698_MAX_BUF_SIZE    2000

extern const struct bus_ops_t mcu_spi_ops;

extern void power_off_special_hook(void *ptr);

struct io_hal_t
{
     void *gpio; 
     uint32_t pin;
     uint8_t  dir_mode;
     uint8_t open_state;
};

struct esam_hal_t
{
    const struct io_hal_t *power;
    const struct io_hal_t *cs;
    uint8_t      spi_NO;
};

//获取4字节的随机数,用于MAC计算。
const unsigned char  Cmd_Get_ESMA_Rand4[5] = { 0x00, 0x84, 0x00, 0x00, 0x04 };
//表号分散得到临时密钥
const unsigned char  Cmd_ESAM_Key_Scatter[5] = { 0x80, 0xFA, 0x00, 0x03, 0x08 };
//读ESAM 序列号
const unsigned char  Cmd_Get_ESAM_SEQ[5] = { 0x00, 0xB0, 0x99, 0x01, 0x08 };
//ESAM模块加密
const unsigned char  Cmd_Card_ESAM_Encrypt[5] = { 0x80, 0xFA, 0x00, 0x00, 0x08 };




struct esam_error_apdu_t
{
    uint8  apdu[6];
    uint16 sw;
};

struct esam_error_apdu_t esam_error_apdu;






/*调试阶段暂不进行代码的精简和合并*/

static uint8 tx_esam(uint8 *tx_apdu , uint8 *tx_data , uint16 len , const struct esam_hal_t *esam_hal)
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



static uint16 rx_esam(uint8 *rx_data , uint16 *len , const struct esam_hal_t *esam_hal , uint16_t *sw)
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




uint8_t f_esam_type_get_esam_all_info1(struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4] = {0x80, 0x36, 0x00, 0xff};
    uint8_t tx_data;
    uint16_t len , sw;
    uint8_t ret;
    
    tx_esam(head , &tx_data , 0 , esam_hal);

    ret = rx_esam(esam_out->data , &len , esam_hal , &sw);
		
    return ret;
}

uint8_t f_esam_type_get_esam_all_info2(struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4] = {0x80, 0x36, 0x00, 0xfe};
    uint8_t tx_data;
    uint16_t len , sw;
    uint8_t ret;
    
    tx_esam(head , &tx_data , 0 , esam_hal);

    ret = rx_esam(esam_out->data , &len , esam_hal , &sw);
		
    return ret;
}

uint8_t f_esam_type_meet_consult(uint8_t type , struct esam_698_input_t *esam_in , struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4] = {0x81, 0x02, 0x00, 0x00};
    uint8_t com_data[68];
    uint16_t len , sw;
    uint8_t ret;
    
    if(type == 1)
    {
	head[0] = 0x82;
    }
		
    memcpy(com_data , esam_in->data , 32);
    memcpy(&com_data[32] , esam_in->MAC_data , 4);
		
    tx_esam(head , com_data , 36 , esam_hal);

    ret = rx_esam(com_data , &len , esam_hal , &sw);
		
    if(type == 1)
    {
	memcpy(esam_out->data, com_data , 64);
				
	memcpy(esam_out->MAC_data, &com_data[64]  , 4);
    }
    else
    {
	memcpy(esam_out->data, com_data , 48);
				
	memcpy(esam_out->MAC_data, &com_data[48]  , 4);
    }
    
    return ret;
}



uint8_t f_esam_type_read_meter(uint8_t type , struct esam_698_input_t *esam_in , struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4] = {0x80, 0x0E, 0x40, 0x02};
    uint16_t len , sw;
    uint8_t ret;
    
    if(type == 0)
    {
	head[1] = 0x0E;  //电表计算明文+MAC
    }
    else if(type == 1)
    {
	head[1] = 0x0A;  //电表计算密文
    }
    else
    {
	head[1] = 0x10;  //电表计算密文+MAC
    }

    tx_esam(head , esam_in->data , esam_in->data_len , esam_hal);

    ret = rx_esam(esam_in->data_buf , &len , esam_hal , &sw);
		
    if(type == 0)
    {
	memcpy(esam_out->MAC_data, esam_in->data_buf , 4); //电表计算明文+MAC
    }
    else if(type == 1)
    {
	memcpy(esam_out->data, esam_in->data_buf , len); //电表计算密文
    }
    else
    {
	memcpy(esam_out->data, esam_in->data_buf , len-4); //电表计算密文
	memcpy(esam_out->MAC_data, &esam_in->data_buf[len-4] , 4); //电表计算密文+MAC
    }

    return ret;
}

uint8_t f_esam_type_broadcast(struct esam_698_input_t *esam_in , struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4];
    uint16_t len = 0 , sw = 0;
    uint8_t ret = 0;
	
    memcpy(head , (uint8_t *)&esam_in->sid_id , 4);
    memcpy(esam_in->data_buf , (uint8_t *)&esam_in->AttachData , esam_in->AttachData_len);
    len = esam_in->AttachData_len;
    memcpy(&esam_in->data_buf[len] , (uint8_t *)&esam_in->data , esam_in->data_len);
    len += esam_in->data_len;
    memcpy(&esam_in->data_buf[len] , (uint8_t *)&esam_in->MAC_data , esam_in->MAC_len);
    len += esam_in->MAC_len;
    tx_esam(head , esam_in->data_buf , len , esam_hal);
    ret = rx_esam(esam_in->data_buf , &len , esam_hal , &sw);
    if(sw == 0x9000)
    {
	memcpy(esam_out->data , esam_in->data_buf , len);
	esam_out->data_len = len;
    }
    return ret;
}


uint8_t f_esam_type_point_get_data2(uint8_t type , struct esam_698_input_t *esam_in , struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4];
    uint16_t len = 0 , sw = 0;
    uint8_t ret = 0;
	
    memcpy(head , (uint8_t *)&esam_in->sid_id , 4);
    memcpy(esam_in->data_buf , (uint8_t *)&esam_in->AttachData , esam_in->AttachData_len);
    len = esam_in->AttachData_len;
    memcpy(&esam_in->data_buf[len] , (uint8_t *)&esam_in->data , esam_in->data_len);
    len += esam_in->data_len;
    if(type == 1)//数据验证信息为SID_MAC
    {
        memcpy(&esam_in->data_buf[len] , (uint8_t *)&esam_in->MAC_data , esam_in->MAC_len);
        len += esam_in->MAC_len;
    }
    tx_esam(head , esam_in->data_buf , len , esam_hal);
    ret = rx_esam(esam_in->data_buf , &len , esam_hal , &sw);
    if(sw == 0x9000)
    {
         memcpy(esam_out->data , esam_in->data_buf , len);
         esam_out->data_len = len;
    }
    return ret;
}

#define HES_PLAINTEXT_MAC     0x11
#define HES_CIPHERTEXT        0xA6
#define HES_CIPHERTEXT_MAC    0xA7

uint8_t f_esam_hes_point_get_data4(uint8_t p2 , struct esam_698_input_t *esam_in , struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4] = {0x81,0x1c,0x00,0x00};
    uint16_t len = 0 , sw = 0;
    uint8_t ret = 0;
	
    head[3] = p2;
    memcpy(esam_in->data_buf , (uint8_t *)&esam_in->data , esam_in->data_len);
    len = esam_in->data_len;
    tx_esam(head , esam_in->data_buf , len , esam_hal);
    ret = rx_esam(esam_in->data_buf , &len , esam_hal , &sw);
    if(sw == 0x9000)
    {
         memcpy(esam_out->data , esam_in->data_buf , len);
         esam_out->data_len = len;
    }
    return ret;
}


uint8_t f_esam_type_read_esam_data(struct esam_698_input_t *esam_in , struct esam_698_output_t *esam_out , const struct esam_hal_t *esam_hal)
{
    uint8_t head[4];
    uint16_t len = 0 , sw = 0;
    uint8_t ret = 0;
    
    memcpy(head , (uint8_t *)&esam_in->sid_id , 4);
    memcpy(esam_in->data_buf , (uint8_t *)&esam_in->AttachData , esam_in->AttachData_len);
    len = esam_in->AttachData_len;
    tx_esam(head , esam_in->data_buf , len , esam_hal);
    ret = rx_esam(esam_in->data_buf , &len , esam_hal , &sw);
    if(sw == 0x9000)
    {
        memcpy(esam_out->data , esam_in->data_buf , len);
	esam_out->data_len = len;
    }
    return ret;
}



static void esam_698_power_ctrl(uint8_t power)
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






void GetKeyStatus(struct instance_t* instance, struct pipeline_t* pipeline)
{	
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)pipeline->ctx;
	uint8_t Buf[16];
    uint32_t keystate=0;
	
	read_esam_info(Buf,0X04);//读密钥状态
    memcpy((uint8_t*)&keystate, (uint8_t*)&Buf[0], 4);

	if( (keystate >= 0x7FFFFFFF) && (Buf[4] >= 0xF0) ) 
        mbase_control_block->key_state = false; //修改35条密钥以上，为私钥状态
	else 
        mbase_control_block->key_state = true;
		
}
