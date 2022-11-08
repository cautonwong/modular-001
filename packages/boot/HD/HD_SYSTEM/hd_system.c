/*!
 * \file      hd_system.c
 *
 * \brief     hd_system implementation
 *
 * \endcode
 *
 * \author	  
 */
#ifndef _HD_SYSTEM_C_
#define _HD_SYSTEM_C_
/* Include ----------------------------------------------------------------------*/
#include "target.h"
#include "HD_Includes.h"
#include "hd_system_typedef.h"
#include "hd_gpio_list.h"

extern void HD_IIC_InitPwrOn(void);
/* Variables --------------------------------------------------------------------*/

/* Const ------------------------------------------------------------------------*/
const HD_SYSTEM_API_t g_stSystemApi = 
{
    HD_SYSTEM_InitRst,
    HD_SYSTEM_InitPwrOn,
    0,//HD_SYSTEM_InitPwrDown,
    0,//HD_SYSTEM_InitWakeUp,
    HD_Delay_us,
    HD_Delay_ms,
    HD_WatchDog_Clear,
    HD_PowerFail_Dtc,
    0,//HD_LowPowerCfg,
    0,//HD_WDT_Config,
    0,//HD_System_SecondPulse_Output,
    HD_System_TimeProc,
    0,//HD_System_RelayCtrl,
    0,//HD_SYSTEM_Enter_Into_Sleep,
    HD_SYSTEM_IrqCtrl,
    HD_SYSTEM_ClkDtc,
    HD_SYSTEM_McuReset,
    0,//HD_SYSTEM_SaveEnergyRegs,
    0,//HD_SYSTEM_GetEnergyRegs,
    0,//HD_SYSTEM_Systick_Config,
    HD_SYSTEM_Jump2App,
    (uint32_t*)&AppRun,
    HD_SYSTEM_FlashProgramByte,
    HD_SYSTEM_EraseSector,
};
/* End --------------------------------------------------------------------------*/
void HD_SYSTEM_InitRst(void)
{
    ;
}

void HD_SYSTEM_InitPwrOn(void)
{
    AppRun = 0;
    g_stGpioApi.pInitPwrOn();
    HD_IIC_InitPwrOn();
    spi_hw_api.inti_pwron();
}

void HD_SYSTEM_InitPwrDown(HD_System_PeripheralCtrl_t Ctrl)
{
    ;
}

void HD_SYSTEM_InitWakeUp(void)
{
    ;
}


void HD_Delay_us(uint16_t Us)
{
    for(; Us > 0; Us--)
    {
        __NOP();__NOP();__NOP();__NOP();__NOP();
        __NOP();__NOP();__NOP();__NOP();__NOP();
        __NOP();__NOP();__NOP();__NOP();
    }
}

void HD_Delay_ms(uint16_t Ms)
{
    for(; Ms > 0; Ms--)
    {
        HD_Delay_us(1000);
    }
}

void HD_WatchDog_Clear(void)
{
    WDT_Clear();
}

uint8_t HD_PowerFail_Dtc(HD_System_PwrFail_Dtc_e Check, uint8_t Ms)
{

    return (0);
}

void HD_LowPowerCfg(void)
{
    ;
}


void HD_SYSTEM_Enter_Into_Sleep(void)
{
	;
}

void HD_WDT_Config(uint8_t En)
{
    if(1 == En)
    {
        WDT_SetPeriod(WDT_2_SECS);
        WDT_Enable();
        WDT_Clear();
    }
    else
    {
        WDT_Disable();
    }
}

void HD_System_SecondPulse_Output(uint8_t Enable)
{
    ;
}

void HD_System_TimeProc(void)
{
    ;
}

void HD_System_RelayCtrl(HD_System_RelayCtrl_e Ctrl)
{
    ;
}

void HD_SYSTEM_IrqCtrl(HD_System_Ctrl_e Ctrl)
{
    switch(Ctrl)
    {
        case HD_ENABLE:
        {
            __enable_irq();
        }break;
        case HD_DISABLE:
        {
            __disable_irq();
        }break;
        default:
        {
        }break;
    }
}

uint8_t HD_SYSTEM_ClkDtc(HD_System_Clk_e Clk)
{
    uint8_t retsts;
    
    switch(Clk)
    {
        case HD_LSE:
        {
            retsts = PMU_GetStatus(PMU_STS_32K);
        }
        case HD_HSE:
        {
            retsts = PMU_GetStatus(PMU_STS_6M);
        }
        default:
        {
            retsts = 1;
        }break;
    }
    
    return (!retsts);
}

void HD_SYSTEM_McuReset(void)
{
    NVIC_SystemReset();
    while(1);
}

void HD_SYSTEM_SaveEnergyRegs(uint32_t RegValue, HD_System_RegName_e RegName)
{
    if(HD_CLRALL == RegName)
    {
        for(uint8_t Cnt = 0; Cnt < HD_CLRALL; Cnt++)
        {
            *(PMU_RAM0 + Cnt)= 0;
        }
    }
    else
    {
        *(PMU_RAM0 + RegName) = RegValue;
    }
}

uint32_t HD_SYSTEM_GetEnergyRegs(HD_System_RegName_e RegName)
{
    uint32_t RegValue;
    
    RegValue = *(PMU_RAM0 + RegName);
    
    return (RegValue);
}

void HD_SYSTEM_Systick_Config(void)
{
    CORTEX_SystemTick_Config(26214400 / 1000 - 1);
}

void HD_SYSTEM_deinit_before_jump(void)
{
    uint32_t temp;
    
    __disable_irq();
    // disable all interrupt 
    NVIC->ICER[0] = 0xFFFFFFFF;
    // clear pendings
    NVIC->ICPR[0] = 0xFFFFFFFF;
    // stop sys tick
    temp = SysTick->CTRL;
    temp &= ~0x02;
    SysTick->CTRL = temp; 
    WDT_Disable();
    AppRun = 1;
}

void HD_SYSTEM_Jump2App(uint32_t AppAddr)
{
	HD_SYSTEM_deinit_before_jump();

    JumpAddress = *(__IO uint32_t*) (AppAddr + 4);
    Jump_To_Application = (pFunc) JumpAddress;

    __set_MSP(*(__IO uint32_t*) AppAddr); 

    Jump_To_Application();
}

void HD_SYSTEM_FlashProgramByte(uint32_t Addr, uint8_t *Buff, uint32_t Length)
{
    FLASH_ProgramByte(Addr, Buff, Length);
}

void HD_SYSTEM_EraseSector(uint32_t Addr)
{
    FLASH_SectorErase(Addr);
}

#endif
