/**
 * @ingroup    drivers_flashdev
 * @brief      nor flash 驱动
 *
 * @{
 * @file
 *
 */
#include "includes.h"

#include "target.h"
#include "HD_Includes.h"
#include "hd_gpio_list.h"
#include "lib_spi.h"
#include "spi_driver.h"
__IO unsigned long nTicks;



#define  PAGE_BLOCK         4096 

/**
  * @brief  Get current ticks. 
  * @param  None
  * @retval None
  */
unsigned long System_GetTick(void)
{
  return nTicks;
}


/**
  * @brief  SPI De-Initialization for flash memory device. 
  * @param  None
  * @retval None
  */
void FlashMem_SPI_DeInit(void)
{
  GPIO_InitType GPIO_InitStruct;
  
  SysTick->CTRL = 0;

  SPI_DeviceInit(SPI_Flash);
  
  /* CSN pin forbidden */
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_FORBIDDEN;
  GPIO_InitStruct.GPIO_Pin = CSN_Pin;
  GPIOBToF_Init(CSN_GPIO, &GPIO_InitStruct);
}

/**
  * @brief  SPI configure CSN pin. 
  */
void FlashMem_SPI_SetCSN(unsigned char level)
{
  if (level)    GPIOBToF_SetBits(CSN_GPIO, CSN_Pin);
  else          GPIOBToF_ResetBits(CSN_GPIO, CSN_Pin);
}

/**
  * @brief  SPI transfer 
  * @param  
  * @retval 
  */
signed long FlashMem_SPI_Transfer(unsigned char *w_data, unsigned char *r_data, unsigned long len, unsigned long Timeout)
{
  unsigned long tick_cur, Tickstart;
  unsigned long i;
  
  Tickstart = System_GetTick();
  
  for (i=0; i<len; i++)
  {
    /* While until TX FIFO is empty */
    while (SPI_GetStatus(SPI_Flash, SPI_STS_TXEMPTY) == 0U)
    {
      if (Timeout == 0UL)    return -1;
      else
      {
        tick_cur = System_GetTick();
        if (tick_cur >= Tickstart)
        {
          if ((tick_cur-Tickstart) > Timeout)    return -1;
        }
        else
        {
          if ((0xFFFFFFFFUL-Tickstart+tick_cur) > Timeout)    return -1;
        }
      }  
    }
    if (w_data == NULL)
      SPI_SendData(SPI_Flash, 0x00);
    else
      SPI_SendData(SPI_Flash, w_data[i]);
    
    /* Wait until receive FIFO is not empty, receive one byte data */
    while (SPI_GetStatus(SPI_Flash, SPI_STS_RNE) == 0U)
    {
      if (Timeout == 0UL)    return -1;
      else
      {
        tick_cur = System_GetTick();
        if (tick_cur >= Tickstart)
        {
          if ((tick_cur-Tickstart) > Timeout)    return -1;
        }
        else
        {
          if ((0xFFFFFFFFUL-Tickstart+tick_cur) > Timeout)    return -1;
        }
      }  
    }
    if (r_data == NULL)
      SPI_ReceiveData(SPI_Flash);
    else
      r_data[i] = SPI_ReceiveData(SPI_Flash);
  }
  
  while (SPI_GetStatus(SPI_Flash, SPI_STS_BSY) == 1U)
  {
    if (Timeout == 0UL)    return -1;
    else
    {
      tick_cur = System_GetTick();
      if (tick_cur >= Tickstart)
      {
        if ((tick_cur-Tickstart) > Timeout)    return -1;
      }
      else
      {
        if ((0xFFFFFFFFUL-Tickstart+tick_cur) > Timeout)    return -1;
      }
    }    
  }
  
  return 0;
}

/**
  * @brief  Write enable 
  * @param  None
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_WriteEnable(void)
{
  unsigned char tmp;
  signed long retval;

  tmp = FM25Q_WriteEnable;
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(&tmp, NULL, 1, 2);
  FlashMem_SPI_SetCSN(1);

  return retval;
}

/**
  * @brief  Write disbale 
  * @param  None
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_WriteDisable(void)
{
  unsigned char tmp;
  signed long retval;

  tmp = FM25Q_WriteDisable;
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(&tmp, NULL, 1, 2);
  FlashMem_SPI_SetCSN(1);

  return retval;
}

/**
  * @brief  Read status register 1
  * @param  [out]data: The pointer of read data
  * @note   S7    S6   S5  S4   S3   S2   S1   S0
  *         SRP0  SEC  TB  BP2  BP1  BP0  WEL  WIP
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_ReadStaReg_1(unsigned char *data)
{
  unsigned char wBuffer[2] = { FM25Q_ReadStatusReg_1, 0x00 };
  unsigned char rBuffer[2];
  signed long retval;

  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, rBuffer, 2, 2);
  FlashMem_SPI_SetCSN(1);

  *data = rBuffer[1];
  return retval;
}

/**
  * @brief  Read status register 2
  * @param  [out]data: The pointer of read data
  * @note   S15   S14   S13  S12   S11   S10   S9   S8
  *         SUS   CMP   LB3  LB2   LB1   LB0   QE   SRP1
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_ReadStaReg_2(unsigned char *data)
{
  unsigned char wBuffer[2] = { FM25Q_ReadStatusReg_2, 0x00 };
  unsigned char rBuffer[2];
  signed long retval;

  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, rBuffer, 2, 2);
  FlashMem_SPI_SetCSN(1);

  *data = rBuffer[1];
  return retval;
} 

/**
  * @brief  Wait for idle
  */
signed long FlashMem_WaitForIdle(unsigned long Tickstart, unsigned long Timeout)
{
  unsigned char data;
  unsigned long tick_cur;

  do 
  {
    FlashMem_ReadStaReg_1(&data);

    if (Timeout == 0UL)
    {
      return -1;
    }
    else
    {
      tick_cur = System_GetTick();
      if (tick_cur >= Tickstart)
      {
        if ((tick_cur-Tickstart) > Timeout)
        {
          return -1;
        }
      }
      else
      {
        if ((0xFFFFFFFFUL-Tickstart+tick_cur) > Timeout)
        {
          return -1;
        }
      }
    }

  } while((data & 0x01) == 0x01);

  return 0;
}

/**
  * @brief  Write status register 1 and 2
  * @param  StaReg1: The value write status register 1
  *         StaReg2: The value write status register 2
  * @note   register 1
  *         S7    S6   S5  S4   S3   S2   S1   S0
  *         SRP0  SEC  TB  BP2  BP1  BP0  WEL  WIP
  *         register 2
  *         S15   S14   S13  S12   S11   S10   S9   S8
  *         SUS   CMP   LB3  LB2   LB1   LB0   QE   SRP1 
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_WriteStaReg(unsigned char StaReg1, unsigned char StaReg2)
{
  signed long retval;
  unsigned char wBuffer[3];
  unsigned long Tickstart;

  retval = FlashMem_WriteEnable();
  if (retval)    return retval;

  wBuffer[0] = FM25Q_Volatile_SR_WriteEnable;
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, NULL, 1, 2);
  FlashMem_SPI_SetCSN(1);
  if (retval)    return retval;

  wBuffer[0] = FM25Q_WriteStatusReg;
  wBuffer[1] = StaReg1;
  wBuffer[2] = StaReg2;
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, NULL, 3, 2);
  FlashMem_SPI_SetCSN(1);
  if (retval)    return retval;  

  Tickstart = System_GetTick();
  return FlashMem_WaitForIdle(Tickstart, 2);
}

/**
  * @brief  Sector erase(4K-bytes)
  * @param  addr: The address of specified sector to the erased state of all 1s
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_SectorErase_4KB(unsigned long addr)
{
  signed long retval;
  unsigned char wBuffer[4];
  unsigned long Tickstart;

  retval = FlashMem_WriteEnable();
  if (retval)    return retval;

  wBuffer[0] = FM25Q_SectorErase_4KB;
  wBuffer[1] = (unsigned char)(addr>>16);
  wBuffer[2] = (unsigned char)(addr>>8); 
  wBuffer[3] = (unsigned char)(addr);
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, NULL, 4, 2);
  FlashMem_SPI_SetCSN(1);
  if (retval)    return retval;

  Tickstart = System_GetTick();
  return FlashMem_WaitForIdle(Tickstart, 300);
}

/**
  * @brief  Block erase(32K-bytes)
  * @param  addr: The address of specified block to the erased state of all 1s
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_BlockErase_32KB(unsigned long addr)
{
  signed long retval;
  unsigned char wBuffer[4];
  unsigned long Tickstart;

  retval = FlashMem_WriteEnable();
  if (retval)    return retval;

  wBuffer[0] = FM25Q_BlockErase_32KB;
  wBuffer[1] = (unsigned char)(addr>>16);
  wBuffer[2] = (unsigned char)(addr>>8); 
  wBuffer[3] = (unsigned char)(addr);
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, NULL, 4, 2);
  FlashMem_SPI_SetCSN(1);
  if (retval)    return retval;

  Tickstart = System_GetTick();
  return FlashMem_WaitForIdle(Tickstart, 1800);
}

/**
  * @brief  Block erase(64K-bytes)
  * @param  addr: The address of specified block to the erased state of all 1s
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_BlockErase_64KB(unsigned long addr)
{
  signed long retval;
  unsigned char wBuffer[4];
  unsigned long Tickstart;

  retval = FlashMem_WriteEnable();
  if (retval)    return retval;

  wBuffer[0] = FM25Q_BlockErase_64KB;
  wBuffer[1] = (unsigned char)(addr>>16);
  wBuffer[2] = (unsigned char)(addr>>8); 
  wBuffer[3] = (unsigned char)(addr);
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(wBuffer, NULL, 4, 2);
  FlashMem_SPI_SetCSN(1);
  if (retval)    return retval;

  Tickstart = System_GetTick();
  return FlashMem_WaitForIdle(Tickstart, 2000);
}

/**
  * @brief  Chip erase
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_ChipErase(void)
{
  signed long retval;
  unsigned char wBuffer;
  unsigned long Tickstart;

  retval = FlashMem_WriteEnable();
  if (retval)    return retval;

  wBuffer = FM25Q_ChipErase;
  FlashMem_SPI_SetCSN(0);
  retval = FlashMem_SPI_Transfer(&wBuffer, NULL, 1, 2);
  FlashMem_SPI_SetCSN(1);
  if (retval)    return retval;

  Tickstart = System_GetTick();
  return FlashMem_WaitForIdle(Tickstart, 128000);
}

/**
  * @brief  Read
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_Read(unsigned char *data, unsigned long addr, uint16_t num, unsigned long Timeout)
{
  signed long retval;
  unsigned char wBuffer[4];
  
  wBuffer[0] = FM25Q_ReadData;
  wBuffer[1] = (unsigned char)(addr>>16);
  wBuffer[2] = (unsigned char)(addr>>8); 
  wBuffer[3] = (unsigned char)(addr);

  FlashMem_SPI_SetCSN(0);

  retval = FlashMem_SPI_Transfer(wBuffer, NULL, 4, 2);
  if (retval)
  {    
    FlashMem_SPI_SetCSN(1); 
    return retval;
  }

  retval = FlashMem_SPI_Transfer(NULL, data, num, Timeout);
  FlashMem_SPI_SetCSN(1);

  return retval;
}
/**
  * @brief  Page write
  * @note   This function allows from one byte to 256 bytes (a page) of data to be
  *         programmed at previously erased (FFh) memory locations
  * @retval ARM_DRIVER_OK or Error code(<0)
  */
signed long FlashMem_PageWrite(unsigned char *data, unsigned long addr, uint16_t num, unsigned long Timeout)
{
		signed long retval;
		unsigned char wBuffer[4];
		unsigned long Tickstart;
		if(addr%PAGE_BLOCK == 0x00)
		{
				FlashMem_SectorErase_4KB(addr);
		}
  
	  retval = FlashMem_WriteEnable();
	  if (retval)    return retval;

		wBuffer[0] = FM25Q_PageProgram;
		wBuffer[1] = (unsigned char)(addr>>16);
		wBuffer[2] = (unsigned char)(addr>>8); 
		wBuffer[3] = (unsigned char)(addr);

		FlashMem_SPI_SetCSN(0);
		retval = FlashMem_SPI_Transfer(wBuffer, NULL, 4, 2);
		if (retval)
		{    
			FlashMem_SPI_SetCSN(1); 
			return retval;
		}
		retval = FlashMem_SPI_Transfer(data, NULL, num, 5);
		FlashMem_SPI_SetCSN(1);
			
  if (retval)    return retval;

  Tickstart = System_GetTick();
  return FlashMem_WaitForIdle(Tickstart, Timeout);
}
signed long FlashMem_Write(unsigned char *data, unsigned long addr, uint16_t num, unsigned long Timeout)
{
	signed long retval;
	unsigned char tmp;
  unsigned long cnt;
  while (num)
  {
			tmp = addr & (256 - 1);  /* ????????? */
			if (tmp != 0)
			{
					cnt = 256 - tmp;       /* ?????????????????? */
			}
			else
			{ 
					cnt = 256;             /* ???????????? EEP_PAGE_SIZE */
			}
			if (num < cnt)
			{
					cnt = num;
			}
		  retval = 	FlashMem_PageWrite(data,addr,cnt,Timeout);
			if (retval)    return retval;
			addr += cnt;
      data += cnt;
      num -= cnt;
	}
	return retval;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

/**
  * @brief  Error_Handler
  */
void Error_Handler(void)
{
  while (1);
}

unsigned char rData[1024];
unsigned char wData[1024];

/**
  * @brief  This is a flash memory device test program
  */
void FlashMemory_Test(void)
{
  signed long retval;
  unsigned long i,j;
  __IO unsigned char err;
  unsigned long Start_Addr;

  /* SPI clock is 6553600 */
  FlashMem_SPI_Init();

  for (i=0; i<1024; i++)
    wData[i] = (unsigned char)(0x33);

  /*- Sector Erase ------------------------------------------------------------------------------------------*/
  Start_Addr = 5*4096;
  retval = FlashMem_SectorErase_4KB(Start_Addr);
  if (retval)    Error_Handler();
#if 1
  err = 0;
  for (i=0; i<4; i++) 
  {
    retval = FlashMem_Read(rData, 1024*i+Start_Addr, 1024, 15);
    if (retval)    Error_Handler();

    for (j=0; j<1024; j++)
    {
      if (rData[i] != 0xFF)
        err = 1;
    }  
  }

  /*- Page write ------------------------------------------------------------------------------------------*/
  err = 0;
  retval = FlashMem_PageWrite(wData, Start_Addr, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData+256, Start_Addr+256, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData+256*2, Start_Addr+256*2, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData+256*3, Start_Addr+256*3, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 1024, 10);
  if (retval)    Error_Handler();

  for (i=0; i<1024; i++)
  {
    if (rData[i] != wData[i])
      err = 1;
  }

  /*- Sector Erase ------------------------------------------------------------------------------------------*/
  retval = FlashMem_SectorErase_4KB(Start_Addr);
  if (retval)    Error_Handler();

  err = 0;
  for (i=0; i<4; i++) 
  {
    retval = FlashMem_Read(rData, 1024*i+Start_Addr, 1024, 15);
    if (retval)    Error_Handler();

    for (j=0; j<1024; j++)
    {
      if (rData[i] != 0xFF)
        err = 1;
    }  
  }

  /*- Block Erase 32KB ------------------------------------------------------------------------------------------*/
  Start_Addr = 55*32*1024;
  err = 0;
  retval = FlashMem_BlockErase_32KB(Start_Addr);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData, Start_Addr, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData+256, Start_Addr+32*1024-256, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 256, 15);
  if (retval)    Error_Handler(); 

  retval = FlashMem_Read(rData+256, Start_Addr+32*1024-256, 256, 15);
  if (retval)    Error_Handler(); 

  for (i=0; i<512; i++)
  {
    if (rData[i] != wData[i])
      err = 1;
  }

  retval = FlashMem_BlockErase_32KB(Start_Addr);
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 256, 15);
  if (retval)    Error_Handler(); 

  retval = FlashMem_Read(rData+256, Start_Addr+32*1024-256, 256, 15);
  if (retval)    Error_Handler(); 

  for (i=0; i<512; i++)
  {
    if (rData[i] != 0xFF)
      err = 1;
  }

  /*- Block Erase 64KB ------------------------------------------------------------------------------------------*/
  Start_Addr = 13*64*1024;
  err = 0;
  retval = FlashMem_BlockErase_64KB(Start_Addr);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData, Start_Addr, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData+256, Start_Addr+64*1024-256, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 256, 10);
  if (retval)    Error_Handler(); 

  retval = FlashMem_Read(rData+256, Start_Addr+64*1024-256, 256, 15);
  if (retval)    Error_Handler(); 

  for (i=0; i<512; i++)
  {
    if (rData[i] != wData[i])
      err = 1;
  }

  retval = FlashMem_BlockErase_64KB(Start_Addr);
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 256, 15);
  if (retval)    Error_Handler(); 

  retval = FlashMem_Read(rData+256, Start_Addr+64*1024-256, 256, 15);
  if (retval)    Error_Handler(); 

  for (i=0; i<512; i++)
  {
    if (rData[i] != 0xFF)
      err = 1;
  }

  /*- Chip Erase ------------------------------------------------------------------------------------------*/ 
  err = 0;
  retval = FlashMem_ChipErase();
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData, Start_Addr, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_PageWrite(wData+256, Start_Addr+4*1024*1024-256, 256, 15);
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 256, 15);
  if (retval)    Error_Handler(); 

  retval = FlashMem_Read(rData+256, Start_Addr+4*1024*1024-256, 256, 15);
  if (retval)    Error_Handler(); 

  for (i=0; i<512; i++)
  {
    if (rData[i] != wData[i])
      err = 1;
  }

  retval = FlashMem_ChipErase();
  if (retval)    Error_Handler();

  retval = FlashMem_Read(rData, Start_Addr, 256, 15);
  if (retval)    Error_Handler(); 

  retval = FlashMem_Read(rData+256, Start_Addr+4*1024*1024-256, 256, 15);
  if (retval)    Error_Handler(); 

  for (i=0; i<512; i++)
  {
    if (rData[i] != 0xFF)
      err = 1;
  }

  //FlashMem_SPI_DeInit();
 #endif
}





#if 0

#define POWER_ON_MODE  0x00
#define POWER_OFF_MODE 0x01

/******************************************************************************
**定义Flash操作命令字  MX25L3206E
******************************************************************************/

/**
- 2.7 to 3.6 volt for read, erase, and program operations
- 1024 Equal Sectors with 4K byte each
- Page base (256 bytes)
- Fast program time: 0.6ms(typ.) and 3ms(max.)/page
- Byte program time: 9us (typical)
- Fast erase time: 40ms(typ.) /sector ; 0.4s(typ.) /block

- Low active read current: 25mA(max.) at 86MHz
- Low active programming current: 15mA (typ.)
- Low active sector erase current: 9mA (typ.)
- Standby current: 15uA (max.)
- Deep power-down mode 2uA (typical)

*/

struct spi_io_t
{
    unsigned char  command[8];
    unsigned char  *buffer;
    unsigned int   length;
    unsigned int   cmdnum;
};

#define  MX_CMD_WRSR     0x01        /** write status register */
#define  MX_CMD_WRDI     0x04        /** write disable         */
#define  MX_CMD_RDSR     0x05        /** read status register  */
#define  MX_CMD_WREN     0x06        /** write enable          */
#define  MX_CMD_READ     0x03        /** Read data byte        */
#define  MX_CMD_SE       0x20        /** Sector Erase          */
#define  MX_CMD_BE       0xD8        /** Block Erase           */
#define  MX_CMD_CE       0x60        /** Chip Erase            */
#define  MX_CMD_PP       0x02        /** page program          */
#define  MX_CMD_DP       0xB9        /** Deep power down       */
#define  MX_CMD_RDP      0xAB        /** Realese from Deep power down */
#define  MX_CMD_RDID     0x9F        /** Read Identification  */



#define CTRL_FLS_SECTOR_ERASE 0x01

#define CTRL_FLS_BLOCK_ERASE  0x02

/*
#ifdef  BIT_BAND_EN
#define  DO_CS_HIGH()     {} //{*(flash_hal->cs->lat_out) = 1;}
#define  DO_CS_LOW()       {}//{*(flash_hal->cs->lat_out) = 0;}
#else
#define  DO_CS_HIGH()     {} //{*(flash_hal->cs->lat_out) |= flash_hal->cs->pin_bit ;}
#define  DO_CS_LOW()      {}// {*(flash_hal->cs->lat_out) &= ~flash_hal->cs->pin_bit ;}
#endif
*/

unsigned char flash_read_and_compare(struct spi_io_t *read_data)
{
    unsigned int ui_i;                         /* 循环变量 */
    
    if (read_data->cmdnum > 8)
    {
        return false;
    }
    
    spi_start();
    
    DO_CS_LOW;
    
    for (ui_i = 0; ui_i < read_data->cmdnum; ui_i ++)
    {
        /* 发送指令和地址 */
        spi_send_byte(read_data->command[ui_i]);
    }
    
    for (ui_i = 0; ui_i < read_data->length; ui_i ++)
    {
        /* 逐个接收字节 */
        if (read_data->buffer[ui_i] != spi_recive_byte( 0 ))
        {
            DO_CS_HIGH;  /* 将CS线拉高 */
            return false;
        }
    }
    
    DO_CS_HIGH;  /* 将CS线拉高 */

     spi_stop();
    
    return true;
}

unsigned char flash_write_data(struct spi_io_t *write_data )
{
    unsigned int ui_i;                         /* 循环变量 */
    
    if (write_data->cmdnum > 8)
    {
        return false;
    }
    
    spi_start();    
    
    DO_CS_LOW;
    

    for (ui_i = 0; ui_i < write_data->cmdnum; ui_i ++)
    {
        /* 发送指令和地址 */
        spi_send_byte(write_data->command[ui_i]);
    }
    
    /* 循环逐个接收字节 */
    for (ui_i = 0; ui_i < write_data->length; ui_i ++)
    { 
        spi_send_byte(write_data->buffer[ui_i]);  /* 逐个接收字节 */
    }
    
    DO_CS_HIGH;  /* 将CS线拉高 */
    
    spi_stop();
    
    return true;
}




unsigned char flash_read_data(struct spi_io_t *read_data)
{
    unsigned int ui_i;                         /* 循环变量 */
    
    if (read_data->cmdnum > 8)
    {
        return false;
    }
    
    spi_start();  
    
    DO_CS_LOW;
    
    for (ui_i = 0; ui_i < read_data->cmdnum; ui_i ++)
    {
        /* 发送指令和地址 */
       spi_send_byte(read_data->command[ui_i]);
    }
    //plugin_machine.mcu_delay_us(50);
    /* 循环逐个接收字节 */
    for (ui_i = 0; ui_i < read_data->length; ui_i ++)
    {
        /* 逐个接收字节 */
        read_data->buffer[ui_i] = spi_recive_byte(0);
    }
    
    DO_CS_HIGH;  /* 将CS线拉高 */
    return flash_read_and_compare(read_data);     /**< 返回读出比较的结果 */
} 



unsigned char get_flash_status()
{
    struct spi_io_t spi_io;
    unsigned int flash_statue = 0;
    
    spi_io.command[0] = MX_CMD_RDSR;
    spi_io.buffer = (unsigned char*)&flash_statue;
    spi_io.length = 1;
    spi_io.cmdnum = 1;
    
    flash_read_data(&spi_io);
    if ((flash_statue & 0x01) == 0)
    {
        spi_io.buffer = (unsigned char*)&flash_statue;
        flash_read_data(&spi_io );
        
        if ((flash_statue & 0x01) == 0)
        {
            return true;
        }
    }
    
    return false;
}

void flash_write_enable(void)
{
    struct spi_io_t spi_io;
    
    spi_io.command[0] = MX_CMD_WREN;
    spi_io.buffer = NULL;
    spi_io.length = 0;
    spi_io.cmdnum = 1;
    
    flash_write_data(&spi_io);
    
}

unsigned char flash_erase(unsigned long int  address,unsigned char erase_mode)
{
    struct spi_io_t spi_io;
    unsigned char err;
    unsigned int cnt = 2000;
    
    
   // const struct spi_bus_ops_t CODE *spi_bus_ops = (const struct spi_bus_ops_t CODE *)device->bus_driver;
  //  const struct spi_bus_resource_t CODE *spi_bus_resource = (const struct spi_bus_resource_t CODE *)device->resource;
  //  const struct flash_hal_cfg_t CODE *flash_hal_cfg = (const struct flash_hal_cfg_t CODE *)device->hal_cfg;
 
  //  if (address > flash_hal_cfg->size)
    {
  //      return false;
    }    
    
    address &= ~0xFFFU;
    flash_write_enable();
   // plugin_machine.mcu_delay_us(10);
    if (erase_mode == CTRL_FLS_SECTOR_ERASE)
    {
        spi_io.command[0] = MX_CMD_SE;
    }
    else
    {
        spi_io.command[0] = MX_CMD_BE;
    }
    
    spi_io.command[1] = (unsigned char)(address >> 16);
    spi_io.command[2] = (unsigned char)(address >> 8);
    spi_io.command[3] = (unsigned char)address;
    spi_io.buffer = NULL;
    spi_io.cmdnum = 4;
    spi_io.length = 0;
    
    err = flash_write_data(&spi_io );
    
    while (false == get_flash_status())
    {
        cnt--;
        if(cnt == 0)
        {
            break;
        }
    }
    
    return err;
}





unsigned char flash_read(unsigned long addr, unsigned char *buf, unsigned int len)
{
    struct spi_io_t spi_io;
    
   // const struct spi_bus_ops_t CODE *spi_bus_ops = (const struct spi_bus_ops_t CODE *)device->bus_driver;
   // const struct spi_bus_resource_t CODE *spi_bus_resource = (const struct spi_bus_resource_t CODE *)device->resource;
   // const struct flash_hal_cfg_t CODE *flash_hal_cfg = (const struct flash_hal_cfg_t CODE *)device->hal_cfg;
    
    //if ((addr + len) > flash_hal_cfg->size)
    {
    //    return false;
    }
    
    spi_io.command[0] = MX_CMD_READ;
    spi_io.command[1] = (unsigned char)(addr >> 16);
    spi_io.command[2] = (unsigned char)(addr >> 8);
    spi_io.command[3] = (unsigned char)addr;
    spi_io.buffer = buf;
    spi_io.length = len;
    spi_io.cmdnum = 4;
    
  //  hfp_system_state.error_word &= ~ERR_FLASH;
    
    if(false == flash_read_data(&spi_io))
    {
     //   hfp_system_state.error_word |= ERR_FLASH;
        return false;
    }
    return true;
}

unsigned char flash_write(unsigned long addr, unsigned char *buf, unsigned int len)
{
    struct spi_io_t spi_io;
    unsigned char ret;
    unsigned char tmp;
    unsigned int cnt;
    unsigned int timeout = 2000;
   // const struct spi_bus_ops_t CODE *spi_bus_ops = (const struct spi_bus_ops_t CODE *)device->bus_driver;
   // const struct spi_bus_resource_t CODE *spi_bus_resource = (const struct spi_bus_resource_t CODE *)device->resource;
  //  const struct flash_hal_cfg_t CODE *flash_hal_cfg = (const struct flash_hal_cfg_t CODE *)device->hal_cfg;
    
//    if ((addr + len) > flash_hal_cfg->size)
    {
  //      return false;
    }
    
    ret = true;
    
   // hfp_system_state.error_word &= ~ERR_FLASH;
    
    while (len)
    {
        flash_write_enable();
        tmp = addr & (256 - 1);  /* 查找一页开始的地方 */
        
        if (tmp != 0)
        {
            cnt = 256 - tmp;       /* 非一页开始的地方最多一次能写的字节数 */
        }
        else
        { 
            cnt = 256;             /* 页开始的地方最多一次能写 EEP_PAGE_SIZE */
        }
        
        if (len < cnt)
        {
            cnt = len;
        }
        
        /* page program */
        spi_io.command[0] = MX_CMD_PP;
        /* 地址高字节 */
        spi_io.command[1] = (unsigned char)(addr >> 16);
        /* 地址中字节 */
        spi_io.command[2] = (unsigned char)(addr >> 8);
        spi_io.command[3] = (unsigned char)addr;          /* 地址低字节 */
        
        spi_io.buffer = buf;           /* 缓存数据 */
        spi_io.cmdnum = 4;             /* 命令长度 */
        spi_io.length = cnt;           /* 数据长度 */
       // plugin_machine.mcu_delay_us(10);
        ret = flash_write_data(&spi_io );       /* 执行命令 */
       // plugin_machine.mcu_delay_us(10);
        while (false == get_flash_status())   /* 等待它的空闲 */
        {
            timeout--;
            if(timeout == 0)
            {
                break;
            }
        }
        
        spi_io.command[0] = MX_CMD_READ;
        
        spi_io.cmdnum = 4;
        
        ret = flash_read_and_compare(&spi_io);
        
        if (ret == false)
        {
    //        hfp_system_state.error_word |= ERR_FLASH;
            break;
        }
        
        addr += cnt;
        buf += cnt;
        len -= cnt;
    }
    
    return ret;
}


void flash_power_ctrl(unsigned char power_mode)
{
   // const struct flash_hal_cfg_t CODE *flash_hal = (const struct flash_hal_cfg_t CODE *)device->hal_cfg;
   // const struct gpio_pin_t CODE *power_ctrl = flash_hal->power_ctrl;
    //const struct spi_bus_ops_t CODE *spi_ops = (const struct spi_bus_ops_t CODE*)device->bus_driver;
    
    
    if(power_mode == POWER_ON_MODE)
    {
      //  if(power_ctrl)
        {
        //    power_ctrl->active_out(power_ctrl->lat_out , power_ctrl->pin_bit);
         //   plugin_machine.mcu_delay_us(1000);
        }
      //  spi_ops->init((const struct spi_bus_resource_t CODE*)device->resource);
      //  DO_CS_HIGH();
       // flash_hal->cs->out(flash_hal->cs->direct , flash_hal->cs->pin_bit);
        
    }
    if(power_mode == POWER_OFF_MODE)
    {
       // power_ctrl->non_active_out(power_ctrl->lat_out , power_ctrl->pin_bit);
      //  spi_ops->stop((const struct spi_bus_resource_t CODE*)device->resource);
      //  flash_hal->cs->in(flash_hal->cs->direct , flash_hal->cs->pin_bit);
    }
}





unsigned char flash_ioctrl(unsigned char cmd,void *ctrl_parameter)
{
    unsigned long *addr = (unsigned long *)ctrl_parameter;

    
   // const struct flash_hal_cfg_t CODE *flash_hal_cfg = (const struct flash_hal_cfg_t CODE *)device->hal_cfg;
    
    
   // if (*addr >= flash_hal_cfg->size)
    {
    //    return false;
    }
   // hfp_system_state.error_word &= ~ERR_FLASH;
    switch(cmd)
    {
        case CTRL_FLS_SECTOR_ERASE:
        case CTRL_FLS_BLOCK_ERASE:
              if(false == flash_erase(*addr, cmd))
              {
                // hfp_system_state.error_word |= ERR_FLASH;
                  return false;
              }
              break ;
        default:
            return false;
    }
    return true;
}

unsigned char flash_init(unsigned char power_mode)
{
    flash_power_ctrl(power_mode);
    return true;
}
/*
const struct driver_t  CODE data_flash_driver = 
{
    flash_init,
    flash_read,
    flash_write,
    flash_ioctrl,
};
*/
#endif


