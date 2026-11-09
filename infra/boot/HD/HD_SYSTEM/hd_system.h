/*!
 * \file      hd_system.h
 *
 * \brief     hd_system implementation
 *
 * \endcode
 *
 * \author	   
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_SYSTEM_H_
#define _HD_SYSTEM_H_
/* Macro ------------------------------------------------------------------------*/
#define APP_START_ADDR         (0x00006000)
#define APP_CODE_START_ADDR    (0x00006000)
/* type prototype ---------------------------------------------------------------*/
typedef enum
{
    CHECK_1_TIME   = 0,
    CHECK_N_MS     = 1,
}HD_System_PwrFail_Dtc_e;

typedef enum
{
    RELAY_ON   = 0,
    RELAY_OFF  = 1, 
    RELAY_IDLE = 2,
}HD_System_RelayCtrl_e;

typedef enum
{
    HD_ENABLE  = 1,
    HD_DISABLE = 0,
}HD_System_Ctrl_e;

typedef enum
{
    HD_LSE  = 0,
    HD_HSE  = 1,
}HD_System_Clk_e;

typedef union
{
    uint8_t Group[2];
    struct
    {
        uint8_t badc  : 1;
        uint8_t bclk  : 1;
        uint8_t bgpio : 1;
        uint8_t biic  : 1;
        uint8_t blcd  : 1;
        uint8_t brtc  : 1;
        uint8_t bspi  : 1;
        uint8_t btim  : 1;
        
        uint8_t buart : 1;
        uint8_t brse0 : 1;
        uint8_t brse1 : 1;
        uint8_t brse2 : 1;
        uint8_t brse3 : 1;
        uint8_t brse4 : 1;
        uint8_t brse5 : 1;
        uint8_t brse6 : 1;
    }Bits;
}HD_System_PeripheralCtrl_t;

typedef enum
{
    HD_EGYPAH = 0,
    HD_EGYPAL,
    HD_EGYPBH,    
    HD_EGYPBL,    
    HD_EGYPCH,    
    HD_EGYPCL,  
    HD_EGYPS0H,
    HD_EGYPS0L,
    
    HD_CLRALL,
}HD_System_RegName_e;

typedef struct
{
	/*!
	 * \brief	SYSTEM init when reset
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitRst)(void);
	/*!
	 * \brief	SYSTEM init when power on
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrOn)(void);
	/*!
	 * \brief	SYSTEM init when power down
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrDown)(HD_System_PeripheralCtrl_t Ctrl);
	/*!
	 * \brief	SYSTEM init when wake up
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitWakeUp)(void);
	/*!
	 * \brief	delay N us
	 * \param	[uint16_t]Us - N us 	
	 * \retval	none
	 */
    void (*Delay_us)(uint16_t Us);
	/*!
	 * \brief	delay N ms
	 * \param	[uint16_t]Ms - N ms 	
	 * \retval	none
	 */
    void (*Delay_ms)(uint16_t Ms);
	/*!
	 * \brief	Clear watch dog
	 * \param	none		
	 * \retval	none
	 */
    void (*WDT_Clear)(void);
    /*!
     * \brief	power fail detect
     * \param	[HD_System_PwrFail_Dtc_e] Check - check type
     * \param	[uint8_t] Ms - ms
     * \retval	[uint8_t] - 1: powen on, 0: powen off
     */
    uint8_t (*PowerFail_Dtc)(HD_System_PwrFail_Dtc_e Check, uint8_t Ms);
	/*!
	 * \brief	low power mode configration
	 * \param	none		
	 * \retval	none
	 */
    void (*LowPowerCfg)(void);
	/*!
	 * \brief	low power mode configration
	 * \param	[uint8_t]En - 0:disable 1:enable	
	 * \retval	none
	 */
    void (*WDT_Config)(uint8_t En);
	/*!
	 * \brief	enable/disable second pulse output
	 * \param	[uint8_t]Enable - 0:disable 1:enable	
	 * \retval	none
	 */
    void (*SecPulse_Output)(uint8_t Enable);
	/*!
	 * \brief	delay count process
	 * \param	none	
	 * \retval	none
	 */
    void (*TimeProc)(void);
	/*!
	 * \brief	relay control
	 * \param	[HD_System_RelayCtrl_e]Ctrl - control relay
	 * \retval	none
	 */
    void (*RelayCtrl)(HD_System_RelayCtrl_e Ctrl);
	/*!
	 * \brief	enter sleep mode
	 * \param	none
	 * \retval	none
	 */
    void (*Enter_Into_Sleep)(void);
	/*!
	 * \brief	enable/disable irq
	 * \param	[HD_System_Ctrl_e]Ctrl
	 * \retval	none
	 */
    void (*IrqCtrl)(HD_System_Ctrl_e Ctrl);
	/*!
	 * \brief	Clk dtc
	 * \param	[HD_System_Clk_e]Clk - clk source
	 * \retval	[uint8_t] - clk work status
	 */
    uint8_t (*ClkDtc)(HD_System_Clk_e Clk);
	/*!
	 * \brief	MCU reset
	 * \param	none
	 * \retval	none
	 */
    void (*McuReset)(void);
    
    void (*SaveEnergyRegs)(uint32_t RegValue, HD_System_RegName_e RegName);
    
    uint32_t (*GetEnergyRegs)(HD_System_RegName_e RegName);
    
    void (*Systick_Config)(void);
    
    void (*Jump2App)(uint32_t AppAddr);
    
    uint32_t *AppRun;
    
    void (*Flash_ProgramByte)(uint32_t Addr, uint8_t *Buff, uint32_t Length);
    
    void (*Flash_EraseSector)(uint32_t Addr);
}HD_SYSTEM_API_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_SYSTEM_API_t g_stSystemApi;
/* End --------------------------------------------------------------------------*/
#endif
