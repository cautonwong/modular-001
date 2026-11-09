/*!
 * \file      hd_spi_hard.c
 *
 * \brief     hd_spi_hard implementation
 *
 * \endcode
 *
 * \author	  hxiii
 */
#ifndef _HD_SPI_HARD_C_
#define _HD_SPI_HARD_C_
/* Include ----------------------------------------------------------------------*/
#include "target.h"
#include "HD_Includes.h"
#include "hd_spi_hard_typedef.h"
#include "hd_gpio_list.h"
#include "lib_spi.h"
#include "hd_flash_map.h"

/* Variables --------------------------------------------------------------------*/

/* Const ------------------------------------------------------------------------*/
const HD_SPI_HARD_API_t spi_hw_api = 
{
    hd_spi_hw_init_pwron,
    hd_spi_hw_init_pwroff,
    hd_spi_hw_write,
    hd_spi_hw_read,
};
/* End --------------------------------------------------------------------------*/
void hd_spi_hw_init_pwron(void)
{
    SPI_InitType InitStruct;
    
    /* dataflash---------------------------------------*/
    SPI_DeviceInit(SPI1);
    InitStruct.SPH = SPI_SPH_0;
    InitStruct.SPO = SPI_SPO_0;
    InitStruct.ClockDivision = SPI_CLKDIV_2;
    InitStruct.Mode = SPI_MODE_MASTER;
    InitStruct.CSNSoft = SPI_CSNSOFT_ENABLE;
    InitStruct.SWAP = SPI_SWAP_DISABLE;
    SPI_Init(SPI1, &InitStruct);
    
    SPI_TransmitFIFOLevelConfig(SPI1, SPI_TXFLEV_0);
    SPI_ReceiveFIFOLevelConfig(SPI1, SPI_RXFLEV_0);

    SPI_INTConfig(SPI1, SPI_INT_TX | SPI_INT_RX, DISABLE);

    SPI_Cmd(SPI1, ENABLE);

    SPI_CS_HIGH();
    
}

void hd_spi_hw_init_pwroff(void)
{
    SPI_DeviceInit(SPI1);
    SPI_Cmd(SPI1, DISABLE);
}

uint8_t hd_spi_hw_wr_word(uint8_t data)
{
    uint8_t data_read;
    
    data_read = 0;
    while(0 == (SPI1->MISC_ & SPI_STS_TFE));
    SPI1->TXDAT = data;
    while(0 == (SPI1->MISC_ & SPI_MISC_RNE));
    data_read = SPI1->RXDAT;
    
    return (data_read);
}

uint8_t hd_spi_hw_write_n_bytes(uint8_t *p_data, uint32_t addr, uint16_t data_len)
{
    uint16_t cnt;
    
    /*step 1. enable write-----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE_ENABLE);
    SPI_CS_HIGH();
    /*step 2. write data-------------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE);
    hd_spi_hw_wr_word((uint8_t)(addr >> 16));
    hd_spi_hw_wr_word((uint8_t)(addr >> 8));
    hd_spi_hw_wr_word((uint8_t)addr);
    for(cnt = 0; cnt < data_len; cnt++)
    {
        hd_spi_hw_wr_word(p_data[cnt]);
    }
    SPI_CS_HIGH();
    /*step 3. disable write----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE_DISABLE);
    SPI_CS_HIGH();
    /*step 4. delay for write finished-----------*/
    g_stSystemApi.Delay_ms(3); //typical 0.7ms, max 3ms
    
    return (1);
}

int8_t hd_spi_hw_read_n_bytes(uint8_t *p_data, uint32_t addr, uint16_t data_len)
{
    uint16_t cnt;
    
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_READ);
    hd_spi_hw_wr_word((uint8_t)(addr >> 16));
    hd_spi_hw_wr_word((uint8_t)(addr >> 8));
    hd_spi_hw_wr_word((uint8_t)addr);
    for(cnt = 0; cnt < data_len; cnt++)
    {
        p_data[cnt] = hd_spi_hw_wr_word(0xff);
    }
    SPI_CS_HIGH();
  
    return (1);
}

uint8_t hd_spi_hw_sector_erase(uint32_t addr)
{
    if(addr % EX_FLASH_SECTOR_SIZE)
    {
        return (0);
    }
    /*step 1. enable write-----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE_ENABLE);
    SPI_CS_HIGH();
    /*step 2. erase sector-----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_SECTOR_ERASE);
    hd_spi_hw_wr_word((uint8_t)(addr >> 16));
    hd_spi_hw_wr_word((uint8_t)(addr >> 8));
    hd_spi_hw_wr_word((uint8_t)addr);
    SPI_CS_HIGH();
    /*step 3. disable write----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE_DISABLE);
    SPI_CS_HIGH();
    /*step 4. delay for write finished-----------*/
    g_stSystemApi.Delay_ms(50); //typical 50ms
    
    return (1);
}

uint8_t hd_spi_hw_block_erase(uint32_t addr)
{
    if(addr % EX_FLASH_SECTOR_SIZE)
    {
        return (0);
    }
    /*step 1. enable write-----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE_ENABLE);
    SPI_CS_HIGH();
    /*step 2. erase sector-----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_BLOCK64K_ERASE);
    hd_spi_hw_wr_word((uint8_t)(addr >> 16));
    hd_spi_hw_wr_word((uint8_t)(addr >> 8));
    hd_spi_hw_wr_word((uint8_t)addr);
    SPI_CS_HIGH();
    /*step 3. disable write----------------------*/
    SPI_CS_LOW();
    hd_spi_hw_wr_word(CMD_WRITE_DISABLE);
    SPI_CS_HIGH();
    /*step 4. delay for write finished-----------*/
    g_stSystemApi.Delay_ms(300);  
    
    return (1);
}

uint8_t hd_spi_hw_write(uint32_t addr, uint8_t *p_data, uint16_t data_len, HD_Spi_Hw_Erase_e erase_flag)
{
    uint16_t page_left_len, offset, write_len;
    
    if(addr > EX_FLASH_DVK_LEN)
    {
        return (0);
    }
    
    offset = 0;
    SPI_WP_LOW();
    while(data_len > 0)
    {
        offset = addr % EX_FLASH_SECTOR_SIZE;
        if((0 == offset) && (HD_FLASH_ERASE ==  erase_flag))
        {
            hd_spi_hw_sector_erase(addr);
        }
        offset = addr % EX_FLASH_PAGE_SIZE;
        page_left_len = EX_FLASH_PAGE_SIZE - offset;
        write_len = data_len > page_left_len ? page_left_len : data_len;
        data_len -= write_len;
        hd_spi_hw_write_n_bytes(p_data, addr, write_len);
        addr += write_len;
        p_data += write_len;
    }
    SPI_WP_HIGH();
    
    return (1);
}

uint8_t hd_spi_hw_read(uint32_t addr, uint8_t *p_data, uint16_t data_len)
{
    if(addr > EX_FLASH_DVK_LEN)
    {
        return (0);
    }
    hd_spi_hw_read_n_bytes(p_data, addr, data_len);
    
    return (1);
}

#define STOR_GET_PHY_ADDR(addr)       (addr & 0x0fffffff)

uint8_t DF_Storage_Access(uint32_t ulAddr, uint8_t *pucDataBuf, uint32_t ulLen, uint8_t ucOperate)
{
    uint32_t addr_phy = STOR_GET_PHY_ADDR(ulAddr);
    if(STORAGE_READ == ucOperate) //read
    {
        return spi_hw_api.read(addr_phy, pucDataBuf, ulLen);
    }
    else  //write 0x02
    {
        return spi_hw_api.write(addr_phy, pucDataBuf, ulLen, HD_FLASH_ERASE);
    }
}
#if 0
uint8_t pucDataBuf[4] = {0x55, 0xaa, 0x55, 0xaa};
void FlashMemory_test()
{
    uint8_t ret = 0;
    for(int i=0;i<8;i++){
        ret = HD_DF_Storage_Access(0, pucDataBuf, 4, STORAGE_WRITE);
        pucDataBuf[0]=0;
        pucDataBuf[1]=0;
        pucDataBuf[2]=0;
        pucDataBuf[3]=0;
        ret = HD_DF_Storage_Access(0, pucDataBuf, 4, STORAGE_READ);
        pucDataBuf[0]=i;
        pucDataBuf[1]=i+1;
        pucDataBuf[2]=i+2;
        pucDataBuf[3]=i+3;
    }
}
#endif
#endif
