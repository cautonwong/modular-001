/*****************************************************************************
//文 件 名：Appdefine.h
//功    能：公用模块头文件
//版本说明：
//起草时间: 2021-04-30
//修改时间：
//作    者：liurf
*****************************************************************************/
#ifndef	__APPDEFINE_H__
#define __APPDEFINE_H__

//定义常量, 常数
#include "stdint.h"
#include "stdio.h"		//打印
#include "stdlib.h"		//malloc
#include "string.h"		//memcpy、memset等

typedef unsigned char    u8;      // 无符号8位整型变量
typedef unsigned char    U8;      // 无符号8位整型变量
typedef signed   char    s8;       // 有符号8位整型变量
typedef signed   char    S8;       // 有符号8位整型变量
typedef unsigned short   u16;     // 无符号16位整型变量
typedef unsigned short   U16;     // 无符号16位整型变量
typedef signed   short   s16;      // 有符号16位整型变量
typedef signed   short   S16;      // 有符号16位整型变量
typedef unsigned long int     u32;     // 无符号32位整型变量
typedef unsigned long int     U32;     // 无符号32位整型变量
typedef signed   long int     s32;      // 有符号32位整型变量
typedef signed   long int     S32;      // 有符号32位整型变量
typedef unsigned long long     u64;     // 无符号32位整型变量
typedef unsigned long long     U64;     // 无符号32位整型变量
typedef signed   long long     s64;      // 有符号32位整型变量
typedef signed   long long     S64;      // 有符号32位整型变量

#define BIT bit 

#ifdef	REG_BIG_MODE
#define	uiuc0	uiuc[1]					///数据绝对的低8位，大端模式就是高地址，小端模式就是低地址
#define	uiuc1	uiuc[0]
#define	uic0	uic[1]
#define	uic1	uic[0]
#else
#define	uiuc0	uiuc[0]					///数据绝对的低8位，大端模式就是高地址，小端模式就是低地址
#define	uiuc1	uiuc[1]
#define	uic0	uic[0]
#define	uic1	uic[1]
#endif

typedef union                            //16位共用体
{
	u16 ui;
	s16 i;
	u8 uiuc[2];
	s8 uic[2];
}UNINT16;

#ifdef	REG_BIG_MODE
#define	uluc0	uluc[3]
#define	uluc1	uluc[2]
#define	uluc2	uluc[1]
#define	uluc3	uluc[0]
#define	ulc0	ulc[3]
#define	ulc1	ulc[2]
#define	ulc2	ulc[1]
#define	ulc3	ulc[0]
#define	ului0	ului[1]
#define	ului1	ului[0]
#define	uli0	uli[1]
#define	uli1	uli[0]
#else
#define	uluc0	uluc[0]
#define	uluc1	uluc[1]
#define	uluc2	uluc[2]
#define	uluc3	uluc[3]
#define	ulc0	ulc[0]
#define	ulc1	ulc[1]
#define	ulc2	ulc[2]
#define	ulc3	ulc[3]
#define	ului0	ului[0]
#define	ului1	ului[1]
#define	uli0	uli[0]
#define	uli1	uli[1]
#endif
typedef union                            //32位共用体
{
	u32   ul;
	s32	  l;
	u16   ului[2];
	s16   uli[2];
	u8    uluc[4];
	s8    ulc[4];
}UNINT32;


#define	uduc0	uduc[0]
#define	uduc1	uduc[1]
#define	uduc2	uduc[2]
#define	uduc3	uduc[3]
#define	uduc4	uduc[4]
#define	uduc5	uduc[5]
#define	uduc6	uduc[6]
#define	uduc7	uduc[7]
#define	udc0	udc[0]
#define	udc1	uduc[1]
#define	udc2	uduc[2]
#define	udc3	uduc[3]
#define	udc4	uduc[4]
#define	udc5	uduc[5]
#define	udc6	uduc[6]
#define	udc7	uduc[7]
#define	udui0	udui[0]
#define	udui1	udui[1]
#define	udui2	udui[2]
#define	udui3	udui[3]
#define	udi0	udi[0]
#define	udi1	udi[1]
#define	udi2	udi[2]
#define	udi3	udi[3]
#define	udul0	udul[0]
#define	udul1	udul[1]
#define	udl0	udl[0]
#define	udl1	udl[1]
typedef union                            //32位共用体
{
	u64	ud;
	s64	d;
	u32	udul[2];
	s32	udl[2];
	u16	udui[4];
	s16	udi[4];
	u8	uduc[8];
	s8	udc[8];
}UN64;
#define UN16 UNINT16
#define UN32 UNINT32
#define bit u8
#define bit16 u16
typedef union
{
  u8 u08;
  struct
  {
    u8 bit0:1;
    u8 bit1:1;
    u8 bit2:1;
    u8 bit3:1;
    u8 bit4:1;
    u8 bit5:1;
    u8 bit6:1;
    u8 bit7:1;
  }Bit;
}s8_status;
typedef union
{
  u16 u016;
  struct
  {
    u16 bit0:1;
    u16 bit1:1;
    u16 bit2:1;
    u16 bit3:1;
    u16 bit4:1;
    u16 bit5:1;
    u16 bit6:1;
    u16 bit7:1;
	u16 bit8:1;
	u16 bit9:1;
	u16 bit10:1;
	u16 bit11:1;
	u16 bit12:1;
	u16 bit13:1;
	u16 bit14:1;
	u16 bit15:1;
  }Bit16;
}s16_status;
#endif

