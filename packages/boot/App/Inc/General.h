/************************************************************************
* 版权所有 (C)2010, 万高。
*
* 文件名称： // General.H
* 文件标识： // 常用数据格式转换程序文件头
* 内容摘要： // 
* 其它说明： //
* 当前版本： // V01.00
* 作 者： // 万高
* 完成日期： // 2010年7月06日
*
* 修改记录1：//
* 修改日期：
* 版 本 号：
* 修 改 人：
* 修改内容：
************************************************************************/

#ifdef	General_H
#define	General_EXT
#else
#define	General_EXT extern
#endif


#include "appdefine.h"
extern unsigned char BYTE_BCD( unsigned char HEX );
extern unsigned char BCD_BYTE( unsigned char BCD );
extern void WORD_BCD2( unsigned char *BCD, unsigned short WORD );
extern unsigned int BCD2_WORD( unsigned char* BCD );
extern void HEX4_BCD4( unsigned char *BCD, unsigned long HEX );
extern unsigned long BCD4_HEX4( unsigned char* BCD );
extern void BCDX_HEX( unsigned char* BCD, unsigned char Len );
extern unsigned char BCD_Check( unsigned char* Addr, unsigned int Len );
extern void BCDX_INC( unsigned char *BCD, unsigned char Len );
extern void BCD4_INC( unsigned char *BCD );
extern void BCD4_DEC( unsigned char *BCD );
extern void HBF_LOOP_LBF( unsigned char *Dest, unsigned char *Src, unsigned char Len );
extern unsigned long MaxCpy( unsigned short *Num0, unsigned short *Num1, unsigned short *Num2);
extern unsigned long MinCpy( unsigned short *Num0, unsigned short *Num1, unsigned short *Num2);
extern unsigned long Average( unsigned long  Num0, unsigned long Num1, unsigned long Num2 );
extern uint16_t CalcChksum(uint8_t *pucBuff, uint16_t usLen);
extern char *c_strstr(uint8_t *src,uint8_t *substr);
extern void Api_StringToChar(uint8_t* num, char* str, uint16_t len);
extern uint8_t c_strlen(uint8_t *str);
extern uint16_t do_CRC(uint8_t *str, uint16_t len, uint16_t Init);
extern void ApiReverseData(uint8_t* buff, uint8_t Len);
extern uint8_t DoSum(uint8_t* pucBuf, uint16_t uiLen);
extern uint16_t Uint8_To_Uint16(uint8_t * buff);
extern void MemInvertCpy(uint8_t *des,uint8_t *src,uint8_t len);
extern uint32_t Uint8_To_Uint32(uint8_t * buff);
extern uint16_t Api_CharToStringPoint(char* str, uint8_t *pbuf, uint16_t ucBytelen, uint8_t ucAtb);
extern void Do33(uint8_t *pucBuf, uint8_t ucLen);