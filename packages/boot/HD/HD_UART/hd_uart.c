/*!
 * \file      hd_uart.c
 *
 * \brief     hd_uart implementation
 *
 * \endcode
 *
 * \author	  hxiii
 */
#ifndef _HD_UART_C_
#define _HD_UART_C_
/* Include ----------------------------------------------------------------------*/
#include "target.h"
#include "HD_Includes.h"
#include "hd_uart_typedef.h"
/* Variables --------------------------------------------------------------------*/

/* Const ------------------------------------------------------------------------*/
const HD_UART_API_t g_stUartApi = 
{
    HD_UART_InitPwrOn,
    HD_UART_InitPwrDown,
    HD_HART_Config,
    HD_HART_Send,
    HD_HART_ISR,
    HD_UART_EnableTx,
    HD_UART_EnableRx,
    HD_UART_TimeProc,
    HD_UART_GetRcvData,
    HD_UART_ClrRcvStatus,
    HD_UART_GetSendStatus,
};
/* End --------------------------------------------------------------------------*/
void HD_UART_InitPwrOn(void)
{
    HD_HART_Config(UART_COM, UART_9600BPS);
    for(HD_UART_PhyChnl_e Cnt = PHY_UART0; Cnt < PHY_CHNL_MAX; Cnt++)
    {
        s_stUartx[Cnt].SndSts = UART_SEND_DONE;
        s_stUartx[Cnt].RcvSts = UART_IDLE;
    }
}

void HD_UART_InitPwrDown(void)
{
    UART_TypeDef *pUartX;
    
    for(uint8_t Cnt = PHY_UART0; Cnt < PHY_CHNL_MAX; Cnt++)
    {
        pUartX = (UART_TypeDef *)pUartXMap[Cnt];
        UART_DeInit(pUartX);
    }
}

HD_UART_PhyChnl_e HD_UART_Chnl_Remap(HD_UART_Chnl_e AbsUartx)
{
    HD_UART_PhyChnl_e PhyUartx;
    
    switch(AbsUartx)
    {
        case UART_COM:
        {
            PhyUartx = PHY_UART4;
        }break;
        case UART_RS485:
        {
            PhyUartx = PHY_UART0;
        }break;
    }
    
    return (PhyUartx);
}

void HD_HART_Config(HD_UART_Chnl_e AbsUartx, HD_UART_Baud_e Baud)
{
    HD_UART_PhyChnl_e PhyUartx;
    UART_TypeDef *pUartX;
    UART_InitType InitStruct;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pUartX = (UART_TypeDef *)pUartXMap[PhyUartx];
    /* config uartX-------------------------------------------*/
    InitStruct.Baudrate = UartBaud[Baud];
    InitStruct.FirstBit = UART_FIRSTBIT_LSB;
    InitStruct.Mode = UART_MODE_TX | UART_MODE_RX;

    InitStruct.Parity = UART_PARITY_EVEN;
    InitStruct.WordLen = UART_WORDLEN_9B;
    
    UART_Init(pUartX, &InitStruct);
    UART_ClearINTStatus(pUartX, UART_INTSTS_TXDONE | UART_INTSTS_RX);
    UART_INTConfig(pUartX, UART_INT_TXDONE | UART_INT_RX, ENABLE);
    switch(PhyUartx)
    {
        case PHY_UART0:
        {
            CORTEX_SetPriority_ClearPending_EnableIRQ(UART0_IRQn, 3);
        }break;

        case PHY_UART4:
        {
            CORTEX_SetPriority_ClearPending_EnableIRQ(UART4_IRQn, 3);
        }break;

    }
    
    UART_Cmd(pUartX, UART_MODE_TX | UART_MODE_RX, ENABLE);
}

void HD_HART_Send(HD_UART_Chnl_e AbsUartx, uint8_t* pBuff, uint16_t DataLen, void (*Hook)(void))
{
    HD_UART_PhyChnl_e PhyUartx;
    UART_TypeDef *pUartX;
    HD_UARTx_Def_t *pstUartCtrl;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pUartX = (UART_TypeDef *)pUartXMap[PhyUartx];
    pstUartCtrl = &s_stUartx[PhyUartx];
    /* init uartX_Ctrl before send----------------------------*/
    pstUartCtrl->HPtr = 0;
    pstUartCtrl->Len = DataLen;
    memcpy(pstUartCtrl->pBuff, pBuff, pstUartCtrl->Len);
    pstUartCtrl->SndSts = UART_SENDING;
	pstUartCtrl->RcvSts = UART_IDLE;
    if(PNULL == Hook)
    {
        pstUartCtrl->Hook = HD_UART_WeakHook;
    }
    else
    {
        pstUartCtrl->Hook = Hook;
    }
    /* put data into shift register---------------------------*/
	UART_Cmd(pUartX, UART_MODE_TX, ENABLE);
	UART_Cmd(pUartX, UART_MODE_RX, DISABLE);
    pUartX->DATA = pstUartCtrl->pBuff[0];
}

void HD_HART_ISR(HD_UART_Chnl_e AbsUartx)
{
    HD_UART_PhyChnl_e PhyUartx;
    UART_TypeDef *pUartX;
    HD_UARTx_Def_t *pstUartCtrl;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pUartX = (UART_TypeDef *)pUartXMap[PhyUartx];
    pstUartCtrl = &s_stUartx[PhyUartx];
    /* data process-------------------------------------------*/
    if(UART_GetINTStatus(pUartX, UART_INTSTS_TXDONE))
    {
        UART_ClearINTStatus(pUartX, UART_INTSTS_TXDONE);
        if(++pstUartCtrl->HPtr < pstUartCtrl->Len)
        {
            pUartX->DATA = pstUartCtrl->pBuff[pstUartCtrl->HPtr];
        }
        else
        {
            pstUartCtrl->HPtr = 0;
            pstUartCtrl->SndSts = UART_SEND_DONE;
            if(pstUartCtrl->Hook != PNULL)
            {
                pstUartCtrl->Hook();
            }
			UART_Cmd(pUartX, UART_MODE_RX, ENABLE);
			UART_Cmd(pUartX, UART_MODE_TX, DISABLE);
        }
    }
    if(UART_GetINTStatus(pUartX, UART_INTSTS_RX))
    {
        UART_ClearINTStatus(pUartX, UART_INTSTS_RX);
        if(pstUartCtrl->HPtr >= HD_UART_BUFF_LENGTH)
        {
            pstUartCtrl->HPtr = 0;
        }
		if((pstUartCtrl->Timeout > 0) || (0 == pstUartCtrl->HPtr)/* && (pstUartCtrl->RcvSts != UART_RCV_DONE)*/)
		{
            pstUartCtrl->pBuff[pstUartCtrl->HPtr++] = pUartX->DATA;
			pstUartCtrl->RcvSts = UART_RCVING;
	    }
        switch(AbsUartx)
        {
            case UART_RS485:
            {
                pstUartCtrl->Timeout = UART_DLY_500MS;
            }break;

            default:
            {
                pstUartCtrl->Timeout = UART_DLY_500MS;
            }break;
        }
    }
//    UART_ClearINTStatus(pUartX, UART_INTSTS_Msk);
}

void HD_UART_EnableTx(HD_UART_Chnl_e AbsUartx, HD_System_Ctrl_e En)
{
    HD_UART_PhyChnl_e PhyUartx;
    UART_TypeDef *pUartX;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pUartX = (UART_TypeDef *)pUartXMap[PhyUartx];
    /* config-------------------------------------------------*/
    UART_Cmd(pUartX, UART_MODE_TX, En);
}

void HD_UART_EnableRx(HD_UART_Chnl_e AbsUartx, HD_System_Ctrl_e En)
{
    HD_UART_PhyChnl_e PhyUartx;
    UART_TypeDef *pUartX;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pUartX = (UART_TypeDef *)pUartXMap[PhyUartx];
    /* config-------------------------------------------------*/
    UART_Cmd(pUartX, UART_MODE_RX, En);
}

void HD_UART_TimeProc(void)
{
    for(uint8_t Cnt = 0; Cnt < PHY_CHNL_MAX; Cnt++)
    {
	    if(s_stUartx[Cnt].Timeout > 0)
	    {
	        s_stUartx[Cnt].Timeout -= 1;
			if(0 == s_stUartx[Cnt].Timeout)
			{
	            s_stUartx[Cnt].RcvSts = UART_RCV_DONE;
	            s_stUartx[Cnt].Len = s_stUartx[Cnt].HPtr;
	            s_stUartx[Cnt].HPtr = 0;
			}
	    }
    }
}

HD_UART_BuffStatus_e HD_UART_GetSendStatus(HD_UART_Chnl_e AbsUartx)
{
    HD_UART_PhyChnl_e PhyUartx;
    HD_UARTx_Def_t *pstUartCtrl;
    HD_UART_BuffStatus_e Status;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pstUartCtrl = &s_stUartx[PhyUartx];
    
    if(pstUartCtrl->SndSts != UART_SEND_DONE)
    {
        Status = UART_BUSY;
    }
    else
    {
        Status = UART_BUFFEMPTY;
    }
    
    return (Status);
}

HD_UART_BuffStatus_e HD_UART_GetRcvData(HD_UART_Chnl_e AbsUartx, uint8_t *pSrcBuff, uint16_t *pLen)
{
    HD_UART_PhyChnl_e PhyUartx;
    HD_UARTx_Def_t *pstUartCtrl;
	HD_UART_BuffStatus_e BuffSts;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pstUartCtrl = &s_stUartx[PhyUartx];
	BuffSts = UART_BUFFEMPTY;
    if(UART_RCV_DONE == pstUartCtrl->RcvSts)
    {
        BuffSts = UART_BUFFFULL;
		memcpy(pSrcBuff, s_stUartx[PhyUartx].pBuff, s_stUartx[PhyUartx].Len);
		*pLen  = s_stUartx[PhyUartx].Len;
        pstUartCtrl->RcvSts = UART_IDLE;
    }
	
	return (BuffSts);
}

void HD_UART_ClrRcvStatus(HD_UART_Chnl_e AbsUartx)
{
    HD_UART_PhyChnl_e PhyUartx;
    HD_UARTx_Def_t *pstUartCtrl;
    /* get physical uartX-------------------------------------*/
    PhyUartx = HD_UART_Chnl_Remap(AbsUartx);
    pstUartCtrl = &s_stUartx[PhyUartx];

	pstUartCtrl->RcvSts = UART_IDLE;
}

void HD_UART_WeakHook(void)
{
    
}

#endif
