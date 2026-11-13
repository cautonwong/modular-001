#include "type_def.h"
#include "nidavellir.h"
#include "cpu_surport.h"
#include "time_link.h"
#include "infra/boards/stategrid_single_phase/esam_pins.h"
#include "fr_698_esam.h"
#include "io_type.h"
#include "lib_ana.h"


#define   ESAM698_MAX_BUF_SIZE    2000

extern const struct bus_ops_t mcu_spi_ops;


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

static uint8_t esam_chip_abnormal = EVENT_RESET;

static uint8_t tx_esam(uint8_t *tx_apdu , uint8_t *tx_data , uint16_t len , const struct esam_hal_t *esam_hal)
{
    uint8_t value = 0;
    uint8_t len1 , len2;
    uint8_t lrc1 = 0;
    uint16_t i;

    
    nd_esam_cs_set(0);
    
    len1 = (uint8_t)(len>>8);
    len2 = (uint8_t)len;
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
    nd_esam_cs_set(1);
    mcu_delay_us(55);

    return true;
}



static uint16_t rx_esam(uint8_t *rx_data , uint16_t *len , const struct esam_hal_t *esam_hal , uint16_t *sw)
{
    uint8_t  busy = 0,lrc1 = 0;
    uint8_t  err,ret[4] = {0},cnt = 0;
    uint16_t i,rcv_len;
    int16_t  timeout;
    uint8_t  tmp;
    do
    {
        cnt++;
        err     = true;
        timeout = 2000;//1000;
        
        nd_esam_cs_set(0);
        
        mcu_delay_us(120);
        //需要循环检查掉电                       int (*rx)(uint8_t* byte , size_t index )
        while((busy != 0x55)&&(timeout--))
        {
            if (ANA_GetStatus(ANA_STATUS_VDCINDROP))    
            {
                return false;   //需要循环检查掉电
            }
                
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
                    rcv_len = ((uint16_t)ret[2]<<8) + ret[3];
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
        
        nd_esam_cs_set(1);
        mcu_delay_us(50);
    }while((cnt <= 3)&&(err));
    *sw = ((uint16_t)ret[0]<<8) + ret[1];
    if(cnt < 3)
    {
        esam_chip_abnormal = EVENT_RESET;
        return true;
    }
    else
    {
        esam_chip_abnormal = EVENT_SET;
        return false;
    }
}


static int _esam_power_ctrl_on(struct esam_data_t *para, void *esam_hal)
{
    nd_esam_cs_set(1);
    nd_esam_power_set(1);
    mcu_spi_ops.ctrl(START , 0);
    mcu_spi_ops.init(0);
    //补充 esam电源打开后的 100ms延时
    mcu_delay_ms_power_down_quit(100);
    
    return 1;
}

static int _esam_power_ctrl_off(struct esam_data_t *para, void *esam_hal)
{
    nd_esam_power_set(0);
    mcu_spi_ops.ctrl(STOP , 0);    
    return 1;
}


static int _esam_comm_handle(struct esam_data_t *para, void *esam_hal)
{
    struct esam_698_input_t *esam_in = para->input;
    struct esam_698_output_t *esam_out = para->output;
    
    tx_esam(para->head , esam_in->data_buf , para->len , NULL);
    rx_esam(esam_in->data_buf , &para->len , NULL , &para->sw);
    
    if(para->sw == 0x9000){
        return 1;
    }
    
    if(esam_out)
    {
        esam_out->esam_chip_abnormal = esam_chip_abnormal;
    }
    return -1;
}

struct func_esam_t
{
    int(*pFunc)(struct esam_data_t *para, void *esam_hal);
};

static const struct func_esam_t Func_esam_v2[] = 
{
    {_esam_power_ctrl_on},
    {_esam_power_ctrl_off},
    {_esam_comm_handle},
};


int openM_ioctl(int fd, int request, void *param)
{
    if((request < esam_type_max_cmd) && (request >= esam_type_base)){
        return Func_esam_v2[request - esam_type_base].pFunc((struct esam_data_t *)param, 0);
    }
    return -1;
}
