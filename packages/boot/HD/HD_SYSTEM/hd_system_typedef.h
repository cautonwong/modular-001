/*!
 * \file      hd_system_typedef.h
 *
 * \brief     hd_system implementation
 *
 * \endcode
 *
 * \author	   
 */
/* Include ----------------------------------------------------------------------*/

/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/

/* Private Variables ------------------------------------------------------------*/
uint32_t RelayTick;
__root __no_init volatile uint32_t AppRun @ 0x20000000;
uint32_t JumpAddress;
pFunc Jump_To_Application;
/* Function ---------------------------------------------------------------------*/
/*!
 * \brief	SYSTEM init when reset
 * \param	none		
 * \retval	none
 */
void HD_SYSTEM_InitRst(void);
/*!
 * \brief	SYSTEM init when power on
 * \param	none		
 * \retval	none
 */
void HD_SYSTEM_InitPwrOn(void);
/*!
 * \brief	SYSTEM init when power down
 * \param	[HD_System_PeripheralCtrl_t]Ctrl - enable/disable peripheral
 * \retval	none
 */
void HD_SYSTEM_InitPwrDown(HD_System_PeripheralCtrl_t Ctrl);
/*!
 * \brief	delay N us
 * \param	[uint16_t]Us - N us		
 * \retval	none
 */
void HD_Delay_us(uint16_t Us);
/*!
 * \brief	delay N ms
 * \param	[uint16_t]Ms - N ms		
 * \retval	none
 */
void HD_Delay_ms(uint16_t Ms);
/*!
 * \brief	Clear watch dog
 * \param	none		
 * \retval	none
 */
void HD_WatchDog_Clear(void);
/*!
 * \brief	power fail detect
 * \param	[HD_System_PwrFail_Dtc_e] Check - check type	
 * \param	[uint8_t] Ms - ms	
 * \retval	[uint8_t] - 1: powen on, 0: powen off
 */
uint8_t HD_PowerFail_Dtc(HD_System_PwrFail_Dtc_e Check, uint8_t Ms);
/*!
 * \brief	low power mode configration
 * \param	none		
 * \retval	none
 */
void HD_LowPowerCfg(void);
/*!
 * \brief	WDT config
 * \param	[uint8_t]En - 0:disable 1:enable	
 * \retval	none
 */
void HD_WDT_Config(uint8_t En);
/*!
 * \brief	enable/disable second pulse output
 * \param	[uint8_t]Enable - 0:disable 1:enable	
 * \retval	none
 */
void HD_System_SecondPulse_Output(uint8_t Enable);
/*!
 * \brief	delay count process
 * \param	none	
 * \retval	none
 */
void HD_System_TimeProc(void);
/*!
 * \brief	relay control
 * \param	[HD_System_RelayCtrl_e]Ctrl - control relay
 * \retval	none
 */
void HD_System_RelayCtrl(HD_System_RelayCtrl_e Ctrl);
/*!
 * \brief	SYSTEM init when wake up
 * \param	none		
 * \retval	none
 */
void HD_SYSTEM_InitWakeUp(void);
/*!
 * \brief	enter sleep mode
 * \param	none
 * \retval	none
 */
void HD_SYSTEM_Enter_Into_Sleep(void);
/*!
 * \brief	enable/disable irq
 * \param	[HD_System_Ctrl_e]Ctrl
 * \retval	none
 */
void HD_SYSTEM_IrqCtrl(HD_System_Ctrl_e Ctrl);
/*!
 * \brief	Clk dtc
 * \param	[HD_System_Clk_e]Clk - clk source
 * \retval	[uint8_t] - clk work status
 */
uint8_t HD_SYSTEM_ClkDtc(HD_System_Clk_e Clk);
/*!
 * \brief	MCU reset
 * \param	none
 * \retval	none
 */
void HD_SYSTEM_McuReset(void);

void HD_SYSTEM_SaveEnergyRegs(uint32_t RegValue, HD_System_RegName_e RegName);

uint32_t HD_SYSTEM_GetEnergyRegs(HD_System_RegName_e RegName);

void HD_SYSTEM_Systick_Config(void);

void HD_SYSTEM_deinit_before_jump(void);

void HD_SYSTEM_Jump2App(uint32_t AppAddr);

void HD_SYSTEM_FlashProgramByte(uint32_t Addr, uint8_t *Buff, uint32_t Length);

void HD_SYSTEM_EraseSector(uint32_t Addr);
/* End --------------------------------------------------------------------------*/