/*!
 * \file      hd_clk.c
 *
 * \brief     hd_clk implementation
 *
 * \endcode
 *
 * \author	  
 */
#ifndef _HD_CLK_C_
#define _HD_CLK_C_
/* Include ----------------------------------------------------------------------*/
#include "target.h"
#include "HD_Includes.h"
#include "hd_clk_typedef.h"
/* Variables --------------------------------------------------------------------*/

/* Const ------------------------------------------------------------------------*/
const HD_CLK_API_t g_stClkApi = 
{
    HD_CLK_InitPwrOn,
    HD_CLK_InitPwrDown,
};
/* End --------------------------------------------------------------------------*/
void HD_CLK_InitPwrOn(void)
{
    CLK_InitTypeDef CLK_Struct;

    CLK_Struct.ClockType = CLK_TYPE_AHBSRC \
                          |CLK_TYPE_PLLL   \
                          |CLK_TYPE_HCLK   \
                          |CLK_TYPE_PCLK;
    CLK_Struct.AHBSource      = CLK_AHBSEL_LSPLL;
    CLK_Struct.PLLL.Frequency = ANA_REG9_PLLLSEL_26M;
    CLK_Struct.PLLL.Source    = CLK_PLLLSRC_XTALL;
    CLK_Struct.PLLL.State     = CLK_PLLL_ON;
    CLK_Struct.HCLK.Divider   = 1;
    CLK_Struct.PCLK.Divider   = 2;
    CLK_ClockConfig(&CLK_Struct); 
    
    CLK_APBPeriphralCmd(CLK_APBPERIPHRAL_UART0 |\
                        CLK_APBPERIPHRAL_UART4 |\
                        
                        CLK_APBPERIPHRAL_TIMER, ENABLE);
    
    CLK_AHBPeriphralCmd(CLK_AHBPERIPHRAL_GPIO, ENABLE);
}

void HD_CLK_InitPwrDown(void)
{
;
}

#endif
