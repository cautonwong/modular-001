/*!
 * \file      hd_iic_typedef.h
 *
 * \brief     hd_iic implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/

/* Macro ------------------------------------------------------------------------*/
#define  SCL_DIR_OUTPUT()      Set_Output_SCL
#define  SCL_SET()             {IO_SCL_H;}
#define  SCL_CLR()             {IO_SCL_L;}
                               
#define  SDA_DIR_OUTPUT()      Set_Output_SDA
#define  SDA_DIR_INPUT()       Set_Input_SDA
#define  SDA_SET()             {IO_SDA_H;}
#define  SDA_CLR()             {IO_SDA_L;}
#define  SDA_GET()             IO_SDA_Read

#define ADDR_EEPROM            (0x00)
#define EEPROM_PAGE_SIZE       (0x80)
#define EEPROM_DVK_LEN         (EEPROM_PAGE_SIZE * (512) - 1)
#define EEPROM_PAGE_2_ADDR     (0x00010000)
#define EEPROM_ADDR_MAX_LEN    (2)
#define EEPROM_INVALID_ADDR    (0xF5F5F5F5)
/* type prototype ---------------------------------------------------------------*/
typedef struct 
{
    uint32_t ulOffset;       /*write  memmory device offset address */
    uint8_t  ucDevAddr;      /*iic device address 8 bits*/
    uint8_t *pData;          /*data buffer*/
    uint16_t usLen;          /*data length*/
    uint8_t  ucOffSetLen;    /*data address length*/ 
}ST_IIC_WRITR_READ_CONF;
/* Private Variables ------------------------------------------------------------*/
/* Function ---------------------------------------------------------------------*/
/*!
 * \brief	IIC init when power on
 * \param	none		
 * \retval	none
 */
void HD_IIC_InitPwrOn(void);
/*!
 * \brief	IIC init when power down
 * \param	none		
 * \retval	none
 */
void HD_IIC_InitPwrDown(void);
/*!
 * \brief	IIC start signal
 * \param	none		
 * \retval	none
 */
void IIC_Start(void);
/*!
 * \brief	IIC stop signal
 * \param	none		
 * \retval	none
 */
void IIC_Stop(void);
/*!
 * \brief	IIC ack signal
 * \param	none		
 * \retval	none
 */
void IIC_Ack(uint8_t ucAck);
/*!
 * \brief	IIC operation API
 * \param	[uint32_t]Addr - address of storage chip
 * \param	[uint8_t*]pDataBuf - ram for data
 * \param	[uint32_t]Len - length of data
 * \param	[HD_IIC_Operation_e]Operate - read/write
 * \retval	none
 */
uint8_t HD_Storage_Access(uint32_t Addr, uint8_t *pDataBuf, uint32_t Len, HD_IIC_Operation_e Operate);
/* End --------------------------------------------------------------------------*/