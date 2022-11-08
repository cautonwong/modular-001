/*!
 * \file      hd_gpio_typedef.h
 *
 * \brief     hd_gpio implementation
 *
 * \endcode
 *
 * \author	   
 */
/* Include ----------------------------------------------------------------------*/

/* Macro ------------------------------------------------------------------------*/
#define TIME_CNT_0P5S_BASE_10MS     (50)
#define TIME_CNT_1S_BASE_10MS       (100)
/* type prototype ---------------------------------------------------------------*/
typedef enum
{
    HD_GPIOA = 0,
    HD_GPIOB = 1,
    HD_GPIOC = 2,
    HD_GPIOD = 3,
    HD_GPIOE = 4,
    HD_GPIOF = 5,
}HD_GPIO_Port_e;

typedef struct
{
    HD_GPIO_Port_e    IO_Port;
    uint32_t          *IO_DAT;
    uint32_t          IO_Pin;
    uint8_t           IO_PinNum;
    HD_GPIO_IO_Ctrl_t *IO_Ctrl;
}HD_GPIO_Config_Parameter_t;

typedef struct
{
    uint32_t TimeTick;
	uint8_t  Lock;
	uint8_t  LvlChng;
}HD_GPIO_Flash_Ctrl_t;

/* Private Variables ------------------------------------------------------------*/
//#if METER_PORT == 0
//const HD_GPIO_Config_Parameter_t IOCfgTbl[] = 
//{
//    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_ALARM_LED,    GPIO_NUM_11,   &AlarmLed_t,   },
//    {HD_GPIOD,    (uint32_t*)IOD_DAT,      IO_D_RELAY,        GPIO_NUM_13,   &RelayLed_t,   },
//    {HD_GPIOE,    (uint32_t*)IOE_DAT,      IO_E_SLOUT,        GPIO_NUM_12,   &Slout_t,      },
//    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_Prevs_DISP,   GPIO_NUM_5,    &Upkey_t,      },
//    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_NEXT_DISP,    GPIO_NUM_4,    &Downkey_t,    },
//    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_IRQ0,         GPIO_NUM_9,    &V9203Irq0_t,  },
//    {HD_GPIOD,    (uint32_t*)IOD_DAT,      IO_D_NETLIGHT,     GPIO_NUM_14,   &NetLight_t,   },
//    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_SCL,          GPIO_NUM_4,    &Scl_t,        },
//    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_SDA,          GPIO_NUM_5,    &Sda_t,        },
//    {HD_GPIOB,    (uint32_t*)IOB_DAT,      IO_B_SPCS1,        GPIO_NUM_9,    &Spcs1_t,      },
//    {HD_GPIOD,    (uint32_t*)IOD_DAT,      IO_D_SO_CTRL_CTL,  GPIO_NUM_15,   &SO_Ctrl_Ctl_t,},
//    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_ON,           GPIO_NUM_6,    &On_t,         },
//    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_OFF,          GPIO_NUM_14,   &Off_t,        },
//    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_IRQ1,         GPIO_NUM_11,   &V9203Irq1_t,  },
//    {HD_GPIOB,    (uint32_t*)IOB_DAT,      IO_B_NB_POWER_CTRL,GPIO_NUM_13,   &NBPower_t,    },
//    {HD_GPIOB,    (uint32_t*)IOB_DAT,      IO_B_RESET_NB_CTRL,GPIO_NUM_14,   &NBReset_t,    },
//};
//#elif METER_PORT == 1
 /* const HD_GPIO_Config_Parameter_t IOCfgTbl[] =
{
  
  {HD_GPIOD,    (uint32_t*)IOD_DAT,      IO_D_ALARM_LED,      GPIO_NUM_13,   &AlarmLed_t,     },
//    {HD_GPIOF,    (uint32_t*)IOF_DAT,      IO_F_RELAY,          GPIO_NUM_0,    &RelayLed_t,     },
    {HD_GPIOD,    (uint32_t*)IOD_DAT,      IO_D_SLOUT,          GPIO_NUM_14,   &Slout_t,        },
    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_Prevs_DISP,     GPIO_NUM_8,    &Upkey_t,        },
    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_NEXT_DISP,      GPIO_NUM_2,    &Downkey_t,      },
    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_IRQ0,           GPIO_NUM_9,    &V9203Irq0_t,    },
//    {HD_GPIOF,    (uint32_t*)IOF_DAT,      IO_F_NETLIGHT,       GPIO_NUM_1,    &NetLight_t,     },
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_SCL,            GPIO_NUM_4,    &Scl_t,          },
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_SDA,            GPIO_NUM_5,    &Sda_t,          },
    {HD_GPIOB,    (uint32_t*)IOB_DAT,      IO_B_SPCS1,          GPIO_NUM_9,    &Spcs1_t,        },
    {HD_GPIOD,    (uint32_t*)IOD_DAT,      IO_D_SO_CTRL_CTL,    GPIO_NUM_15,   &SO_Ctrl_Ctl_t,  },
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_ON,             GPIO_NUM_6,    &On_t,           },
    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_OFF,            GPIO_NUM_14,   &Off_t,          },
    {HD_GPIOA,    (uint32_t*)PMU_IOADAT,   IO_A_IRQ1,           GPIO_NUM_11,   &V9203Irq1_t,    },
    {HD_GPIOB,    (uint32_t*)IOB_DAT,      IO_B_NB_POWER_CTRL,  GPIO_NUM_13,   &NBPower_t,      },
    {HD_GPIOB,    (uint32_t*)IOB_DAT,      IO_B_RESET_NB_CTRL,  GPIO_NUM_14,   &NBReset_t,      },
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_FLASH_CS,       GPIO_NUM_8,    &Flash_CS_t,     },
    {HD_GPIOE,    (uint32_t*)IOE_DAT,      IO_E_FLASH_PWR_CTRL, GPIO_NUM_12,   &Flash_PwrCtrl_t,},
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_FLASH_SDO,      GPIO_NUM_10,   &Flash_SDO_t,    },
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_FLASH_CLK,      GPIO_NUM_7,    &Flash_CLK_t,    },
    {HD_GPIOC,    (uint32_t*)IOC_DAT,      IO_C_FLASH_SDI,      GPIO_NUM_9,    &Flash_SDI_t,    },
  
};*/
//#endif
HD_GPIO_Flash_Ctrl_t FlashCtrl;

/* Public Variables -------------------------------------------------------------*/
HD_GPIO_IO_Ctrl_t AlarmLed_t      = {.IOx = ALARM_LED    };
HD_GPIO_IO_Ctrl_t RelayLed_t      = {.IOx = RELAY_LED    };
HD_GPIO_IO_Ctrl_t Slout_t         = {.IOx = SLOUT        };
HD_GPIO_IO_Ctrl_t Upkey_t         = {.IOx = UPKEY        };
HD_GPIO_IO_Ctrl_t Downkey_t       = {.IOx = DOWNKEY      };
HD_GPIO_IO_Ctrl_t V9203Irq0_t     = {.IOx = V9203_IRQ0   };
HD_GPIO_IO_Ctrl_t NetLight_t      = {.IOx = NETLIGHT     };
HD_GPIO_IO_Ctrl_t Scl_t           = {.IOx = SCL          };
HD_GPIO_IO_Ctrl_t Sda_t           = {.IOx = SDA          };
HD_GPIO_IO_Ctrl_t Spcs1_t         = {.IOx = SPCS1        };
HD_GPIO_IO_Ctrl_t SO_Ctrl_Ctl_t   = {.IOx = SO_CTRL_CTL  };
HD_GPIO_IO_Ctrl_t On_t            = {.IOx = ON           };
HD_GPIO_IO_Ctrl_t Off_t           = {.IOx = OFF          };
HD_GPIO_IO_Ctrl_t V9203Irq1_t     = {.IOx = V9203_IRQ1   };
HD_GPIO_IO_Ctrl_t NBPower_t       = {.IOx = NB_Power     };
HD_GPIO_IO_Ctrl_t NBReset_t       = {.IOx = NB_Reset     };
//#if METER_PORT == 1
HD_GPIO_IO_Ctrl_t Flash_CS_t      = {.IOx = FLASH_CS     }; 
HD_GPIO_IO_Ctrl_t Flash_PwrCtrl_t = {.IOx = FLASH_PWRCTRL}; 
HD_GPIO_IO_Ctrl_t Flash_SDO_t     = {.IOx = FLASH_SDO    }; 
HD_GPIO_IO_Ctrl_t Flash_CLK_t     = {.IOx = FLASH_CLK    }; 
HD_GPIO_IO_Ctrl_t Flash_SDI_t     = {.IOx = FLASH_SDI    }; 
//#endif
/* Function ---------------------------------------------------------------------*/
/*!
 * \brief	GPIO init when reset
 * \param	none		
 * \retval	none
 */
void HD_GPIO_InitRst(void);
/*!
 * \brief	GPIO init when power on
 * \param	none		
 * \retval	none
 */
void HD_GPIO_InitPwrOn(void);
/*!
 * \brief	GPIO init when power down
 * \param	none		
 * \retval	none
 */
void HD_GPIO_InitPwrDown(void);
/*!
 * \brief	Ctrl IO
 * \param	[HD_GPIO_IO_Ctrl_t]Ctrl - parameters of control 		
 * \retval	none
 */
void HD_GPIO_IoCtrl(HD_GPIO_IO_Ctrl_t *Ctrl);

uint8_t HD_GPIO_GetInputValue(HD_GPIO_IO_Ctrl_t *Ctrl);
//[uint8_t] - get value of io when the io is configured to input
void HD_GPIO_OutputValue(HD_GPIO_IO_Ctrl_t *Ctrl);

void HD_GPIO_Flash(HD_GPIO_IO_Ctrl_t *Ctrl, HD_GPIO_Flash_Freq_e Freq);

void HD_GPIO_TimeProcIsr(void);
/* End --------------------------------------------------------------------------*/