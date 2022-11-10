/*!
 * \file      hd_uart.h
 *
 * \brief     hd_uart implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_UART_H_
#define _HD_UART_H_
/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/
typedef enum
{
    UART_COM       = 0,
    UART_RS485     = 1,
    UART_BLUETOOTH = 2,
    
    UART_CHNL_MAX  = 3,
}HD_UART_Chnl_e;

typedef enum
{
    UART_8E1 = 1,
}HD_UART_Format_e;

typedef enum
{
    UART_9600BPS   = 4,
}HD_UART_Baud_e;

typedef enum
{
    UART_DLY_20MS  = 200,
	UART_DLY_500MS = 5000,
	UART_DLY_700MS = 7000,	
}HD_UART_Dly_e;

typedef enum
{
    UART_BUFFEMPTY  = 0,
    UART_BUSY       = 1,
    UART_BUFFFULL   = 2,
}HD_UART_BuffStatus_e;

typedef struct
{
	/*!
	 * \brief	UART init when power on
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrOn)(void);
	/*!
	 * \brief	UART init when power down
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrDown)(void);
	/*!
	 * \brief	config uart 
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
	 * \param	[HD_UART_Format_e]Fmt - uart format
	 * \param	[HD_UART_Baud_e]Baud - baud rate
	 * \retval	none
	 */
    void (*Config)(HD_UART_Chnl_e AbsUartx, HD_UART_Baud_e Baud);
	/*!
	 * \brief	send data via uart
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
	 * \param	[uint8_t]pBuff - data buffer
	 * \param	[uint16_t]DataLen - data length
	 * \param	[void(*)(void)]Hook - hook 
	 * \retval	none
	 */
    void (*Send)(HD_UART_Chnl_e AbsUartx, uint8_t* pBuff, uint16_t DataLen, void (*Hook)(void));
	/*!
	 * \brief	uart interrupt service routine
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel	
	 * \retval	none
	 */
    void (*ISR)(HD_UART_Chnl_e AbsUartx);
	/*!
	 * \brief	enable uartx txdone
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel	
	 * \retval	none
	 */
    void (*EnableTx)(HD_UART_Chnl_e AbsUartx, HD_System_Ctrl_e En);
	/*!
	 * \brief	enable uartx rx
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel	
	 * \retval	none
	 */
    void (*EnableRx)(HD_UART_Chnl_e AbsUartx, HD_System_Ctrl_e En);
	/*!
	 * \brief	time process
	 * \param	none	
	 * \retval	none
	 */
	void (*TimeProc)(void);
	/*!
	 * \brief	get recived data
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
	 * \param   [uint8_t*]pSrcBuff - data buffer 
	 * \param   [uint16_t*]pLen - Data length
	 * \retval	[HD_UART_BuffStatus_e] - Buffer status
	 */
	HD_UART_BuffStatus_e (*GetRcvData)(HD_UART_Chnl_e AbsUartx, uint8_t *pSrcBuff, uint16_t *pLen);
	/*!
	 * \brief	clear recive status
	 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
	 * \retval	none
	 */
	void (*ClrRcvStatus)(HD_UART_Chnl_e AbsUartx);
    
    HD_UART_BuffStatus_e (*GetSendStatus)(HD_UART_Chnl_e AbsUartx);
}HD_UART_API_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_UART_API_t g_stUartApi;
/* End --------------------------------------------------------------------------*/
#endif
