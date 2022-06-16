
#ifndef MEA_RN8209_H_
#define MEA_RN8209_H_

#include "target.h"
#include "lib_gpio.h"

#define	SYSCON		0x00
#define	EMUCON		0x01
#define	HFCONST		0x02
#define	PSTART		0x03
#define	DSTART		0x04
#define	GPQA		0x05
#define	GPQB		0x06
#define	PHSA		0x07
#define	PHSB		0x08
#define	APOSA		0x0A
#define	APOSB		0x0B
#define	IARMSOS		0x0E
#define	IBRMSOS		0x0F
#define	IBGAIN		0x10
#define	EMUCON2		0x17
#define	PFCNT		0x20
#define	IARMS		0x22
#define	IBRMS		0x23
#define	URMS		0x24
#define	UFREQ		0x25
#define	POWERPA		0x26
#define	POWERPB		0x27
#define POWERQA         0x28
#define	ENERGYP		0x29
#define	EMUSTATUS	0x2D
#define	IE		0x40
#define	SYSSTATUS	0x43
#define	DEVICEID	0x7F

#define	CMD_WR_EN		0xE5
#define	CMD_WR_DIS		0xDC
#define	CMD_RESET		0xFA


#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif


/*--------------------------单相RN8209计量芯片控制-------------------------------*/ 
#define  BITBAND_ADDR(addr, bitnum)   ((((uint32_t)addr) & 0xF0000000) + \
                                        0x2000000 + ((((uint32_t)addr) & 0xFFFFF) << 5) + (bitnum << 2))

#define  PIN_RN8209_RX_OUT_MODE()   {*((uint32_t*)GPIOA_OEN) &= ~BIT(15); *((uint32_t*)GPIOA_IE) &= ~BIT(15);}
#define  PIN_RN8209_RX_IN_MODE()    {*((uint32_t*)GPIOA_IE) |= BIT(15);  *((uint32_t*)GPIOA_OEN) |= BIT(15);}
#define  PIN_RN8209_RX_OUT_HIGH()   {*((uint32_t*)BITBAND_ADDR(GPIOA_DAT,15)) = 1;}
#define  PIN_RN8209_RX_OUT_LOW()    {*((uint32_t*)BITBAND_ADDR(GPIOA_DAT,15)) = 0;}
#define  PIN_RN8209_TX_OUT_MODE()   {*((uint32_t*)GPIOB_OEN) &= ~BIT(5); *((uint32_t*)GPIOB_IE) &= ~BIT(5);}
#define  PIN_RN8209_TX_OUT_HIGH()   {*((uint32_t*)BITBAND_ADDR(GPIOB_DAT,5)) = 1;}



#define  CAL_U    1.00
#define  CAL_I    1.00
#define  CAL_P    1.00


struct Rn8209AdjSet_t
{
    unsigned short Gpq;
    unsigned short Apos;
    unsigned short Irmsos;
    unsigned char  Phs;
};

struct McuUartConfig_t
{
    unsigned short Baud;
    unsigned char DataWidth;
    unsigned char Parity;
    unsigned char Stop;
};



struct CalMesaureValue_t
{
    signed long Voltage;
    signed long Current;
    signed long ActivePower;
    signed long ReactivePower;
    signed long FreQ;
};


struct factor_adj_meter_t
{
    unsigned char adjen;	//Ð£±íÊ¹ÄÜ	0xAA
    unsigned char adjsec;	//Ð£±íµ¹¼ÆÊ± 2ÃëºóÐ£±í
    unsigned char adjflag;	//Ð£±í±êÖ¾Î»
    unsigned long   Ua;
    unsigned long   Ia;
    signed   long   Pa;
    signed   long   Paw;
    unsigned long   Ub;
    unsigned long   Ib;
    signed   long   Pb;
    signed   long   Pbw;
    unsigned long   Uc;
    unsigned long   Ic;
    signed   long   Pc;
    signed   long   Pcw;
    unsigned long   Ian;
    unsigned char   null[2];
};

#define F_ALadj   0x01
#define F_ANadj   0x10

//æä¾›ç»™ä¸»ç¨‹åºè°ƒç”¨çš„å‡½æ•°ç”³æ˜Ž
extern void ResetRn8209(void);                  //RN8209èŠ¯ç‰‡ç¡¬ä»¶å¤ä½
extern unsigned char Rn8209cCommInit(void);     //RN8209èŠ¯ç‰‡ä¸²å£é€šè®¯é…ç½®
extern unsigned char Rn8209AdjParaSet(struct Rn8209AdjSet_t* Set , const struct dev_info_t* dev); //RN8209èŠ¯ç‰‡æ ¡è¡¨å‚æ•°è®¾ç½®
extern unsigned char Rn8209DeviceIDCheck();     //RN8209èŠ¯ç‰‡IDæ£€æŸ¥
extern unsigned char Rn8209InitConfig();        //RN8209èŠ¯ç‰‡åŸºæœ¬å‚æ•°è®¾ç½®æ ¡è¡¨
extern void MeasureValue(struct dev_info_t* dev);//ä¸»ç¨‹åºè°ƒç”¨å®žæ—¶æ•°æ®è®¡ç®—
extern unsigned char CmdRn8209(unsigned char cmd  , const struct dev_info_t* dev);

#endif