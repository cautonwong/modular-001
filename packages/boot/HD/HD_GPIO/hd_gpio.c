/*!
 * \file      hd_gpio.c
 *
 * \brief     hd_gpio implementation
 *
 * \endcode
 *
 * \author	  
 */
#ifndef _HD_GPIO_C_
#define _HD_GPIO_C_
/* Include ----------------------------------------------------------------------*/
#include "target.h"
#include "HD_Includes.h"
#include "hd_gpio_list.h"
#include "hd_gpio_typedef.h"

/* Variables --------------------------------------------------------------------*/

/* Const ------------------------------------------------------------------------*/
const HD_GPIO_API_t g_stGpioApi = 
{
    HD_GPIO_InitRst,
    HD_GPIO_InitPwrOn,
    HD_GPIO_InitPwrDown,
    HD_GPIO_IoCtrl,
    HD_GPIO_GetInputValue,
    HD_GPIO_OutputValue,
    HD_GPIO_Flash,
    HD_GPIO_TimeProcIsr,
};
/* End --------------------------------------------------------------------------*/
void HD_GPIO_InitRst(void)
{
}

void HD_GPIO_InitPwrOn(void)
{
    
    GPIOE->OEN &= ~BIT12;    //使能输出
    GPIOE->IE &= ~BIT12;     //禁止输入
    //GPIOE->DAT |= BIT12;    //拉闸灯亮
    
    GPIOB->OEN &= ~BIT8;    //使能输出
    GPIOB->IE &= ~BIT8;     //禁止输入
    GPIOB->DAT &= ~BIT8;    //
    
    GPIOB->OEN &= ~BIT4;    //使能输出
    GPIOB->IE &= ~BIT4;     //禁止输入
    GPIOB->DAT &= ~BIT4;    //
}

void HD_GPIO_InitPwrDown(void)
{
    ;
}

void HD_GPIO_IoCtrl(HD_GPIO_IO_Ctrl_t *Ctrl)
{
    ;
}

uint8_t HD_GPIO_GetInputValue(HD_GPIO_IO_Ctrl_t *Ctrl)
{
    return 0;
}

void HD_GPIO_OutputValue(HD_GPIO_IO_Ctrl_t *Ctrl)
{
    ;
}

void HD_GPIO_Flash(HD_GPIO_IO_Ctrl_t *Ctrl, HD_GPIO_Flash_Freq_e Freq)
{
    ;
}

void HD_GPIO_TimeProcIsr(void)
{
    ;
}
#endif
