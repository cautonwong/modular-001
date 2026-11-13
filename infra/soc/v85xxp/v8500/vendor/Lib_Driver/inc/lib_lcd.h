/**
  ******************************************************************************
  * @file    lib_lcd.h 
  * @author  Application Team
  * @version V2.0.0
  * @date    2020-07-06
  * @brief   LCD library.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
#ifndef __LIB_LCD_H
#define __LIB_LCD_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "target.h"

/* LCD COMx IO typedef */
typedef struct 
{
  __IO uint32_t    *GPIO;
       uint16_t     Pin;
}LCD_COMIO;

typedef struct 
{
  uint32_t Type;
  uint32_t Drv;
  uint32_t FRQ;
  uint32_t SWPR;
  uint32_t FBMODE;
  uint32_t BKFILL;
} LCD_InitType;

typedef struct
{
  uint32_t SegCtrl0;
  uint32_t SegCtrl1;
  uint16_t SegCtrl2;
  uint32_t COMMode;
} LCD_IOInitType;

/**************  Bits definition for ANA_REG6 register       ******************/
#define ANA_REG6_VLCD_0               (0x0U << ANA_REG6_VLCD_Pos) 
#define ANA_REG6_VLCD_1               (0x1U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_2               (0x2U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_3               (0x3U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_4               (0x4U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_5               (0x5U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_6               (0x6U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_7               (0x7U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_8               (0x8U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_9               (0x9U << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_A               (0xAU << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_B               (0xBU << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_C               (0xCU << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_D               (0xDU << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_E               (0xEU << ANA_REG6_VLCD_Pos)
#define ANA_REG6_VLCD_F               (0xFU << ANA_REG6_VLCD_Pos)

/**************  Bits definition for LCD_CTRL register       ******************/

/**************  Bits definition for LCD_CTRL2 register      ******************/

//Type
#define LCD_TYPE_4COM          (0x0U << LCD_CTRL_TYPE_Pos)              /*!< 0x00000000 */
#define LCD_TYPE_6COM          (0x1U << LCD_CTRL_TYPE_Pos)              /*!< 0x00000010 */
#define LCD_TYPE_8COM          (0x2U << LCD_CTRL_TYPE_Pos)              /*!< 0x00000020 */
#define IS_LCD_TYPE(__TYPE__)  (((__TYPE__) == LCD_TYPE_4COM) ||\
                                ((__TYPE__) == LCD_TYPE_6COM) ||\
                                ((__TYPE__) == LCD_TYPE_8COM))

//DrivingRes
#define LCD_DRV_300            (0x0U << LCD_CTRL_DRV_Pos)               /*!< 0x00000000 */
#define LCD_DRV_600            (0x1U << LCD_CTRL_DRV_Pos)               /*!< 0x00000004 */
#define LCD_DRV_150            (0x2U << LCD_CTRL_DRV_Pos)               /*!< 0x00000008 */
#define LCD_DRV_200            (0x3U << LCD_CTRL_DRV_Pos)               /*!< 0x0000000C */
#define IS_LCD_DRV(__DRV__)    (((__DRV__) == LCD_DRV_300) ||\
                                ((__DRV__) == LCD_DRV_600) ||\
                                ((__DRV__) == LCD_DRV_150) ||\
                                ((__DRV__) == LCD_DRV_200)) 

//ScanFRQ
#define LCD_FRQ_64H            (0x0U << LCD_CTRL_FRQ_Pos)               /*!< 0x00000000 */
#define LCD_FRQ_128H           (0x1U << LCD_CTRL_FRQ_Pos)               /*!< 0x00000001 */
#define LCD_FRQ_256H           (0x2U << LCD_CTRL_FRQ_Pos)               /*!< 0x00000002 */
#define LCD_FRQ_512H           (0x3U << LCD_CTRL_FRQ_Pos)               /*!< 0x00000003 */
#define IS_LCD_FRQ(__FRQ__)    (((__FRQ__) == LCD_FRQ_64H)  ||\
                                ((__FRQ__) == LCD_FRQ_128H) ||\
                                ((__FRQ__) == LCD_FRQ_256H) ||\
                                ((__FRQ__) == LCD_FRQ_512H))

#define IS_LCD_SWPR(__SWPR__)  ((__SWPR__) <= 0xFFUL)   

//SwitchMode
#define LCD_FBMODE_BUFA            (0x0U << LCD_CTRL2_FBMODE_Pos)           /*!< 0x00000000 */
#define LCD_FBMODE_BUFAB           (0x1U << LCD_CTRL2_FBMODE_Pos)           /*!< 0x00000040 */
#define LCD_FBMODE_BUFABLANK       (0x2U << LCD_CTRL2_FBMODE_Pos)           /*!< 0x00000080 */
#define IS_LCD_FBMODE(__FBMODE__)  (((__FBMODE__) == LCD_FBMODE_BUFA)  ||\
                                    ((__FBMODE__) == LCD_FBMODE_BUFAB) ||\
                                    ((__FBMODE__) == LCD_FBMODE_BUFABLANK))                                   

//BlankFill
#define LCD_BKFILL_1       LCD_CTRL2_BKFILL
#define LCD_BKFILL_0       0 
#define IS_LCD_BKFILL(__BKFILL__)  (((__BKFILL__) == LCD_BKFILL_1) || ((__BKFILL__) == LCD_BKFILL_0))

//BiasSelection
#define LCD_BMODE_DIV3           0
#define LCD_BMODE_DIV4           ANA_REG6_LCD_BMODE
#define IS_LCD_BMODE(__BMODE__)  (((__BMODE__) == LCD_BMODE_DIV3) ||\
                                  ((__BMODE__) == LCD_BMODE_DIV4))
                                  
//VLCDSelection
#define LCD_VOL_DEFAULT       (0UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_INC60MV       (1UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_INC120MV      (2UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_INC180MV      (3UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_INC240MV      (4UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_INC300MV      (5UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC60MV       (6UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC120MV      (7UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC180MV      (8UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC240MV      (9UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC300MV      (10UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC360MV      (11UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC420MV      (12UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC480MV      (13UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC540MV      (14UL << ANA_REG6_VLCD_Pos)
#define LCD_VOL_DEC600MV      (15UL << ANA_REG6_VLCD_Pos)
#define IS_LCD_VOL(__VOL__)  (((__VOL__) == LCD_VOL_DEFAULT)  ||\
                              ((__VOL__) == LCD_VOL_INC60MV)  ||\
                              ((__VOL__) == LCD_VOL_INC120MV) ||\
                              ((__VOL__) == LCD_VOL_INC180MV) ||\
                              ((__VOL__) == LCD_VOL_INC240MV) ||\
                              ((__VOL__) == LCD_VOL_INC300MV) ||\
                              ((__VOL__) == LCD_VOL_DEC60MV)  ||\
                              ((__VOL__) == LCD_VOL_DEC120MV) ||\
                              ((__VOL__) == LCD_VOL_DEC180MV) ||\
                              ((__VOL__) == LCD_VOL_DEC240MV) ||\
                              ((__VOL__) == LCD_VOL_DEC300MV) ||\
                              ((__VOL__) == LCD_VOL_DEC360MV) ||\
                              ((__VOL__) == LCD_VOL_DEC420MV) ||\
                              ((__VOL__) == LCD_VOL_DEC480MV) ||\
                              ((__VOL__) == LCD_VOL_DEC540MV) ||\
                              ((__VOL__) == LCD_VOL_DEC600MV))


/****************************** LCD Instances *********************************/
#define IS_LCD_ALL_INSTANCE(INSTANCE) ((INSTANCE) == LCD)

/* Exported Functions ------------------------------------------------------- */
/* LCD Exported Functions Group1: 
                                  (De)Initialization -------------------------*/
void LCD_DeInit(void);
void LCD_StructInit(LCD_InitType *LCD_InitStruct);
void LCD_Init(LCD_InitType *InitStruct);
/* LCD Exported Functions Group1: 
                                  MISC Configuration -------------------------*/
void LCD_Cmd(LCD_IOInitType *IOInitType, uint32_t NewState);
void LCD_BiasModeConfig(uint32_t BiasSelection);
void LCD_VoltageConfig(uint32_t VLCDSelection);

#ifdef __cplusplus
}
#endif
     
#endif /* __LIB_LCD_H */

/*********************************** END OF FILE ******************************/

