/*!
 * \file      hd_iic.h
 *
 * \brief     hd_iic implementation
 *
 * \endcode
 *
 * \author	   
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_IIC_H_
#define _HD_IIC_H_
/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/
typedef enum
{
    STORAGE_READ  = 0,
    STORAGE_WRITE = 1,
}HD_IIC_Operation_e;

typedef struct
{
	/*!
	 * \brief	IIC init when power on
	 * \param	none		
	 * \retval	none
	 */
    void    (*pInitPwrOn)(void);
	/*!
	 * \brief	IIC init when power down
	 * \param	none		
	 * \retval	none
	 */
    void    (*pInitPwrDown)(void);
	/*!
	 * \brief	IIC operation API
	 * \param	[uint32_t]Addr - address of storage chip
	 * \param	[uint8_t*]pDataBuf - ram for data
	 * \param	[uint32_t]Len - length of data
	 * \param	[HD_IIC_Operation_e]Operate - read/write
	 * \retval	none
	 */
    uint8_t (*Storage_Access)(uint32_t Addr, uint8_t *pDataBuf, uint32_t Len, HD_IIC_Operation_e Operate);
}HD_IIC_API_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_IIC_API_t g_stIICApi;
/* End --------------------------------------------------------------------------*/
#endif
