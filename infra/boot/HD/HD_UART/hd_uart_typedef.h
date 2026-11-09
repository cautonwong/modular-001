/*!
 * \file      hd_uart_typedef.h
 *
 * \brief     hd_uart implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Include ----------------------------------------------------------------------*/

/* Macro ------------------------------------------------------------------------*/
#define HD_UART_BUFF_LENGTH    (1200)
/* type prototype ---------------------------------------------------------------*/
typedef void (*pFuncVoid)(void);

typedef enum
{
    PHY_UART0    = 0,
    PHY_UART4    = 4,
    PHY_CHNL_MAX = 6,
}HD_UART_PhyChnl_e;

typedef enum
{
    //send status
    UART_READY_2_SEND = 0,
    UART_SENDING      = 1,
    UART_SEND_DONE    = 2,
    //recive status
    UART_RCVING       = 3,
    UART_RCV_DONE     = 4,
    //other status
    UART_IDLE         = 5,
}HD_UART_Status_e;

typedef struct
{
    uint16_t HPtr;     
    uint16_t Len;
    uint8_t  pBuff[HD_UART_BUFF_LENGTH];
    uint16_t Timeout;
    uint8_t  RXDly;
    HD_UART_Status_e  SndSts;
    HD_UART_Status_e  RcvSts;
    pFuncVoid Hook;
}HD_UARTx_Def_t;
/* Private Variables ------------------------------------------------------------*/
HD_UARTx_Def_t s_stUartx[PHY_CHNL_MAX];

const UART_TypeDef *pUartXMap[PHY_CHNL_MAX] = 
{
    UART0,
    UART4,
};
const uint32_t UartBaud[] = 
{
    9600,
};
/* Function ---------------------------------------------------------------------*/
/*!
 * \brief	UART init when power on
 * \param	none		
 * \retval	none
 */
void HD_UART_InitPwrOn(void);
/*!
 * \brief	UART init when power down
 * \param	none		
 * \retval	none
 */
void HD_UART_InitPwrDown(void);
/*!
 * \brief	config uart 
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
 * \param	[HD_UART_Format_e]Fmt - uart format
 * \param	[HD_UART_Baud_e]Baud - baud rate
 * \retval	none
 */
void HD_HART_Config(HD_UART_Chnl_e AbsUartx, HD_UART_Baud_e Baud);
/*!
 * \brief	send data via uart
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
 * \param	[uint8_t]pBuff - data buffer
 * \param	[uint16_t]DataLen - data length
 * \param	[void(*)(void)]Hook - hook
 * \retval	none
 */
void HD_HART_Send(HD_UART_Chnl_e AbsUartx, uint8_t* pBuff, uint16_t DataLen, void (*Hook)(void));
/*!
 * \brief   uart interrupt service routine
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel	
 * \retval	none
 */
void HD_HART_ISR(HD_UART_Chnl_e AbsUartx);
/*!
 * \brief	enable uartx txdone
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel	
 * \retval	none
 */
void HD_UART_EnableTx(HD_UART_Chnl_e AbsUartx, HD_System_Ctrl_e En);
/*!
 * \brief	enable uartx rx
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel	
 * \retval	none
 */
void HD_UART_EnableRx(HD_UART_Chnl_e AbsUartx, HD_System_Ctrl_e En);
/*!
 * \brief	time process
 * \param	none	
 * \retval	none
 */
void HD_UART_TimeProc(void);
/*!
 * \brief	time process
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
 * \param   [uint8_t*]pSrcBuff - data buffer
 * \param   [uint16_t*]pLen - Data length
 * \retval	[HD_UART_BuffStatus_e] - Buffer status
 */
HD_UART_BuffStatus_e HD_UART_GetRcvData(HD_UART_Chnl_e AbsUartx, uint8_t *pSrcBuff, uint16_t *pLen);
/*!
 * \brief	clear recive status
 * \param	[HD_UART_Chnl_e]AbsUartx - abstract uart channel
 * \retval	none
 */
void HD_UART_ClrRcvStatus(HD_UART_Chnl_e AbsUartx);

HD_UART_BuffStatus_e HD_UART_GetSendStatus(HD_UART_Chnl_e AbsUartx);

void HD_UART_WeakHook(void);
/* End --------------------------------------------------------------------------*/