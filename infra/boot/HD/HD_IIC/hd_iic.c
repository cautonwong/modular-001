/*!
 * \file      hd_iic.c
 *
 * \brief     hd_iic implementation
 *
 * \endcode
 *
 * \author	  
 */
#ifndef _HD_IIC_C_
#define _HD_IIC_C_
/* Include ----------------------------------------------------------------------*/
#include "target.h"
#include "HD_Includes.h"
#include "hd_iic_typedef.h"
#include "hd_gpio_list.h"
/* Variables --------------------------------------------------------------------*/

/* Const ------------------------------------------------------------------------*/
const HD_IIC_API_t g_stIICApi = 
{
    HD_IIC_InitPwrOn,
    HD_IIC_InitPwrDown,
    HD_Storage_Access,
};
/* End --------------------------------------------------------------------------*/
void HD_IIC_InitPwrOn(void)
{
    Set_Output_ON;
    IO_ON_H;
    g_stSystemApi.Delay_us(4); //delay 5us
    g_stSystemApi.Delay_us(4); //delay 5us
    g_stSystemApi.Delay_us(4); //delay 5us
    g_stSystemApi.Delay_us(4); //delay 5us
}

void HD_IIC_InitPwrDown(void)
{
    IO_ON_L;
    
}

void IIC_Start(void)
{
    
    SDA_DIR_OUTPUT();
    SCL_DIR_OUTPUT();
    SDA_SET();
    SCL_SET();
    g_stSystemApi.Delay_us(4); //delay 5us
    SDA_CLR();
    g_stSystemApi.Delay_us(4); //delay 5us
    SCL_CLR();
}

void IIC_Stop(void)
{
    SDA_CLR();
    SCL_SET();
    g_stSystemApi.Delay_us(4); //delay 5us
    SDA_SET();
    g_stSystemApi.Delay_us(4); //delay 5us
}

void IIC_Ack(uint8_t Ack)
{
    SCL_CLR();
    g_stSystemApi.Delay_us(4);

    if(1 == Ack)
    {
        SDA_SET();
    }
    else
    {
        SDA_CLR();
    }
    g_stSystemApi.Delay_us(4);

    SCL_SET();
    g_stSystemApi.Delay_us(4);

    SCL_CLR();
    g_stSystemApi.Delay_us(4);
}

uint8_t HD_IICDRV_ByteRead(uint8_t *pData)
{
    uint8_t Index, Result;
    SDA_DIR_INPUT(); 
    SCL_CLR();
    g_stSystemApi.Delay_us(4);
   
    for(Index = 0; Index < 8; Index++ )
    {
        *pData <<= 0x01;
        SCL_SET();
        g_stSystemApi.Delay_us(4);
        if(0 != SDA_GET())
        {
            (*pData)++;
        }
        SCL_CLR();
        g_stSystemApi.Delay_us(4);
    }
    Result = *pData ;
    SDA_DIR_OUTPUT();
   
    return (Result);
}

uint8_t HD_IICDRV_ByteWrite(uint8_t Data)
{
    uint8_t Index, Result;
   
    g_stSystemApi.Delay_us(4);
   
    /* Sending eight bits */
    for(Index = 0; Index < 8; Index++)
    {
        SCL_CLR();
        g_stSystemApi.Delay_us(4);
        if(0x80 & Data)
        {
            SDA_SET(); 
        }
        else
        {
            SDA_CLR(); 
        }
        SCL_SET();
        Data <<= 1;
        g_stSystemApi.Delay_us(4);
    }
    /*Get ACK from slave*/
    SCL_CLR();
    g_stSystemApi.Delay_us(4);
    SDA_DIR_INPUT(); 
    SCL_SET();
    g_stSystemApi.Delay_us(4);
    Result = (SDA_GET() == 0) ? 0 : 1;
    SCL_CLR();
    g_stSystemApi.Delay_us(4);
    SDA_CLR();
    SDA_DIR_OUTPUT();
   
    return (Result);
}

uint8_t HD_IICDRV_BulkWriteData(ST_IIC_WRITR_READ_CONF *pstWrite)
{
    uint16_t i;
   
    /*Send start*/
    IIC_Start();
    /*Send slave address*/
    if(0x01 == HD_IICDRV_ByteWrite(pstWrite->ucDevAddr & 0xFE))
    {
        return (false);
    }
    /*Send data address*/
    for(i = pstWrite->ucOffSetLen; i > 0; i--)
    {
        if(0x01 == HD_IICDRV_ByteWrite((uint8_t)((pstWrite->ulOffset) >> (8 * (i - 1)))))
        {
            return (false);
        }
    }
    /*Send real data*/
    for(i = 0; i < pstWrite->usLen; i++)
    {
        if(0x01 == HD_IICDRV_ByteWrite(*((pstWrite->pData)++)))
        {
            return (false);
        }
    }
    /*Send stop*/
    IIC_Stop();
   
    return (true);
}

uint8_t HD_IICDRV_BulkReadData(ST_IIC_WRITR_READ_CONF * pstRead)
{
    uint16_t i;
    
    /*Send start*/
    IIC_Start();
    /*Send slave address   write*/
    if(0x01 == HD_IICDRV_ByteWrite(pstRead->ucDevAddr & 0xFE))
    {
        return (false);
    }
    /*Send data address*/		
    for(i = pstRead->ucOffSetLen; i > 0; i--)
    {
        if(0x01 == HD_IICDRV_ByteWrite((uint8_t)((pstRead->ulOffset) >> (8 * (i-1)))))
        { 
           return (false);
        }
    }
    /*Send start again*/
    g_stSystemApi.Delay_us(4);
    IIC_Start();
    /*Send slave address   read*/
    if(0x01 == HD_IICDRV_ByteWrite(pstRead->ucDevAddr | 0x01))
    {
        return (false);
    }
    /*Read real data*/
    for(i = 0; i < pstRead->usLen - 1; i++)
    {
        HD_IICDRV_ByteRead((pstRead->pData)++);
    
        /*Send ACK*/
        IIC_Ack(0);
    }
    HD_IICDRV_ByteRead((pstRead->pData)++);
    /*Send NOACK*/
    IIC_Ack(1);
    /*Send stop*/
    IIC_Stop();
    
    return (true);
}

uint8_t HD_EEPROM_Read(uint8_t SlaveAddr, uint32_t Addr, uint8_t *pDataBuf, uint32_t Len)
{
    uint8_t Cnt;
    ST_IIC_WRITR_READ_CONF stRead;
   
    /*Invalid address and length*/
    if(Addr >= EEPROM_DVK_LEN)
    {
        return(false);
    }
    /*Invalid address and length*/
    if((Addr + Len) > EEPROM_DVK_LEN)
    {
        return(false);
    }
    stRead.ulOffset = Addr;
    stRead.pData = pDataBuf;	
    stRead.ucOffSetLen = EEPROM_ADDR_MAX_LEN;
    Cnt = 1;
    if(Addr + Len < EEPROM_PAGE_2_ADDR)
    {
        stRead.ucDevAddr = SlaveAddr;
        stRead.usLen = Len;
    }
    else
    {
        if(Addr <= EEPROM_PAGE_2_ADDR)
        {
            stRead.usLen = EEPROM_PAGE_2_ADDR - Addr;
            stRead.ucDevAddr = SlaveAddr;
            Cnt = 2;
        }
        else
        {
            stRead.ucDevAddr = SlaveAddr + 0x02;
            stRead.usLen = Len;
        }
    }
    if(0 == HD_IICDRV_BulkReadData(&stRead))
    {
        return (false);
    }
    Cnt -= 1;
    if(Cnt > 0)
    {
        stRead.pData = pDataBuf + stRead.usLen;
        stRead.ulOffset = EEPROM_PAGE_2_ADDR;
        stRead.usLen = Len - stRead.usLen;
        stRead.ucDevAddr = SlaveAddr + 0x02;
        if(0 == HD_IICDRV_BulkReadData(&stRead))
        {
            return (false);
        }
    }
   
    return (true);
}

uint8_t HD_EEPROM_Write(uint8_t SlaveAddr, uint32_t Addr, uint8_t *pDataBuf, uint32_t Len)
{
    uint8_t LoopWrite;
    uint32_t max, chunk;
    ST_IIC_WRITR_READ_CONF stWrite;
    
    LoopWrite = 0;
    /*Invalid address and length*/
    if (Addr >= EEPROM_DVK_LEN)
    {
        return (false);
    }
    /*Invalid address and length*/
    if ((Addr + Len) > EEPROM_DVK_LEN)
    {
        return (false);
    }
    /*Disable protection*/
    /* Write max one page at a time */
    while(Len)
    {
        max = EEPROM_PAGE_SIZE - (Addr % EEPROM_PAGE_SIZE);
        if(Len > max)
        {
            chunk = max;
        }
        else
        {
            chunk = Len;
        }
        stWrite.ucDevAddr = SlaveAddr;
        if(Addr >= EEPROM_PAGE_2_ADDR)
        {
            stWrite.ucDevAddr += 0x02;
        }
        stWrite.ulOffset = Addr;
        stWrite.usLen = chunk;
        stWrite.pData = pDataBuf;
        stWrite.ucOffSetLen = EEPROM_ADDR_MAX_LEN;
   
        while(1)
        {
            if(HD_IICDRV_BulkWriteData(&stWrite) == 0)
            {
                /*Most time to write: 3*/
                if(3 > ++LoopWrite)
                {
                    continue;
                }
                return (false);
            }
            break;
        }
        g_stSystemApi.Delay_ms(6);
        /* Update counters etc */
        pDataBuf += chunk;
        Addr += chunk;
        Len -= chunk;
    }
    
    return (true);
}

uint8_t HD_Storage_Access(uint32_t ulAddr, uint8_t *pucDataBuf, uint32_t ulLen, HD_IIC_Operation_e ucOperate)
{
    if(STORAGE_READ == ucOperate) //read
    {
        return (HD_EEPROM_Read(ADDR_EEPROM, ulAddr, pucDataBuf, ulLen));
    }
    else  //write 0x02
    {
        return (HD_EEPROM_Write(ADDR_EEPROM, ulAddr, pucDataBuf, ulLen));
    }
}

#endif
