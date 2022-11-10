/*!
 * \file      hd_gpio.h
 *
 * \brief     hd_gpio implementation
 *
 * \endcode
 *
 * \author	   
 */
/* Include ----------------------------------------------------------------------*/
#ifndef _HD_GPIO_H_
#define _HD_GPIO_H_
/* Macro ------------------------------------------------------------------------*/

/* type prototype ---------------------------------------------------------------*/
typedef enum
{
    ALARM_LED     = 0,
    RELAY_LED     = 1,
    SLOUT         = 2,
    UPKEY         = 3,
    DOWNKEY       = 4,
    V9203_IRQ0    = 5,
    NETLIGHT      = 6,
    
    SCL           = 7,
    SDA           = 8,
    SPCS1         = 9,
    SO_CTRL_CTL   = 10,
    ON            = 11,
    OFF           = 12,
    V9203_IRQ1    = 13,
    NB_Power      = 14,
    NB_Reset      = 15,
    FLASH_CS      = 16,
    FLASH_PWRCTRL = 17,
    FLASH_SDO     = 18,
    FLASH_CLK     = 19,
    FLASH_SDI     = 20,
    
    IO_MAX        = 21,
}HD_GPIO_IO_Tag_e;

typedef enum
{
    GPIO_OUTPUT = 0,
    GPIO_INPUT  = 1,
}HD_GPIO_Dir_e;

typedef enum
{
    GPIO_HIGH = 1,
    GPIO_LOW  = 0,
}HD_GPIO_Level_e;

typedef struct
{
    const HD_GPIO_IO_Tag_e IOx;
    HD_GPIO_Dir_e          Dir;
    HD_GPIO_Level_e        Lvl;
}HD_GPIO_IO_Ctrl_t;

typedef enum
{
    GPIO_FLASH_1S = 0,   //freqency 1s(0.5s high,0.5s low)
    GPIO_FLASH_2S,       //freqency 2s(1s high,1s low)
    GPIO_FLASH_MANUAL,   //control by manual
}HD_GPIO_Flash_Freq_e;

typedef struct
{
	/*!
	 * \brief	GPIO init when reset
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitRst)(void);
	/*!
	 * \brief	GPIO init when power on
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrOn)(void);
	/*!
	 * \brief	GPIO init when power down
	 * \param	none		
	 * \retval	none
	 */
    void (*pInitPwrDown)(void);
	/*!
	 * \brief	Ctrl IO
	 * \param	[HD_GPIO_IO_Ctrl_t]Ctrl - parameters of control 		
	 * \retval	none
	 */
    void (*IoCtrl)(HD_GPIO_IO_Ctrl_t *Ctrl);
    
    uint8_t (*GetInputValue)(HD_GPIO_IO_Ctrl_t *Ctrl);
    
    void (*OutputValue)(HD_GPIO_IO_Ctrl_t *Ctrl);

	void (*Flash)(HD_GPIO_IO_Ctrl_t *Ctrl, HD_GPIO_Flash_Freq_e Freq);

	void (*TimeProcIsr)(void);
}HD_GPIO_API_t;
/* Public Variables -------------------------------------------------------------*/
extern const HD_GPIO_API_t g_stGpioApi;

extern HD_GPIO_IO_Ctrl_t AlarmLed_t;
extern HD_GPIO_IO_Ctrl_t RelayLed_t;
extern HD_GPIO_IO_Ctrl_t Slout_t;
extern HD_GPIO_IO_Ctrl_t Upkey_t;
extern HD_GPIO_IO_Ctrl_t Downkey_t;
extern HD_GPIO_IO_Ctrl_t V9203Irq0_t;
extern HD_GPIO_IO_Ctrl_t NetLight_t;
extern HD_GPIO_IO_Ctrl_t Scl_t;
extern HD_GPIO_IO_Ctrl_t Sda_t;
extern HD_GPIO_IO_Ctrl_t Spcs1_t;
extern HD_GPIO_IO_Ctrl_t SO_Ctrl_Ctl_t;
extern HD_GPIO_IO_Ctrl_t On_t;
extern HD_GPIO_IO_Ctrl_t Off_t;
extern HD_GPIO_IO_Ctrl_t V9203Irq1_t;
extern HD_GPIO_IO_Ctrl_t NBPower_t;
extern HD_GPIO_IO_Ctrl_t NBReset_t;
extern HD_GPIO_IO_Ctrl_t Flash_CS_t;
extern HD_GPIO_IO_Ctrl_t Flash_PwrCtrl_t;
extern HD_GPIO_IO_Ctrl_t Flash_SDO_t;
extern HD_GPIO_IO_Ctrl_t Flash_CLK_t;
extern HD_GPIO_IO_Ctrl_t Flash_SDI_t;

/* End --------------------------------------------------------------------------*/
#endif
