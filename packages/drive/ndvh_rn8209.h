
#ifndef MEA_RN8209_H_
#define MEA_RN8209_H_

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


#define  PIN_RN8209_RX_OUT_HIGH()   {;}  //根据宏定义配置管脚
#define  PIN_RN8209_RX_OUT_LOW()    {;}  //根据宏定义配置管脚
#define  PIN_RN8209_RX_IN_MODE()    {;}  //根据宏定义配置管脚



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






//提供给主程序调用的函数申明
extern void ResetRn8209(void);                  //RN8209芯片硬件复位
extern unsigned char Rn8209cCommInit(void);     //RN8209芯片串口通讯配置
extern unsigned char Rn8209AdjParaSet(struct Rn8209AdjSet_t* Set , struct dev_info_t* dev); //RN8209芯片校表参数设置
extern unsigned char Rn8209DeviceIDCheck();     //RN8209芯片ID检查
extern unsigned char Rn8209InitConfig();        //RN8209芯片基本参数设置校表
extern void MeasureValue(struct dev_info_t* dev);//主程序调用实时数据计算

#endif


