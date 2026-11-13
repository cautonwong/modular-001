/**
  ******************************************************************************
  * @file    type_def.h 
  * @author  Application Team
  * @version V4.4.0
  * @date    2018-09-27
  * @brief   Typedef file
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

#ifndef __TYPE_DEF_H
#define __TYPE_DEF_H



#define NO_BITS    0x00
#define BIT0       0x01
#define BIT1       0x02
#define BIT2       0x04
#define BIT3       0x08
#define BIT4       0x10
#define BIT5       0x20
#define BIT6       0x40
#define BIT7       0x80
#define BIT8       0x0100
#define BIT9       0x0200
#define BIT10      0x0400
#define BIT11      0x0800
#define BIT12      0x1000
#define BIT13      0x2000
#define BIT14      0x4000
#define BIT15      0x8000
#define BIT16      0x010000L
#define BIT17      0x020000L
#define BIT18      0x040000L
#define BIT19      0x080000L
#define BIT20      0x100000L
#define BIT21      0x200000L
#define BIT22      0x400000L
#define BIT23      0x800000L
#define BIT24      0x01000000L
#define BIT25      0x02000000L
#define BIT26      0x04000000L
#define BIT27      0x08000000L
#define BIT28      0x10000000L
#define BIT29      0x20000000L
#define BIT30      0x40000000L
#define BIT31      0x80000000L




#define ENABLE  1
#define DISABLE 0
#define IS_FUNCTIONAL_STATE(__STATE__) (((__STATE__) == DISABLE) || ((__STATE__) == ENABLE))

#if  defined ( __GNUC__ )
  #ifndef __weak
    #define __weak   __attribute__((weak))
  #endif /* __weak */
  #ifndef __packed
    #define __packed __attribute__((__packed__))
  #endif /* __packed */
#endif /* __GNUC__ */


/* Macro to get variable aligned on 4-bytes, for __ICCARM__ the directive "#pragma data_alignment=4" must be used instead */
#if defined   (__GNUC__)        /* GNU Compiler */
  #ifndef __ALIGN_END
    #define __ALIGN_END    __attribute__ ((aligned (4)))
  #endif /* __ALIGN_END */
  #ifndef __ALIGN_BEGIN
    #define __ALIGN_BEGIN
  #endif /* __ALIGN_BEGIN */
#else
  #ifndef __ALIGN_END
    #define __ALIGN_END
  #endif /* __ALIGN_END */
  #ifndef __ALIGN_BEGIN
    #if defined   (__CC_ARM)      /* ARM Compiler */
      #define __ALIGN_BEGIN    __align(4)
    #elif defined (__ICCARM__)    /* IAR Compiler */
      #define __ALIGN_BEGIN
    #endif /* __CC_ARM */
  #endif /* __ALIGN_BEGIN */
#endif /* __GNUC__ */

/**
  * @brief  __NOINLINE definition
  */
#if defined ( __CC_ARM   ) || defined   (  __GNUC__  )
/* ARM & GNUCompiler
   ----------------
*/
#define __NOINLINE __attribute__ ( (noinline) )

#elif defined ( __ICCARM__ )
/* ICCARM Compiler
   ---------------
*/
#define __NOINLINE _Pragma("optimize = no_inline")

#endif

#endif /* __TYPE_DEF_H */

/*********************************** END OF FILE ******************************/
