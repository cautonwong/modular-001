/*!
 * \file      hd_gpio_list.h
 *
 * \brief     hd_gpio_list implementation
 *
 * \endcode
 *
 * \author	  hxiii 
 */
/* Macro ------------------------------------------------------------------------*/
/* IO bit manipulation*/
#define IO_DAT(IOx, PINx)        *((volatile unsigned long *)((((uint32_t)IOx) & 0xF0000000) + \
                                    0x2000000 + ((((uint32_t)IOx) & 0xFFFFF) << 5) + (PINx << 2))) 
/* GPIO number*/
#define GPIO_NUM_0               (0)
#define GPIO_NUM_1               (1)
#define GPIO_NUM_2               (2)
#define GPIO_NUM_3               (3)
#define GPIO_NUM_4               (4)
#define GPIO_NUM_5               (5)
#define GPIO_NUM_6               (6)
#define GPIO_NUM_7               (7)
#define GPIO_NUM_8               (8)
#define GPIO_NUM_9               (9)
#define GPIO_NUM_10              (10)
#define GPIO_NUM_11              (11)
#define GPIO_NUM_12              (12)
#define GPIO_NUM_13              (13)
#define GPIO_NUM_14              (14)
#define GPIO_NUM_15              (15)

//#if METER_PORT == 0
///* GPIOA-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
//#define IO_A_SO_CTRL             GPIO_Pin_3
//#define Set_Input_SO_CTRL        {GPIOA->OEN |= IO_A_SO_CTRL; GPIOA->IE |= IO_A_SO_CTRL; \
//                                  GPIOA->ATT |= IO_A_SO_CTRL;}
//#define Set_Output_SO_CTRL       {GPIOA->OEN &= ~IO_A_SO_CTRL; GPIOA->IE &= ~IO_A_SO_CTRL; \
//                                  GPIOA->ATT &= ~IO_A_SO_CTRL;}
//#define IO_SO_CTRL_H             {IO_DAT(PMU_IOADAT, GPIO_NUM_3) = 1;}
//#define IO_SO_CTRL_L             {IO_DAT(PMU_IOADAT, GPIO_NUM_3) = 0;}
//#define IO_SO_CTRL_Read          ((GPIOA->STS & IO_A_SO_CTRL) >> GPIO_NUM_3)
///*---------------------ÏÂ·­¼üÅäÖÃ----------------------------------*/
//#define IO_A_NEXT_DISP				GPIO_Pin_4
//#define Set_Input_NEXT_DISP			{GPIOA->OEN |= IO_A_NEXT_DISP; GPIOA->IE |= IO_A_NEXT_DISP; \
//									GPIOA->ATT |= IO_A_NEXT_DISP;}
//#define Set_Output_NEXT_DISP		{GPIOA->OEN &= ~IO_A_NEXT_DISP; GPIOA->IE &= ~IO_A_NEXT_DISP; \
//									GPIOA->ATT &= ~IO_A_NEXT_DISP;}
//#define IO_NEXT_DISP_H				{IO_DAT(PMU_IOADAT, GPIO_NUM_4) = 1;}
//#define IO_NEXT_DISP_L				{IO_DAT(PMU_IOADAT, GPIO_NUM_4) = 0;}
//#define IO_NEXT_DISP_Read			((GPIOA->STS & IO_A_NEXT_DISP) >> GPIO_NUM_4)
///*---------------------ÉÏ·­¼üÅäÖÃ---------------------------------------*/
//#define IO_A_Prevs_DISP				GPIO_Pin_5
//#define Set_Input_Prevs_DISP		{GPIOA->OEN |= IO_A_Prevs_DISP; GPIOA->IE |= IO_A_Prevs_DISP; \
//									GPIOA->ATT |= IO_A_Prevs_DISP;}
//#define Set_Output_Prevs_DISP		{GPIOA->OEN &= ~IO_A_Prevs_DISP; GPIOA->IE &= ~IO_A_Prevs_DISP; \
//									GPIOA->ATT &= ~IO_A_Prevs_DISP;}
//#define IO_Prevs_DISP_En_Interupt	(GPIOA->WKUEN |= 0x0400)
//#define IO_Prevs_DISP_Disb_Interupt	(GPIOA->WKUEN &= ~0x0400)
//#define IO_Prevs_DISP_En_Debounce	(GPIOA->IOANODEG &= ~IO_A_Prevs_DISP)
//#define IO_Prevs_DISP_Disb_Debounce	(GPIOA->IOANODEG |= IO_A_Prevs_DISP)
//#define IO_Prevs_DISP_H				{IO_DAT(PMU_IOADAT, GPIO_NUM_5) = 1;}
//#define IO_Prevs_DISP_L				{IO_DAT(PMU_IOADAT, GPIO_NUM_5) = 0;}
//#define IO_Prevs_DISP_Read			((GPIOA->STS & IO_A_Prevs_DISP) >> GPIO_NUM_5)
///*-------------------------------------------------------------------------*/
//#define IO_A_CF1                 GPIO_Pin_6
//#define Set_Input_CF1            {GPIOA->OEN |= IO_A_CF1; GPIOA->IE |= IO_A_CF1; \
//                                  GPIOA->ATT |= IO_A_CF1;}
///*-------------------------------------------------------------------------*/
//#define IO_A_SEG10               GPIO_Pin_7
///*-------------------------------------------------------------------------*/
//#define IO_A_IRQ0                GPIO_Pin_9
//#define Set_Input_IRQ0           {GPIOA->OEN |= IO_A_IRQ0; GPIOA->IE |= IO_A_IRQ0; \
//                                  GPIOA->ATT |= IO_A_IRQ0;}
//#define IO_IRQ0_Read             ((GPIOA->STS & IO_A_IRQ0) >> GPIO_NUM_9)
///*-------------------------------------------------------------------------*/
//#define IO_A_IRQ1                GPIO_Pin_11
//#define Set_Input_IRQ1           {GPIOA->OEN |= IO_A_IRQ1; GPIOA->IE |= IO_A_IRQ1; \
//                                  GPIOA->ATT |= IO_A_IRQ1;}
//#define IO_IRQ1_Read             ((GPIOA->STS & IO_A_IRQ1) >> GPIO_NUM_11)
///*-------------------------------------------------------------------------*/
//#define IO_A_SEG8                GPIO_Pin_12
///*-------------------------------------------------------------------------*/
//#define IO_A_SEG6                GPIO_Pin_13
///*-------------------------------------------------------------------------*/
//#define IO_A_OFF                 GPIO_Pin_14
//#define Set_Input_OFF            {GPIOA->OEN |= IO_A_OFF; GPIOA->IE |= IO_A_OFF; \
//                                  GPIOA->ATT |= IO_A_OFF;}
//#define Set_Output_OFF           {GPIOA->OEN &= ~IO_A_OFF; GPIOA->IE &= ~IO_A_OFF; \
//                                  GPIOA->ATT &= ~IO_A_OFF;}
//#define IO_OFF_H                 {IO_DAT(PMU_IOADAT, GPIO_NUM_14) = 1;}
//#define IO_OFF_L                 {IO_DAT(PMU_IOADAT, GPIO_NUM_14) = 0;}
//#define IO_OFF_Read              ((GPIOA->STS & IO_A_OFF) >> GPIO_NUM_14)
///*-------------------------------------------------------------------------*/
//#define IO_A_RXD0                GPIO_Pin_15
//#define Set_Input_RXD0           {GPIOA->OEN |= IO_A_RXD0; GPIOA->IE |= IO_A_RXD0; \
//                                  GPIOA->ATT |= IO_A_RXD0;}
///*-------------------------------------------------------------------------*/
///* GPIOB-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
//#define IO_B_BT_TXD              GPIO_Pin_0
//#define Set_Input_BT_TXD         {GPIOB->OEN |= IO_B_BT_TXD; GPIOB->IE |= IO_B_BT_TXD; \
//                                  GPIOB->ATT |= IO_B_BT_TXD;}
//#define Set_Output_BT_TXD        {GPIOB->OEN &= ~IO_B_BT_TXD; GPIOB->IE &= ~IO_B_BT_TXD; \
//                                  GPIOB->ATT &= ~IO_B_BT_TXD;}
//#define IO_BT_TXD_H              {IO_DAT(IOB_DAT, GPIO_NUM_0) = 1;}
//#define IO_BT_TXD_L              {IO_DAT(IOB_DAT, GPIO_NUM_0) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_485RXD1             GPIO_Pin_1
//#define Set_Input_485RXD1        {GPIOB->OEN |= IO_B_485RXD1; GPIOB->IE |= IO_B_485RXD1; \
//                                  GPIOB->ATT |= IO_B_485RXD1;}
//#define Set_Output_485RXD1       {GPIOB->OEN &= ~IO_B_485RXD1; GPIOB->IE &= ~IO_B_485RXD1; \
//                                  GPIOB->ATT &= ~IO_B_485RXD1;}
//#define IO_485RXD1_H             {IO_DAT(IOB_DAT, GPIO_NUM_1) = 1;}
//#define IO_485RXD1_L             {IO_DAT(IOB_DAT, GPIO_NUM_1) = 0;}
//#define IO_485RXD1_Read          ((GPIOB->STS & IO_B_485RXD1) >> GPIO_NUM_1)
///*-------------------------------------------------------------------------*/
//#define IO_B_SEG9                GPIO_Pin_2
///*-------------------------------------------------------------------------*/
//#define IO_B_SEG7                GPIO_Pin_3
///*-------------------------------------------------------------------------*/
//#define IO_B_CF_CTRL             GPIO_Pin_4
//#define Set_Input_CF_CTRL        {GPIOB->OEN |= IO_B_CF_CTRL; GPIOB->IE |= IO_B_CF_CTRL; \
//                                  GPIOB->ATT |= IO_B_CF_CTRL;}
//#define Set_Output_CF_CTRL       {GPIOB->OEN &= ~IO_B_CF_CTRL; GPIOB->IE &= ~IO_B_CF_CTRL; \
//                                  GPIOB->ATT &= ~IO_B_CF_CTRL;}
//#define IO_CF_CTRL_H             {IO_DAT(IOB_DAT, GPIO_NUM_4) = 1;}
//#define IO_CF_CTRL_L             {IO_DAT(IOB_DAT, GPIO_NUM_4) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_TXD0                GPIO_Pin_5
//#define Set_Output_TXD0          {GPIOB->OEN &= ~IO_B_TXD0; GPIOB->IE &= ~IO_B_TXD0; \
//                                  GPIOB->ATT &= ~IO_B_TXD0;}
//#define IO_TXD0_H                {IO_DAT(IOB_DAT, GPIO_NUM_5) = 1;}
//#define IO_TXD0_L                {IO_DAT(IOB_DAT, GPIO_NUM_5) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_BT_RXD              GPIO_Pin_6
//#define Set_Input_BT_RXD         {GPIOB->OEN |= IO_B_BT_RXD; GPIOB->IE |= IO_B_BT_RXD; \
//                                  GPIOB->ATT |= IO_B_BT_RXD;}
//#define Set_Output_BT_RXD        {GPIOB->OEN &= ~IO_B_BT_RXD; GPIOB->IE &= ~IO_B_BT_RXD; \
//                                  GPIOB->ATT &= ~IO_B_BT_RXD;}
//#define IO_BT_RXD_H              {IO_DAT(IOB_DAT, GPIO_NUM_6) = 1;}
//#define IO_BT_RXD_L              {IO_DAT(IOB_DAT, GPIO_NUM_6) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_485TXD1             GPIO_Pin_7
//#define Set_Output_485TXD1       {GPIOB->OEN &= ~IO_B_485TXD1; GPIOB->IE &= ~IO_B_485TXD1; \
//                                  GPIOB->ATT &= ~IO_B_485TXD1;}
//#define IO_485TXD1_H             {IO_DAT(IOB_DAT, GPIO_NUM_7) = 1;}
//#define IO_485TXD1_L             {IO_DAT(IOB_DAT, GPIO_NUM_7) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_SPCS1               GPIO_Pin_9
//#define Set_Input_SPCS1          {GPIOB->OEN |= IO_B_SPCS1; GPIOB->IE |= IO_B_SPCS1; \
//                                  GPIOB->ATT |= IO_B_SPCS1;}
//#define Set_Output_SPCS1         {GPIOB->OEN &= ~IO_B_SPCS1; GPIOB->IE &= ~IO_B_SPCS1; \
//                                  GPIOB->ATT &= ~IO_B_SPCS1;}
//#define IO_SPCS1_H               {IO_DAT(IOB_DAT, GPIO_NUM_9) = 1;}
//#define IO_SPCS1_L               {IO_DAT(IOB_DAT, GPIO_NUM_9) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_SPCK1               GPIO_Pin_10
//#define Set_Output_SPCK1         {GPIOB->OEN &= ~IO_B_SPCK1; GPIOB->IE &= ~IO_B_SPCK1; \
//                                  GPIOB->ATT &= ~IO_B_SPCK1;}
//#define IO_SPCK1_H               {IO_DAT(IOB_DAT, GPIO_NUM_10) = 1;}
//#define IO_SPCK1_L               {IO_DAT(IOB_DAT, GPIO_NUM_10) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_SPDO1               GPIO_Pin_11
//#define Set_Input_SPDO1          {GPIOB->OEN |= IO_B_SPDO1; GPIOB->IE |= IO_B_SPDO1; \
//                                  GPIOB->ATT |= IO_B_SPDO1;}
//#define Set_Output_SPDO1         {GPIOB->OEN &= ~IO_B_SPDO1; GPIOB->IE &= ~IO_B_SPDO1; \
//                                  GPIOB->ATT &= ~IO_B_SPDO1;}
//#define IO_SPDO1_H               {IO_DAT(IOB_DAT, GPIO_NUM_11) = 1;}
//#define IO_SPDO1_L               {IO_DAT(IOB_DAT, GPIO_NUM_11) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_SPDI1               GPIO_Pin_12
//#define Set_Input_SPDI1          {GPIOB->OEN |= IO_B_SPDI1; GPIOB->IE |= IO_B_SPDI1; \
//                                  GPIOB->ATT |= IO_B_SPDI1;}
//#define Set_Output_SPDI1         {GPIOB->OEN &= ~IO_B_SPDI1; GPIOB->IE &= ~IO_B_SPDI1; \
//                                  GPIOB->ATT &= ~IO_B_SPDI1;}
//#define IO_SPDI1_H               {IO_DAT(IOB_DAT, GPIO_NUM_12) = 1;}
//#define IO_SPDI1_L               {IO_DAT(IOB_DAT, GPIO_NUM_12) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_NB_POWER_CTRL       GPIO_Pin_13
//#define Set_Output_NB_POWER_CTRL {GPIOB->OEN &= ~IO_B_NB_POWER_CTRL; GPIOB->IE &= ~IO_B_NB_POWER_CTRL; \
//                                  GPIOB->ATT &= ~IO_B_NB_POWER_CTRL;}
//#define IO_NB_POWER_CTRL_H       {IO_DAT(IOB_DAT, GPIO_NUM_13) = 1;}
//#define IO_NB_POWER_CTRL_L       {IO_DAT(IOB_DAT, GPIO_NUM_13) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_RESET_NB_CTRL       GPIO_Pin_14
//#define Set_Output_RESET_NB_CTRL {GPIOB->OEN &= ~IO_B_RESET_NB_CTRL; GPIOB->IE &= ~IO_B_RESET_NB_CTRL; \
//                                  GPIOB->ATT &= ~IO_B_RESET_NB_CTRL;}
//#define IO_RESET_NB_CTRL_H       {IO_DAT(IOB_DAT, GPIO_NUM_14) = 1;}
//#define IO_RESET_NB_CTRL_L       {IO_DAT(IOB_DAT, GPIO_NUM_14) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_B_BT_RST              GPIO_Pin_15
//#define Set_Input_BT_RST         {GPIOB->OEN |= IO_B_BT_RST; GPIOB->IE |= IO_B_BT_RST; \
//                                  GPIOB->ATT |= IO_B_BT_RST;}
//#define Set_Output_BT_RST        {GPIOB->OEN &= ~IO_B_BT_RST; GPIOB->IE &= ~IO_B_BT_RST; \
//                                  GPIOB->ATT &= ~IO_B_BT_RST;}
//#define IO_BT_RST_H              {IO_DAT(IOB_DAT, GPIO_NUM_15) = 1;}
//#define IO_BT_RST_L              {IO_DAT(IOB_DAT, GPIO_NUM_15) = 0;}
///*-------------------------------------------------------------------------*/
///* GPIOC-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
//#define IO_C_DMACS1              GPIO_Pin_0
//#define Set_Input_DMACS1         {GPIOC->OEN |= IO_C_DMACS1; GPIOC->IE |= IO_C_DMACS1; \
//                                  GPIOC->ATT |= IO_C_DMACS1;}
//#define Set_Output_DMACS1        {GPIOC->OEN &= ~IO_C_DMACS1; GPIOC->IE &= ~IO_C_DMACS1; \
//                                  GPIOC->ATT &= ~IO_C_DMACS1;}
//#define IO_DMACS1_H              {IO_DAT(IOC_DAT, GPIO_NUM_0) = 1;}
//#define IO_DMACS1_L              {IO_DAT(IOC_DAT, GPIO_NUM_0) = 0;}
//#define IO_DMACS1_Read           ((GPIOC->STS & IO_C_DMACS1) >> GPIO_NUM_0)
///*-------------------------------------------------------------------------*/
//#define IO_C_DMACK1              GPIO_Pin_1
//#define Set_Input_DMACK1         {GPIOC->OEN |= IO_C_DMACK1; GPIOC->IE |= IO_C_DMACK1; \
//                                  GPIOC->ATT |= IO_C_DMACK1;}
//#define Set_Output_DMACK1        {GPIOC->OEN &= ~IO_C_DMACK1; GPIOC->IE &= ~IO_C_DMACK1; \
//                                  GPIOC->ATT &= ~IO_C_DMACK1;}
//#define IO_DMACK1_H              {IO_DAT(IOC_DAT, GPIO_NUM_1) = 1;}
//#define IO_DMACK1_L              {IO_DAT(IOC_DAT, GPIO_NUM_1) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_C_DMADO1              GPIO_Pin_3
//#define Set_Input_DMADO1         {GPIOC->OEN |= IO_C_DMADO1; GPIOC->IE |= IO_C_DMADO1; \
//                                  GPIOC->ATT |= IO_C_DMADO1;}
//#define Set_Output_DMADO1        {GPIOC->OEN &= ~IO_C_DMADO1; GPIOC->IE &= ~IO_C_DMADO1; \
//                                  GPIOC->ATT &= ~IO_C_DMADO1;}
//#define IO_DMADO1_H              {IO_DAT(IOC_DAT, GPIO_NUM_3) = 1;}
//#define IO_DMADO1_L              {IO_DAT(IOC_DAT, GPIO_NUM_3) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_C_SCL                 GPIO_Pin_4
//#define Set_Input_SCL            {GPIOC->OEN |= IO_C_SCL; GPIOC->IE |= IO_C_SCL; \
//                                  GPIOC->ATT |= IO_C_SCL;}
//#define Set_Output_SCL           {GPIOC->OEN &= ~IO_C_SCL; GPIOC->IE &= ~IO_C_SCL; \
//                                  GPIOC->ATT &= ~IO_C_SCL;}
//#define IO_SCL_H                 {IO_DAT(IOC_DAT, GPIO_NUM_4) = 1;}
//#define IO_SCL_L                 {IO_DAT(IOC_DAT, GPIO_NUM_4) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_C_SDA                 GPIO_Pin_5
//#define Set_Input_SDA            {GPIOC->OEN |= IO_C_SDA; GPIOC->IE |= IO_C_SDA; \
//                                  GPIOC->ATT |= IO_C_SDA;}
//#define Set_Output_SDA           {GPIOC->OEN &= ~IO_C_SDA; GPIOC->IE &= ~IO_C_SDA; \
//                                  GPIOC->ATT &= ~IO_C_SDA;}
//#define IO_SDA_H                 {IO_DAT(IOC_DAT, GPIO_NUM_5) = 1;}
//#define IO_SDA_L                 {IO_DAT(IOC_DAT, GPIO_NUM_5) = 0;}
//#define IO_SDA_Read              ((GPIOC->STS & IO_C_SDA) >> GPIO_NUM_5)
///*-------------------------------------------------------------------------*/
//#define IO_C_ON                  GPIO_Pin_6
//#define Set_Output_ON            {GPIOC->OEN &= ~IO_C_ON; GPIOC->IE &= ~IO_C_ON; \
//                                  GPIOC->ATT &= ~IO_C_ON;}
//#define IO_ON_H                  {IO_DAT(IOC_DAT, GPIO_NUM_6) = 1;}
//#define IO_ON_L                  {IO_DAT(IOC_DAT, GPIO_NUM_6) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_C_SEG11               GPIO_Pin_7
///*-------------------------------------------------------------------------*/
//#define IO_C_SEG12               GPIO_Pin_8
///*-------------------------------------------------------------------------*/
//#define IO_C_SEG13               GPIO_Pin_9
///*-------------------------------------------------------------------------*/
//#define IO_C_SEG14               GPIO_Pin_10
///*-------------------------------------------------------------------------*/
//#define IO_C_ALARM_LED           GPIO_Pin_11
//#define Set_Output_ALARM_LED     {GPIOC->OEN &= ~IO_C_ALARM_LED; GPIOC->IE &= ~IO_C_ALARM_LED; \
//                                  GPIOC->ATT &= ~IO_C_ALARM_LED;}
//#define IO_ALARM_LED_H           {IO_DAT(IOC_DAT, GPIO_NUM_11) = 1;}
//#define IO_ALARM_LED_L           {IO_DAT(IOC_DAT, GPIO_NUM_11) = 0;}
///*-------------------------------------------------------------------------*/
///* GPIOD-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
//#define IO_D_COM6                GPIO_Pin_0
///*-------------------------------------------------------------------------*/
//#define IO_D_COM5                GPIO_Pin_1
///*-------------------------------------------------------------------------*/
//#define IO_D_COM4                GPIO_Pin_2
///*-------------------------------------------------------------------------*/
//#define IO_D_COM3                GPIO_Pin_3
///*-------------------------------------------------------------------------*/
//#define IO_D_COM2                GPIO_Pin_4
///*-------------------------------------------------------------------------*/
//#define IO_D_COM1                GPIO_Pin_5
///*-------------------------------------------------------------------------*/
//#define IO_D_SEG3                GPIO_Pin_6
///*-------------------------------------------------------------------------*/
//#define IO_D_SEG2                GPIO_Pin_7
///*-------------------------------------------------------------------------*/
//#define IO_D_SEG1                GPIO_Pin_12
///*-------------------------------------------------------------------------*/
//#define IO_D_RELAY               GPIO_Pin_13
//#define Set_Output_RELAY         {GPIOD->OEN &= ~IO_D_RELAY; GPIOD->IE &= ~IO_D_RELAY; \
//                                  GPIOD->ATT &= ~IO_D_RELAY;}
//#define IO_RELAY_H               {IO_DAT(IOD_DAT, GPIO_NUM_13) = 1;}
//#define IO_RELAY_L               {IO_DAT(IOD_DAT, GPIO_NUM_13) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_D_NETLIGHT            GPIO_Pin_14
//#define Set_Input_NETLIGHT       {GPIOD->OEN |= IO_D_NETLIGHT; GPIOD->IE |= IO_D_NETLIGHT; \
//                                  GPIOD->ATT |= IO_D_NETLIGHT;}
//#define Set_Output_NETLIGHT      {GPIOD->OEN &= ~IO_D_NETLIGHT; GPIOD->IE &= ~IO_D_NETLIGHT; \
//                                  GPIOD->ATT &= ~IO_D_NETLIGHT;}
//#define IO_NETLIGHT_H            {IO_DAT(IOD_DAT, GPIO_NUM_14) = 1;}
//#define IO_NETLIGHT_L            {IO_DAT(IOD_DAT, GPIO_NUM_14) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_D_SO_CTRL_CTL         GPIO_Pin_15
//#define Set_Input_SO_CTRL_CTL    {GPIOD->OEN |= IO_D_SO_CTRL_CTL; GPIOD->IE |= IO_D_SO_CTRL_CTL; \
//                                  GPIOD->ATT |= IO_D_SO_CTRL_CTL;}
//#define Set_Output_SO_CTRL_CTL   {GPIOD->OEN &= ~IO_D_SO_CTRL_CTL; GPIOD->IE &= ~IO_D_SO_CTRL_CTL; \
//                                  GPIOD->ATT &= ~IO_D_SO_CTRL_CTL;}
//#define IO_SO_CTRL_CTL_H         {IO_DAT(IOD_DAT, GPIO_NUM_15) = 1;}
//#define IO_SO_CTRL_CTL_L         {IO_DAT(IOD_DAT, GPIO_NUM_15) = 0;}
///*-------------------------------------------------------------------------*/
///* GPIOE-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
//#define IO_E_SEG5                GPIO_Pin_0
///*-------------------------------------------------------------------------*/
//#define IO_E_SEG4                GPIO_Pin_1
///*-------------------------------------------------------------------------*/
//#define IO_E_VDCIN               GPIO_Pin_5
//#define Set_Input_VDCIN          {GPIOE->OEN |= IO_E_VDCIN; GPIOE->IE |= IO_E_VDCIN; \
//                                  GPIOE->ATT |= IO_E_VDCIN;}
///*-------------------------------------------------------------------------*/
//#define IO_E_INPUT1              GPIO_Pin_6
//#define Set_Input_INPUT1         {GPIOE->OEN |= IO_E_INPUT1; GPIOE->IE |= IO_E_INPUT1; \
//                                  GPIOE->ATT |= IO_E_INPUT1;}
//#define IO_INPUT1_Read           ((GPIOE->STS & IO_E_INPUT1) >> GPIO_NUM_6)
///*-------------------------------------------------------------------------*/
//#define IO_E_INPUT2              GPIO_Pin_7
//#define Set_Input_INPUT2         {GPIOE->OEN |= IO_E_INPUT1; GPIOE->IE |= IO_E_INPUT1; \
//                                  GPIOE->ATT |= IO_E_INPUT1;}
//#define IO_INPUT2_Read           ((GPIOE->STS & IO_E_INPUT2) >> GPIO_NUM_7)
///*-------------------------------------------------------------------------*/
//#define IO_E_AT_EN               GPIO_Pin_9
//#define Set_Input_AT_EN          {GPIOE->OEN |= IO_E_AT_EN; GPIOE->IE |= IO_E_AT_EN; \
//                                  GPIOE->ATT |= IO_E_AT_EN;}
//#define Set_Output_AT_EN         {GPIOE->OEN &= ~IO_E_AT_EN; GPIOE->IE &= ~IO_E_AT_EN; \
//                                  GPIOE->ATT &= ~IO_E_AT_EN;}
//#define IO_AT_EN_H               {IO_DAT(IOE_DAT, GPIO_NUM_9) = 1;}
//#define IO_AT_EN_L               {IO_DAT(IOE_DAT, GPIO_NUM_9) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_E_SLOUT               GPIO_Pin_12
//#define Set_Output_SLOUT         {GPIOE->OEN &= ~IO_E_SLOUT; GPIOE->IE &= ~IO_E_SLOUT; \
//                                  GPIOE->ATT &= ~IO_E_SLOUT;}
//#define IO_SLOUT_H               {IO_DAT(IOE_DAT, GPIO_NUM_12) = 1;}
//#define IO_SLOUT_L               {IO_DAT(IOE_DAT, GPIO_NUM_12) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_E_PWRKEY_CTRL         GPIO_Pin_13
//#define Set_Output_PWRKEY_CTRL   {GPIOE->OEN &= ~IO_E_PWRKEY_CTRL; GPIOE->IE &= ~IO_E_PWRKEY_CTRL; \
//                                  GPIOE->ATT &= ~IO_E_PWRKEY_CTRL;}
//#define IO_PWRKEY_CTRL_H         {IO_DAT(IOE_DAT, GPIO_NUM_13) = 1;}
//#define IO_PWRKEY_CTRL_L         {IO_DAT(IOE_DAT, GPIO_NUM_13) = 0;}
///*-------------------------------------------------------------------------*/
//#define IO_E_SEG16               GPIO_Pin_14
///*-------------------------------------------------------------------------*/
//#define IO_E_SEG15               GPIO_Pin_15
///* GPIOF-------------------------------------------------------------------------*/
///*-------------------------------------------------------------------------*/
//#define IO_F0                    GPIO_Pin_0
///*-------------------------------------------------------------------------*/
//#define IO_F1                    GPIO_Pin_1
//#elif METER_PORT == 1
/* GPIOA-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
#define IO_A_NEXT_DISP           GPIO_Pin_2
#define Set_Input_NEXT_DISP	     {GPIOA->OEN |= IO_A_NEXT_DISP; GPIOA->IE |= IO_A_NEXT_DISP; \
                                 GPIOA->ATT |= IO_A_NEXT_DISP;}
#define Set_Output_NEXT_DISP     {GPIOA->OEN &= ~IO_A_NEXT_DISP; GPIOA->IE &= ~IO_A_NEXT_DISP; \
                                 GPIOA->ATT &= ~IO_A_NEXT_DISP;}
#define IO_NEXT_DISP_H           {IO_DAT(PMU_IOADAT, GPIO_NUM_2) = 1;}
#define IO_NEXT_DISP_L           {IO_DAT(PMU_IOADAT, GPIO_NUM_2) = 0;}
#define IO_NEXT_DISP_Read        ((GPIOA->STS & IO_A_NEXT_DISP) >> GPIO_NUM_2)
/*-------------------------------------------------------------------------*/
#define IO_A_SO_CTRL             GPIO_Pin_3
#define Set_Input_SO_CTRL        {GPIOA->OEN |= IO_A_SO_CTRL; GPIOA->IE |= IO_A_SO_CTRL; \
                                  GPIOA->ATT |= IO_A_SO_CTRL;}
#define Set_Output_SO_CTRL       {GPIOA->OEN &= ~IO_A_SO_CTRL; GPIOA->IE &= ~IO_A_SO_CTRL; \
                                  GPIOA->ATT &= ~IO_A_SO_CTRL;}
#define IO_SO_CTRL_H             {IO_DAT(PMU_IOADAT, GPIO_NUM_3) = 1;}
#define IO_SO_CTRL_L             {IO_DAT(PMU_IOADAT, GPIO_NUM_3) = 0;}
#define IO_SO_CTRL_Read          ((GPIOA->STS & IO_A_SO_CTRL) >> GPIO_NUM_3)
/*-------------------------------------------------------------------------*/
#define IO_A_SEG12               GPIO_Pin_4
/*-------------------------------------------------------------------------*/
#define IO_A_SEG11               GPIO_Pin_5
/*-------------------------------------------------------------------------*/
#define IO_A_CF1                 GPIO_Pin_6
#define Set_Input_CF1            {GPIOA->OEN |= IO_A_CF1; GPIOA->IE |= IO_A_CF1; \
                                  GPIOA->ATT |= IO_A_CF1;}
/*-------------------------------------------------------------------------*/
#define IO_A_SEG10               GPIO_Pin_7
/*-------------------------------------------------------------------------*/
#define IO_A_Prevs_DISP          GPIO_Pin_8
#define Set_Input_Prevs_DISP     {GPIOA->OEN |= IO_A_Prevs_DISP; GPIOA->IE |= IO_A_Prevs_DISP; \
                                 GPIOA->ATT |= IO_A_Prevs_DISP;}
#define Set_Output_Prevs_DISP    {GPIOA->OEN &= ~IO_A_Prevs_DISP; GPIOA->IE &= ~IO_A_Prevs_DISP; \
                                 GPIOA->ATT &= ~IO_A_Prevs_DISP;}
#define IO_Prevs_DISP_H	         {IO_DAT(PMU_IOADAT, GPIO_NUM_8) = 1;}
#define IO_Prevs_DISP_L          {IO_DAT(PMU_IOADAT, GPIO_NUM_8) = 0;}
#define IO_Prevs_DISP_Read       ((GPIOA->STS & IO_A_Prevs_DISP) >> GPIO_NUM_8)
/*-------------------------------------------------------------------------*/
#define IO_A_IRQ0                GPIO_Pin_9
#define Set_Input_IRQ0           {GPIOA->OEN |= IO_A_IRQ0; GPIOA->IE |= IO_A_IRQ0; \
                                  GPIOA->ATT |= IO_A_IRQ0;}
#define IO_IRQ0_Read             ((GPIOA->STS & IO_A_IRQ0) >> GPIO_NUM_9)
/*-------------------------------------------------------------------------*/
#define IO_A_IRQ1                GPIO_Pin_11
#define Set_Input_IRQ1           {GPIOA->OEN |= IO_A_IRQ1; GPIOA->IE |= IO_A_IRQ1; \
                                  GPIOA->ATT |= IO_A_IRQ1;}
#define IO_IRQ1_Read             ((GPIOA->STS & IO_A_IRQ1) >> GPIO_NUM_11)
/*-------------------------------------------------------------------------*/
#define IO_A_SEG8                GPIO_Pin_12
/*-------------------------------------------------------------------------*/
#define IO_A_SEG6                GPIO_Pin_13
/*-------------------------------------------------------------------------*/
#define IO_A_OFF                 GPIO_Pin_14
#define Set_Input_OFF            {GPIOA->OEN |= IO_A_OFF; GPIOA->IE |= IO_A_OFF; \
                                  GPIOA->ATT |= IO_A_OFF;}
#define Set_Output_OFF           {GPIOA->OEN &= ~IO_A_OFF; GPIOA->IE &= ~IO_A_OFF; \
                                  GPIOA->ATT &= ~IO_A_OFF;}
#define IO_OFF_H                 {IO_DAT(PMU_IOADAT, GPIO_NUM_14) = 1;}
#define IO_OFF_L                 {IO_DAT(PMU_IOADAT, GPIO_NUM_14) = 0;}
#define IO_OFF_Read              ((GPIOA->STS & IO_A_OFF) >> GPIO_NUM_14)
/*-------------------------------------------------------------------------*/
#define IO_A_RXD0                GPIO_Pin_15
#define Set_Input_RXD0           {GPIOA->OEN |= IO_A_RXD0; GPIOA->IE |= IO_A_RXD0; \
                                  GPIOA->ATT |= IO_A_RXD0;}
/*-------------------------------------------------------------------------*/
/* GPIOB-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
#define IO_B_BT_TXD              GPIO_Pin_0
#define Set_Input_BT_TXD         {GPIOB->OEN |= IO_B_BT_TXD; GPIOB->IE |= IO_B_BT_TXD; \
                                  GPIOB->ATT |= IO_B_BT_TXD;}
#define Set_Output_BT_TXD        {GPIOB->OEN &= ~IO_B_BT_TXD; GPIOB->IE &= ~IO_B_BT_TXD; \
                                  GPIOB->ATT &= ~IO_B_BT_TXD;}
#define IO_BT_TXD_H              {IO_DAT(IOB_DAT, GPIO_NUM_0) = 1;}
#define IO_BT_TXD_L              {IO_DAT(IOB_DAT, GPIO_NUM_0) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_485RXD1             GPIO_Pin_1
#define Set_Input_485RXD1        {GPIOB->OEN |= IO_B_485RXD1; GPIOB->IE |= IO_B_485RXD1; \
                                  GPIOB->ATT |= IO_B_485RXD1;}
#define Set_Output_485RXD1       {GPIOB->OEN &= ~IO_B_485RXD1; GPIOB->IE &= ~IO_B_485RXD1; \
                                  GPIOB->ATT &= ~IO_B_485RXD1;}
#define IO_485RXD1_H             {IO_DAT(IOB_DAT, GPIO_NUM_1) = 1;}
#define IO_485RXD1_L             {IO_DAT(IOB_DAT, GPIO_NUM_1) = 0;}
#define IO_485RXD1_Read          ((GPIOB->STS & IO_B_485RXD1) >> GPIO_NUM_1)
/*-------------------------------------------------------------------------*/
#define IO_B_SEG9                GPIO_Pin_2
/*-------------------------------------------------------------------------*/
#define IO_B_SEG7                GPIO_Pin_3
/*-------------------------------------------------------------------------*/
#define IO_B_CF_CTRL             GPIO_Pin_4
#define Set_Input_CF_CTRL        {GPIOB->OEN |= IO_B_CF_CTRL; GPIOB->IE |= IO_B_CF_CTRL; \
                                  GPIOB->ATT |= IO_B_CF_CTRL;}
#define Set_Output_CF_CTRL       {GPIOB->OEN &= ~IO_B_CF_CTRL; GPIOB->IE &= ~IO_B_CF_CTRL; \
                                  GPIOB->ATT &= ~IO_B_CF_CTRL;}
#define IO_CF_CTRL_H             {IO_DAT(IOB_DAT, GPIO_NUM_4) = 1;}
#define IO_CF_CTRL_L             {IO_DAT(IOB_DAT, GPIO_NUM_4) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_TXD0                GPIO_Pin_5
#define Set_Output_TXD0          {GPIOB->OEN &= ~IO_B_TXD0; GPIOB->IE &= ~IO_B_TXD0; \
                                  GPIOB->ATT &= ~IO_B_TXD0;}
#define IO_TXD0_H                {IO_DAT(IOB_DAT, GPIO_NUM_5) = 1;}
#define IO_TXD0_L                {IO_DAT(IOB_DAT, GPIO_NUM_5) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_BT_RXD              GPIO_Pin_6
#define Set_Input_BT_RXD         {GPIOB->OEN |= IO_B_BT_RXD; GPIOB->IE |= IO_B_BT_RXD; \
                                  GPIOB->ATT |= IO_B_BT_RXD;}
#define Set_Output_BT_RXD        {GPIOB->OEN &= ~IO_B_BT_RXD; GPIOB->IE &= ~IO_B_BT_RXD; \
                                  GPIOB->ATT &= ~IO_B_BT_RXD;}
#define IO_BT_RXD_H              {IO_DAT(IOB_DAT, GPIO_NUM_6) = 1;}
#define IO_BT_RXD_L              {IO_DAT(IOB_DAT, GPIO_NUM_6) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_485TXD1             GPIO_Pin_7
#define Set_Output_485TXD1       {GPIOB->OEN &= ~IO_B_485TXD1; GPIOB->IE &= ~IO_B_485TXD1; \
                                  GPIOB->ATT &= ~IO_B_485TXD1;}
#define IO_485TXD1_H             {IO_DAT(IOB_DAT, GPIO_NUM_7) = 1;}
#define IO_485TXD1_L             {IO_DAT(IOB_DAT, GPIO_NUM_7) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_PWRKEY_CTRL         GPIO_Pin_8
#define Set_Output_PWRKEY_CTRL   {GPIOB->OEN &= ~IO_B_PWRKEY_CTRL; GPIOB->IE &= ~IO_B_PWRKEY_CTRL; \
                                  GPIOB->ATT &= ~IO_B_PWRKEY_CTRL;}
#define IO_PWRKEY_CTRL_H         {IO_DAT(IOB_DAT, GPIO_NUM_8) = 1;}
#define IO_PWRKEY_CTRL_L         {IO_DAT(IOB_DAT, GPIO_NUM_8) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_SPCS1               GPIO_Pin_9
#define Set_Input_SPCS1          {GPIOB->OEN |= IO_B_SPCS1; GPIOB->IE |= IO_B_SPCS1; \
                                  GPIOB->ATT |= IO_B_SPCS1;}
#define Set_Output_SPCS1         {GPIOB->OEN &= ~IO_B_SPCS1; GPIOB->IE &= ~IO_B_SPCS1; \
                                  GPIOB->ATT &= ~IO_B_SPCS1;}
#define IO_SPCS1_H               {IO_DAT(IOB_DAT, GPIO_NUM_9) = 1;}
#define IO_SPCS1_L               {IO_DAT(IOB_DAT, GPIO_NUM_9) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_SPCK1               GPIO_Pin_10
#define Set_Output_SPCK1         {GPIOB->OEN &= ~IO_B_SPCK1; GPIOB->IE &= ~IO_B_SPCK1; \
                                  GPIOB->ATT &= ~IO_B_SPCK1;}
#define IO_SPCK1_H               {IO_DAT(IOB_DAT, GPIO_NUM_10) = 1;}
#define IO_SPCK1_L               {IO_DAT(IOB_DAT, GPIO_NUM_10) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_SPDO1               GPIO_Pin_11
#define Set_Input_SPDO1          {GPIOB->OEN |= IO_B_SPDO1; GPIOB->IE |= IO_B_SPDO1; \
                                  GPIOB->ATT |= IO_B_SPDO1;}
#define Set_Output_SPDO1         {GPIOB->OEN &= ~IO_B_SPDO1; GPIOB->IE &= ~IO_B_SPDO1; \
                                  GPIOB->ATT &= ~IO_B_SPDO1;}
#define IO_SPDO1_H               {IO_DAT(IOB_DAT, GPIO_NUM_11) = 1;}
#define IO_SPDO1_L               {IO_DAT(IOB_DAT, GPIO_NUM_11) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_SPDI1               GPIO_Pin_12
#define Set_Input_SPDI1          {GPIOB->OEN |= IO_B_SPDI1; GPIOB->IE |= IO_B_SPDI1; \
                                  GPIOB->ATT |= IO_B_SPDI1;}
#define Set_Output_SPDI1         {GPIOB->OEN &= ~IO_B_SPDI1; GPIOB->IE &= ~IO_B_SPDI1; \
                                  GPIOB->ATT &= ~IO_B_SPDI1;}
#define IO_SPDI1_H               {IO_DAT(IOB_DAT, GPIO_NUM_12) = 1;}
#define IO_SPDI1_L               {IO_DAT(IOB_DAT, GPIO_NUM_12) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_NB_POWER_CTRL       GPIO_Pin_13
#define Set_Output_NB_POWER_CTRL {GPIOB->OEN &= ~IO_B_NB_POWER_CTRL; GPIOB->IE &= ~IO_B_NB_POWER_CTRL; \
                                  GPIOB->ATT &= ~IO_B_NB_POWER_CTRL;}
#define IO_NB_POWER_CTRL_H       {IO_DAT(IOB_DAT, GPIO_NUM_13) = 1;}
#define IO_NB_POWER_CTRL_L       {IO_DAT(IOB_DAT, GPIO_NUM_13) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_RESET_NB_CTRL       GPIO_Pin_14
#define Set_Output_RESET_NB_CTRL {GPIOB->OEN &= ~IO_B_RESET_NB_CTRL; GPIOB->IE &= ~IO_B_RESET_NB_CTRL; \
                                  GPIOB->ATT &= ~IO_B_RESET_NB_CTRL;}
#define IO_RESET_NB_CTRL_H       {IO_DAT(IOB_DAT, GPIO_NUM_14) = 1;}
#define IO_RESET_NB_CTRL_L       {IO_DAT(IOB_DAT, GPIO_NUM_14) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_B_BT_RST              GPIO_Pin_15
#define Set_Input_BT_RST         {GPIOB->OEN |= IO_B_BT_RST; GPIOB->IE |= IO_B_BT_RST; \
                                  GPIOB->ATT |= IO_B_BT_RST;}
#define Set_Output_BT_RST        {GPIOB->OEN &= ~IO_B_BT_RST; GPIOB->IE &= ~IO_B_BT_RST; \
                                  GPIOB->ATT &= ~IO_B_BT_RST;}
#define IO_BT_RST_H              {IO_DAT(IOB_DAT, GPIO_NUM_15) = 1;}
#define IO_BT_RST_L              {IO_DAT(IOB_DAT, GPIO_NUM_15) = 0;}
/*-------------------------------------------------------------------------*/
/* GPIOC-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
#define IO_C_DMACS1              GPIO_Pin_0
#define Set_Input_DMACS1         {GPIOC->OEN |= IO_C_DMACS1; GPIOC->IE |= IO_C_DMACS1; \
                                  GPIOC->ATT |= IO_C_DMACS1;}
#define Set_Output_DMACS1        {GPIOC->OEN &= ~IO_C_DMACS1; GPIOC->IE &= ~IO_C_DMACS1; \
                                  GPIOC->ATT &= ~IO_C_DMACS1;}
#define IO_DMACS1_H              {IO_DAT(IOC_DAT, GPIO_NUM_0) = 1;}
#define IO_DMACS1_L              {IO_DAT(IOC_DAT, GPIO_NUM_0) = 0;}
#define IO_DMACS1_Read           ((GPIOC->STS & IO_C_DMACS1) >> GPIO_NUM_0)
/*-------------------------------------------------------------------------*/
#define IO_C_DMACK1              GPIO_Pin_1
#define Set_Input_DMACK1         {GPIOC->OEN |= IO_C_DMACK1; GPIOC->IE |= IO_C_DMACK1; \
                                  GPIOC->ATT |= IO_C_DMACK1;}
#define Set_Output_DMACK1        {GPIOC->OEN &= ~IO_C_DMACK1; GPIOC->IE &= ~IO_C_DMACK1; \
                                  GPIOC->ATT &= ~IO_C_DMACK1;}
#define IO_DMACK1_H              {IO_DAT(IOC_DAT, GPIO_NUM_1) = 1;}
#define IO_DMACK1_L              {IO_DAT(IOC_DAT, GPIO_NUM_1) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_DMADO1              GPIO_Pin_3
#define Set_Input_DMADO1         {GPIOC->OEN |= IO_C_DMADO1; GPIOC->IE |= IO_C_DMADO1; \
                                  GPIOC->ATT |= IO_C_DMADO1;}
#define Set_Output_DMADO1        {GPIOC->OEN &= ~IO_C_DMADO1; GPIOC->IE &= ~IO_C_DMADO1; \
                                  GPIOC->ATT &= ~IO_C_DMADO1;}
#define IO_DMADO1_H              {IO_DAT(IOC_DAT, GPIO_NUM_3) = 1;}
#define IO_DMADO1_L              {IO_DAT(IOC_DAT, GPIO_NUM_3) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_SCL                 GPIO_Pin_4
#define Set_Input_SCL            {GPIOC->OEN |= IO_C_SCL; GPIOC->IE |= IO_C_SCL; \
                                  GPIOC->ATT |= IO_C_SCL;}
#define Set_Output_SCL           {GPIOC->OEN &= ~IO_C_SCL; GPIOC->IE &= ~IO_C_SCL; \
                                  GPIOC->ATT &= ~IO_C_SCL;}
#define IO_SCL_H                 {IO_DAT(IOC_DAT, GPIO_NUM_4) = 1;}
#define IO_SCL_L                 {IO_DAT(IOC_DAT, GPIO_NUM_4) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_SDA                 GPIO_Pin_5
#define Set_Input_SDA            {GPIOC->OEN |= IO_C_SDA; GPIOC->IE |= IO_C_SDA; \
                                  GPIOC->ATT |= IO_C_SDA;}
#define Set_Output_SDA           {GPIOC->OEN &= ~IO_C_SDA; GPIOC->IE &= ~IO_C_SDA; \
                                  GPIOC->ATT &= ~IO_C_SDA;}
#define IO_SDA_H                 {IO_DAT(IOC_DAT, GPIO_NUM_5) = 1;}
#define IO_SDA_L                 {IO_DAT(IOC_DAT, GPIO_NUM_5) = 0;}
#define IO_SDA_Read              ((GPIOC->STS & IO_C_SDA) >> GPIO_NUM_5)
/*-------------------------------------------------------------------------*/
#define IO_C_ON                  GPIO_Pin_6
#define Set_Output_ON            {GPIOC->OEN &= ~IO_C_ON; GPIOC->IE &= ~IO_C_ON; \
                                  GPIOC->ATT &= ~IO_C_ON;}
#define IO_ON_H                  {IO_DAT(IOC_DAT, GPIO_NUM_6) = 1;}
#define IO_ON_L                  {IO_DAT(IOC_DAT, GPIO_NUM_6) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_FLASH_CLK           GPIO_Pin_10
#define Set_Input_FLASH_CLK      {GPIOB->OEN |= IO_C_FLASH_CLK; GPIOB->IE |= IO_C_FLASH_CLK; \
                                 GPIOB->ATT |= IO_C_FLASH_CLK;}
#define Set_Output_FLASH_CLK     {GPIOB->OEN &= ~IO_C_FLASH_CLK; GPIOB->IE &= ~IO_C_FLASH_CLK; \
                                 GPIOB->ATT &= ~IO_C_FLASH_CLK;}
#define IO_FLASH_CLK_H           {IO_DAT(IOB_DAT, GPIO_NUM_10) = 1;}
#define IO_FLASH_CLK_L           {IO_DAT(IOB_DAT, GPIO_NUM_10) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_FLASH_CS            GPIO_Pin_9
#define Set_Input_FLASH_CS       {GPIOB->OEN |= IO_C_FLASH_CS; GPIOB->IE |= IO_C_FLASH_CS; \
                                  GPIOB->ATT |= IO_C_FLASH_CS;}
#define Set_Output_FLASH_CS      {GPIOB->OEN &= ~IO_C_FLASH_CS; GPIOB->IE &= ~IO_C_FLASH_CS; \
                                  GPIOB->ATT &= ~IO_C_FLASH_CS;}
#define IO_FLASH_CS_H            {IO_DAT(IOB_DAT, GPIO_NUM_9) = 1;}
#define IO_FLASH_CS_L            {IO_DAT(IOB_DAT, GPIO_NUM_9) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_FLASH_SDI           GPIO_Pin_12
#define Set_Input_FLASH_SDI      {GPIOB->OEN |= IO_C_FLASH_SDI; GPIOB->IE |= IO_C_FLASH_SDI; \
                                 GPIOB->ATT |= IO_C_FLASH_SDI;}
#define Set_Output_FLASH_SDI     {GPIOB->OEN &= ~IO_C_FLASH_SDI; GPIOB->IE &= ~IO_C_FLASH_SDI; \
                                 GPIOB->ATT &= ~IO_C_FLASH_SDI;}
#define IO_FLASH_SDI_H           {IO_DAT(IOB_DAT, GPIO_NUM_12) = 1;}
#define IO_FLASH_SDI_L           {IO_DAT(IOB_DAT, GPIO_NUM_12) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_C_FLASH_SDO           GPIO_Pin_11
#define Set_Input_FLASH_SDO      {GPIOB->OEN |= IO_C_FLASH_SDO; GPIOB->IE |= IO_C_FLASH_SDO; \
                                 GPIOB->ATT |= IO_C_FLASH_SDO;}
#define Set_Output_FLASH_SDO     {GPIOB->OEN &= ~IO_C_FLASH_SDO; GPIOB->IE &= ~IO_C_FLASH_SDO; \
                                 GPIOB->ATT &= ~IO_C_FLASH_SDO;}
#define IO_FLASH_SDO_H           {IO_DAT(IOB_DAT, GPIO_NUM_11) = 1;}
#define IO_FLASH_SDO_L           {IO_DAT(IOB_DAT, GPIO_NUM_11) = 0;}
#define IO_FLASH_SDO_Read        ((GPIOB->STS & IO_C_FLASH_SDO) >> GPIO_NUM_11)
/*-------------------------------------------------------------------------*/
#define IO_C_FLASH_WP            GPIO_Pin_11
#define Set_Input_FLASH_WP       {GPIOC->OEN |= IO_C_FLASH_WP; GPIOC->IE |= IO_C_FLASH_WP; \
                                  GPIOC->ATT |= IO_C_FLASH_WP;}
#define Set_Output_FLASH_WP      {GPIOC->OEN &= ~IO_C_FLASH_WP; GPIOC->IE &= ~IO_C_FLASH_WP; \
                                  GPIOC->ATT &= ~IO_C_FLASH_WP;}
#define IO_FLASH_WP_H            {IO_DAT(IOC_DAT, GPIO_NUM_11) = 1;}
#define IO_FLASH_WP_L            {IO_DAT(IOC_DAT, GPIO_NUM_11) = 0;}

#define IO_E_FLASH_PWR_CTRL         GPIO_Pin_8
#define Set_Input_FLASH_PWR_CTRL       {GPIOB->OEN |= IO_E_FLASH_PWR_CTRL; GPIOB->IE |= IO_E_FLASH_PWR_CTRL; \
                                  GPIOB->ATT |= IO_E_FLASH_PWR_CTRL;}
#define Set_Output_FLASH_PWR_CTRL      {GPIOB->OEN &= ~IO_E_FLASH_PWR_CTRL; GPIOB->IE &= ~IO_E_FLASH_PWR_CTRL; \
                                  GPIOB->ATT &= ~IO_E_FLASH_PWR_CTRL;}
#define IO_FLASH_PWR_CTRL_H            {IO_DAT(IOB_DAT, GPIO_NUM_8) = 1;}
#define IO_FLASH_PWR_CTRL_L            {IO_DAT(IOB_DAT, GPIO_NUM_8) = 0;}

/*-------------------------------------------------------------------------*/
/* GPIOD-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
#define IO_D_ALARM_LED           GPIO_Pin_13
#define Set_Output_ALARM_LED     {GPIOD->OEN &= ~IO_D_ALARM_LED; GPIOD->IE &= ~IO_D_ALARM_LED; \
                                  GPIOD->ATT &= ~IO_D_ALARM_LED;}
#define IO_ALARM_LED_H           {IO_DAT(IOD_DAT, GPIO_NUM_13) = 1;}
#define IO_ALARM_LED_L           {IO_DAT(IOD_DAT, GPIO_NUM_13) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_D_SLOUT               GPIO_Pin_14
#define Set_Output_SLOUT         {GPIOD->OEN &= ~IO_D_SLOUT; GPIOD->IE &= ~IO_D_SLOUT; \
                                  GPIOD->ATT &= ~IO_D_SLOUT;}
#define IO_SLOUT_H               {IO_DAT(IOD_DAT, GPIO_NUM_14) = 1;}
#define IO_SLOUT_L               {IO_DAT(IOD_DAT, GPIO_NUM_14) = 0;}
/*-------------------------------------------------------------------------*/
#define IO_D_SO_CTRL_CTL         GPIO_Pin_15
#define Set_Input_SO_CTRL_CTL    {GPIOD->OEN |= IO_D_SO_CTRL_CTL; GPIOD->IE |= IO_D_SO_CTRL_CTL; \
                                  GPIOD->ATT |= IO_D_SO_CTRL_CTL;}
#define Set_Output_SO_CTRL_CTL   {GPIOD->OEN &= ~IO_D_SO_CTRL_CTL; GPIOD->IE &= ~IO_D_SO_CTRL_CTL; \
                                  GPIOD->ATT &= ~IO_D_SO_CTRL_CTL;}
#define IO_SO_CTRL_CTL_H         {IO_DAT(IOD_DAT, GPIO_NUM_15) = 1;}
#define IO_SO_CTRL_CTL_L         {IO_DAT(IOD_DAT, GPIO_NUM_15) = 0;}
/*-------------------------------------------------------------------------*/
/* GPIOE-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
#define IO_E_VDCIN               GPIO_Pin_5
#define Set_Input_VDCIN          {GPIOE->OEN |= IO_E_VDCIN; GPIOE->IE |= IO_E_VDCIN; \
                                  GPIOE->ATT |= IO_E_VDCIN;}
/*-------------------------------------------------------------------------*/
#define IO_E_INPUT1              GPIO_Pin_6
#define Set_Input_INPUT1         {GPIOE->OEN |= IO_E_INPUT1; GPIOE->IE |= IO_E_INPUT1; \
                                  GPIOE->ATT |= IO_E_INPUT1;}
#define IO_INPUT1_Read           ((GPIOE->STS & IO_E_INPUT1) >> GPIO_NUM_6)
/*-------------------------------------------------------------------------*/
#define IO_E_INPUT2              GPIO_Pin_7
#define Set_Input_INPUT2         {GPIOE->OEN |= IO_E_INPUT1; GPIOE->IE |= IO_E_INPUT1; \
                                  GPIOE->ATT |= IO_E_INPUT1;}
#define IO_INPUT2_Read           ((GPIOE->STS & IO_E_INPUT2) >> GPIO_NUM_7)
/*-------------------------------------------------------------------------*/
#define IO_E_SEG16               GPIO_Pin_8
/*-------------------------------------------------------------------------*/
#define IO_E_AT_EN               GPIO_Pin_9
#define Set_Input_AT_EN          {GPIOE->OEN |= IO_E_AT_EN; GPIOE->IE |= IO_E_AT_EN; \
                                  GPIOE->ATT |= IO_E_AT_EN;}
#define Set_Output_AT_EN         {GPIOE->OEN &= ~IO_E_AT_EN; GPIOE->IE &= ~IO_E_AT_EN; \
                                  GPIOE->ATT &= ~IO_E_AT_EN;}
#define IO_AT_EN_H               {IO_DAT(IOE_DAT, GPIO_NUM_9) = 1;}
#define IO_AT_EN_L               {IO_DAT(IOE_DAT, GPIO_NUM_9) = 0;}
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

//#endif


/* End --------------------------------------------------------------------------*/