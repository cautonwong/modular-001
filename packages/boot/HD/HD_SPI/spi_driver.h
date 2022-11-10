/**
  ******************************************************************************
  * @file    lib_spi.h 
  * @author  Application Team
  * @version V4.4.0
  * @date    2018-09-27
  * @brief   SPI library.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
#ifndef __SPI_DRIVER_H
#define __SPI_DRIVER_H


// SPIx be used
#define SPI_Flash SPI2

#define CSN_GPIO GPIOC
#define CSN_Pin  GPIO_Pin_0

//FM25Q command
#define FM25Q_WriteEnable                 0x06
#define FM25Q_WriteDisable                0x04
#define FM25Q_ReadStatusReg_1             0x05
#define FM25Q_ReadStatusReg_2             0x35
#define FM25Q_Volatile_SR_WriteEnable     0x50
#define FM25Q_WriteStatusReg              0x01
#define FM25Q_SectorErase_4KB             0x20
#define FM25Q_BlockErase_32KB             0x52
#define FM25Q_BlockErase_64KB             0xD8
#define FM25Q_ChipErase                   0xC7
#define FM25Q_ReadData                    0x03
#define FM25Q_PageProgram                 0x02 

void FlashMemory_Test(void);


unsigned long System_GetTick(void);
void FlashMem_SPI_Init(void);
void FlashMem_SPI_DeInit(void);
void FlashMem_SPI_SetCSN(unsigned char level);
signed long FlashMem_SPI_Transfer(unsigned char *w_data, unsigned char *r_data, unsigned long len, unsigned long Timeout);

signed long FlashMem_WriteEnable(void);
signed long FlashMem_WriteDisable(void);

signed long FlashMem_WaitForIdle(unsigned long Tickstart, unsigned long Timeout);

signed long FlashMem_ReadStaReg_1(unsigned char *data);
signed long FlashMem_ReadStaReg_2(unsigned char *data);
signed long FlashMem_WriteStaReg(unsigned char StaReg1, unsigned char StaReg2);

signed long FlashMem_SectorErase_4KB(unsigned long addr);
signed long FlashMem_BlockErase_32KB(unsigned long addr);
signed long FlashMem_BlockErase_64KB(unsigned long addr);
signed long FlashMem_ChipErase(void);

signed long FlashMem_Read(unsigned char *data, unsigned long addr, unsigned short num, unsigned long Timeout);
signed long FlashMem_Write(unsigned char *data, unsigned long addr, unsigned short num, unsigned long Timeout);
signed long FlashMem_PageWrite(unsigned char *data, unsigned long addr, unsigned short num, unsigned long Timeout);


extern unsigned char spi_bus_init(void);
extern void spi_start(void);
extern void spi_stop(void);
extern unsigned char spi_send_byte(unsigned char byte);
extern unsigned char spi_recive_byte(unsigned char ctrl);
extern unsigned char spi_send_byte_mode3(unsigned char byte);
extern unsigned char spi_recive_byte_mode3(unsigned char ctrl);
extern unsigned char flash_read(unsigned long addr, unsigned char *buf, unsigned int len);
extern unsigned char flash_write(unsigned long addr, unsigned char *buf, unsigned int len);
extern unsigned char flash_erase(unsigned long int  address,unsigned char erase_mode);
#endif  /* __LIB_SPI_H */

/*********************************** END OF FILE ******************************/
