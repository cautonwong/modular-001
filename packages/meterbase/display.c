/**********************************************************************/
/*  液晶显示程序                                                       */
/***********************************************************************/
#include "includes.h"

/***********************************************************************/
/*  液晶显示段码定义                                                   */
/***********************************************************************/

/***********************************************************************/
/*          0                a                                         */
/*        -----            -----                                       */
/*       |     |          |     |                                      */
/*      4|     |1        f|     |b                                     */
/*       |  5  |          |  g  |                                      */
/*        -----            -----                                       */
/*       |     |          |     |                                      */
/*      6|     |2        e|     |c                                     */
/*       |  3  |          |  d  |                                      */
/*        -----  *7       -----  *p                                    */
/***********************************************************************/
//CON0--CON7
#define 	DS_Chr_0     			0x5F
#define 	DS_Chr_1     			0x06
#define 	DS_Chr_2     			0x6B
#define 	DS_Chr_3     			0x2F
#define 	DS_Chr_4    			0x36
#define 	DS_Chr_5     			0x3D
#define 	DS_Chr_6     			0x7D
#define 	DS_Chr_7     			0x07
#define 	DS_Chr_8     			0x7F
#define 	DS_Chr_9     			0x3F
#define 	DS_Chr_A     			0x77
#define 	DS_Chr_B     			0x7C
#define 	DS_Chr_C     			0x59
#define 	DS_Chr_D     			0x6E
#define 	DS_Chr_E     			0x79
#define 	DS_Chr_F     			0x71
#define 	DS_Chr_t     			0x78
#define 	DS_Chr_S     			0x3D

#define		DS_Chr__				0x20
#define		DS_Chr_g				0x3F
#define		DS_Chr_o				0x6C
#define		DS_Chr_d				0x6E
#define		DS_Chr_I				0x06
//#define		DS_Chr_C				0x59
#define		DS_Chr_r				0x60
#define		DS_Chr_U				0x5E
#define		DS_Chr_P				0x73
#define		DS_Chr_H				0x76
#define		DS_Chr_L				0x58
#define		DS_Chr_n				0x64
#define		DS_Chr_J				0x0E
//#define		DS_Chr_S				0x3D
	
/*************** CON14 ***************/
#define     DS_Chr_COS              0x80
#define     DS_Chr_P0               0x40
#define     DS_Chr_KeyProgram       0x20
#define     DS_Chr_Shang            0x10
#define     DS_Chr_Lock             0x08
#define     DS_Chr_Zheng            0x04
#define     DS_Chr_CommPLC          0x02
#define     DS_Chr_Fan              0x01

/*************** CON15 ***************/
#define     DS_Chr_Comm				0x80
#define     DS_Chr_Shi				0x40   //时
#define     DS_Chr_Xiang       		0x20 	//向
#define     DS_Chr_Jian            	0x10  //间
#define     DS_Chr_Wu             	0x08  //无
#define     DS_Chr_Dot4				0x04
#define     DS_Chr_Q1				0x02 //I象限
#define     DS_Chr_Yong				0x01   //用

/*************** CON16 ***************/
#define     DS_Chr_Q4              0x80 //V象限
#define     DS_Chr_Zong               0x40
#define     DS_Chr_Kvar       0x20
#define     DS_Chr_T            0x10    //费率T
#define     DS_Chr_P2             0x08
#define     DS_Chr_TouTable2            0x04
#define     DS_Chr_TouTable1         0x02
#define     DS_Chr_Dot1              0x01

/*************** CON17 ***************/
#define     DS_Chr_UV              0x80
#define     DS_Chr_IA               0x40
#define     DS_Chr_Dot2       0x20
#define     DS_Chr_ChangShu            0x10
#define     DS_Chr_Dot22             0x08//时钟小数点
#define     DS_Chr_kW            0x04
#define     DS_Chr_L2          0x02//阶梯L
#define     DS_Chr_Hao              0x01

/*************** CON18 ***************/
#define     DS_Chr_h              0x80  //kwh-h
#define     DS_Chr_StepTable2               0x40
#define     DS_Chr_Dot3       0x20
#define     DS_Chr_Hu            0x10
#define     DS_Chr_h2             0x08  //kvarh-h
#define     DS_Chr_StepTable1            0x04
#define     DS_Chr_Dot42          0x02  //时钟小数点
#define     DS_Chr_Q2              0x01 //2象限

/*************** CON19 ***************/
#define     DS_Chr_DianLiang              0x80  
#define     DS_Chr_LaZha               0x40
#define     DS_Chr_Biao       0x20
#define     DS_Chr_Gong            0x10 //工
#define     DS_Chr_Lab             0x08  
#define     DS_Chr_GouDian            0x04  //请购电
#define     DS_Chr_Duan          0x02  //段
#define     DS_Chr_You              0x01 //有

/*************** CON20 ***************/
#define     DS_Chr_P1               0x80 //费率10
#define     DS_Chr_FeiLv            0x40 //费率
#define     DS_Chr_ZuHe             0x20
#define     DS_Chr_JieTi            0x10
#define     DS_Chr_Yue              0x08
#define     DS_Chr_Bat              0x04
#define     DS_Chr_DangQian         0x02

/*************** CON21 ***************/
#define     DS_Chr_Gu               0x80
#define     DS_Chr_Feng             0x40
#define     DS_Chr_Reverse          0x20
#define     DS_Chr_Nline            0x10
#define     DS_Chr_Fu               0x08
#define     DS_Chr_Lline            0x04
#define     DS_Chr_Ping             0x02
#define     DS_Chr_Jian2            0x01  //费率尖2

//Info16bit定义
// 6 bits	数据长度
#define	    DispDataLen				0x003F
// 1 bits   数据正负标志
#define		DispData_Sign			0x2000

//显示内容定义
#define		Sign_Null				0
#define		Sign_1					1//1
#define		Sign_2					2//2
#define		Sign_3					3//3
#define		Sign_4					4//4
#define		Sign_5					5//5
#define		Sign_6					6//6
#define		Sign_7					7//7
#define		Sign_8					8//8
#define		Sign_9					9//9
#define		Sign_10					10//10
#define		Sign_11					11//11
#define		Sign_12					12//12
#define		Sign_13					13//13
#define		Sign_14					14//14
#define		Sign_15					15//15
#define		Sign_16					16//16
#define		Sign_17					17//17
#define		Sign_18					18//18


#define		Sign_kWh				19//kwh
#define		Sign_kW					20//kw
#define		Sign_yuan				21//元
#define		Sign_dot1				22//1位小数点
#define		Sign_dot2				23//2位小数点
#define		Sign_dot3				24//3位小数点
#define		Sign_dot4				25//4位小数点
#define		Sign_time				26//： ：
#define		Sign_date				27//- -
#define		Sign_dangqian			28//当前
#define		Sign_yue				29
#define		Sign_shangyue			30//上月
#define		Sign_huhao				31//户号

#define		Sign_FeiLvNum			32//费率号
#define		Sign_YouGong			33//有功
#define		Sign_WuGong				34//无功
#define		Sign_shengyu			37//剩余
#define		Sign_changshu		    38//常数
#define		Sign_jieti				39//阶梯
#define		Sign_TouZhiEc			40//赊欠
#define		Sign_yong				41//用
#define		Sign_dian				42//电
#define		Sign_liang				43//量
#define		Sign_jia				44//价
#define		Sign_shi				45//时
#define		Sign_jian2				46//间
#define		Sign_duan				47//段
#define		Sign_jine				48//金额
#define		Sign_biaohao			49//表号
#define		Sign_touzhi				50//透支
#define		Sign_goudian			51//购电
#define		Sign_Err				52//Err-
#define		Sign_Dianya				53//U-
#define		Sign_Dianliu			54//I-
#define		Sign_Gonglv				55//P-
#define		Sign_Gonglvyinsu		56//PF-
#define		Sign_Linxiandianliu		57//Ib-
#define		Sign_Zuhe				58//组合
#define		Sign_Zhengxiang			59//正向
#define		Sign_Fanxiang			60//反向
#define		Sign_shang				61//上
#define		Sign_doubledot			62//:
#define		Sign_Page_Sequ			63//显示序号
#define		Sign_Jiesuanri			64//结算日
#define		Sign_dianliang			65//电量
#define		Sign_dangqianyue        66//当前月
#define		Sign_Linxian            67
#define		Sign_HuoXian            68

#define		MaxLcd				22

//需要重新定义
#define		Send_DisplayData()	{;;}	//送显示数据
#define		Init_LCD()			{;;}	//初始化LCD
#define		P_RELAY_ON			1;		
#define		P_BACKLIGHT_ON		1;
#define		KeyPortCheck		1;		//按键检测

//Flag.Power
#define		F_PowerOn			0x0001
//Flag.Key
#define		F_KeyDisp0			0x01
#define		F_KeyDisp1			0x02
#define		F_KeyDispay			0x04
#define		F_KeyAwake			0x08
//Flag.Disp
#define		F_DispStop			0x01
#define		F_DispOff			0x02
#define		F_DispUpdate		0x04
#define		F_DispFlash			0x08
#define		F_DispPause			0x10
#define		F_DispRst			0x20
#define		F_DispIr_BG			0x40
#define		F_DispAll			0x80
//Flag.CommDisp
#define		F_CommDisp485		0x10
#define		F_CommDispIr		0x20
#define		F_CommDispPlc		0x40

typedef struct
{
	dyn_persistent_start
		const unsigned long		ID;//显示项ID
		const unsigned char		Sequ;//显示项序号
		unsigned short			Addr;//数据地址
		const unsigned short	Info;//数据信息
		const unsigned short	Sign;//显示内容信息
	dyn_persistent_end
} DISPLAYTAB;

 const  DISPLAYTAB DisplayTab[28];
 
#define 	DisplayTabNum		sizeof(DisplayTab)/sizeof(DISPLAYTAB)

typedef struct
{
	dyn_persistent_start
		unsigned char   Alarm;
		unsigned short	Power;
		unsigned char   Disp;
		unsigned char   Key;
		unsigned char	CommDisp;
	dyn_persistent_end
} FLAG;
FLAG				Flag;

typedef struct
{
	
	dyn_persistent_start
		unsigned long	Page_ID;		//液晶显示内容对应的数据标识
		unsigned char   Page_Sequ;		//液晶显示内容对应的序号
		unsigned char	AutoInv;		//轮显计时
		unsigned char   AutoNum;		//轮显项数
		unsigned char	AutoPtr;		//轮显项指针
		unsigned char	KeyInv;			//按键时间
		unsigned char   KeyNum;			//按键显示项数
		unsigned char	KeyPtr;			//按键显示项指针
		unsigned short 	BackLightTmr;	//背光延时时间
		unsigned char 	Key;			//按键标志
	dyn_persistent_end
} DISP;
extern		DISP				Disp;

unsigned char 		Lcdcon[MaxLcd];

//空表格
const unsigned char  SIGN_NULL[9] = { Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
//瞬时量
const unsigned char  SIGN_DIANYA[9] = { Sign_dot1,Sign_Dianya, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_DIANLIU[9] = { Sign_dot3,Sign_Dianliu, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_LINGXIANDIANLIU[9] = { Sign_dot3,Sign_Linxiandianliu, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_GONGLV[9] = { Sign_kW, Sign_dot4, Sign_Gonglv, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_WUGONGGONGLV[9] = { Sign_kvar, Sign_dot4, Sign_Gonglv, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_GONGLVYINSU[9] = { Sign_dot3,Sign_Gonglvyinsu, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_HuoXianGONGLVYINSU[9] = { Sign_dot3,Sign_Gonglvyinsu, Sign_HuoXian, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
//费率表
const unsigned char  SIGN_DANGQIANFEILVDANJIA[9] = { Sign_dangqian, Sign_dian, Sign_jia, Sign_dot4, Sign_yuan, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_DANGQIANJIETIDANJIA[9] = { Sign_dangqian, Sign_jieti, Sign_dian, Sign_jia, Sign_dot4, Sign_yuan, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_DANGQIANDANJIA[9] = { Sign_dangqian, Sign_dian, Sign_jia, Sign_dot4, Sign_yuan, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
//表号
const unsigned char  SIGN_BIAOHAO[9] = { Sign_biaohao, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
//户号
const unsigned char  SIGN_HUHAO[9] = { Sign_huhao, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
//当前时间
const unsigned char  SIGN_RIQI[9] = { Sign_shi, Sign_jian2, Sign_date, Sign_dangqian, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_SHIJIAN[9] = { Sign_shi, Sign_jian2, Sign_time, Sign_dangqian, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_CHANGSHU[9] = { Sign_changshu, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_JIESUANRI[9] = { Sign_Jiesuanri, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };

//显示表格定义
const  DISPLAYTAB DisplayTab[] =
{
	//当前时间
	0x04000101,		0x00,		(unsigned int)&Clock_Block.Clock.Day,	DispData_BCD + DispData_RAM + 3,					(unsigned int)SIGN_RIQI,//日期
	0x04000102,		0x00,		(unsigned int)&Clock_Block.Clock.Sec,	DispData_BCD + DispData_RAM + 3,					(unsigned int)SIGN_SHIJIAN,//时间
	//瞬时量
	0x02010100,		0x00,		(unsigned int)&Real.U,				DispData_BCD + DispData_RAM + 2,						(unsigned int)SIGN_DIANYA,//电压
	0x02020100,		0x00,		(unsigned int)&Real.I,				DispData_Sign + DispData_BCD + DispData_RAM + 3,		(unsigned int)SIGN_DIANLIU,//火线电流
	0x02800001,		0x00,		(unsigned int)&Real.Ib,				DispData_Sign + DispData_BCD + DispData_RAM + 3,		(unsigned int)SIGN_LINGXIANDIANLIU,//零线电流
	0x02030100,		0x00,		(unsigned int)&Real.P,				DispData_Sign + DispData_BCD + DispData_RAM + 3,		(unsigned int)SIGN_GONGLV,//A相功率
	0x02030000,		0x00,		(unsigned int)&Real.P,				DispData_Sign + DispData_BCD + DispData_RAM + 3,		(unsigned int)SIGN_GONGLV,//A相功率
	0x02040000,		0x00,		(unsigned int)&Real.Q,				DispData_Sign + DispData_BCD + DispData_RAM + 3,		(unsigned int)SIGN_WUGONGGONGLV,//无功功率
	0x02040100,		0x00,		(unsigned int)&Real.Q,				DispData_Sign + DispData_BCD + DispData_RAM + 3,		(unsigned int)SIGN_WUGONGGONGLV,//无功功率
	0x02060100,		0x00,		(unsigned int)&Real.PF,				DispData_Sign + DispData_BCD + DispData_RAM + 2,		(unsigned int)SIGN_GONGLVYINSU,//A相功率因数
	0x02060000,		0x00,		(unsigned int)&Real.PF,				DispData_Sign + DispData_BCD + DispData_RAM + 2,		(unsigned int)SIGN_GONGLVYINSU,//A相功率因数
	//表号、客户编号
	0x04000401,		0x01,		Comm_Number+4,						DispData_BCD + DispData_E2PROM + 2,						(unsigned int)SIGN_NULL,
	0x04000401,		0x00,		Comm_Number,						DispData_BCD + DispData_E2PROM + 4,						(unsigned int)SIGN_NULL,
	0x04000402,		0x01,		Meter_Number+4,						DispData_BCD + DispData_E2PROM + 2,						(unsigned int)SIGN_BIAOHAO,//表号高4位
	0x04000402,		0x00,		Meter_Number,						DispData_BCD + DispData_E2PROM + 4,						(unsigned int)SIGN_BIAOHAO,//表号低8位
	0x0400040E,		0x01,		User_Number+4,						DispData_BCD + DispData_E2PROM + 2,						(unsigned int)SIGN_HUHAO,//客户编号高4位
	0x0400040E,		0x00,		User_Number,						DispData_BCD + DispData_E2PROM + 4,						(unsigned int)SIGN_HUHAO,//客户编号低8位
	0x04000409,		0x00,		Meter_Const1,						DispData_BCD + DispData_E2PROM + 3,						(unsigned int)SIGN_CHANGSHU,//电表有功常数
	0x04000B01,		0x00,		Balance_Time1,						DispData_BCD + DispData_E2PROM + 2,						(unsigned int)SIGN_JIESUANRI,//第一结算日		
	0x0c0c0c0c,		0x00,		(unsigned int)&Disp.Verison,		DispData_BCD + DispData_RAM + 3,						(unsigned int)SIGN_NULL,
}
//0-F显示表格
const unsigned char  NumSeg[16] = 
{
	DS_Chr_0, DS_Chr_1, DS_Chr_2, DS_Chr_3,
	DS_Chr_4, DS_Chr_5, DS_Chr_6, DS_Chr_7,
	DS_Chr_8, DS_Chr_9, DS_Chr_A, DS_Chr_B,
	DS_Chr_C, DS_Chr_D, DS_Chr_E, DS_Chr_F
};

void ClearLCD(void)
{
	unsigned char i;

	for( i=0;i<MaxLcd;i++ ) Lcdcon[i] = 0x00;
}

/***********************************************************************/
/*  函数名称：AllDisplayLCD                                            */
/*  函数功能：液晶全屏显示                                             */
/***********************************************************************/
void AllDisplayLCD(void)
{
	unsigned char i, Tmr;
	
	P_RELAY_ON;		//拉闸灯
	P_BACKLIGHT_ON;	//背光
	Disp.BackLightTmr = 0;
	//从EEPROM中获取全屏显示时间
	if(Tmr > 30)
		Tmr = 5;
    Disp.BackLightTmr = Tmr;		
	Flag.Disp |= F_DispAll;
	for( i=0;i<MaxLcd;i++ )
	{
		Lcdcon[i] = 0xFF;
	}
	Send_DisplayData();	//送显示数据
	Disp.AutoInv = Tmr;
	Disp.AutoPtr = 0xFF;
	Disp.Page_ID = 0xFFFFFFFF;
	Disp.Page_Sequ = 0x00;
}

/***********************************************************************/
/*  函数名称：Key_Display                                              */
/*  函数功能：获取按键显的显示项ID                                     */
/***********************************************************************/
void Key_Display(void)
{
	if( Disp.Key & F_KeyDisp0 )
	{
		if(KeyPortCheck == 0)
		{
			Delay_Xms(1);
			if(KeyPortCheck == 0)
				Disp.Key |= F_KeyDisp1;
			else 
				Disp.Key &= ~(F_KeyDisp0+F_KeyDisp1);
		}
		else
			Disp.Key &= ~(F_KeyDisp0+F_KeyDisp1);
	}
	if( Disp.Key & F_KeyDisp1 )//按键显
	{
		Disp.Key &= ~(F_KeyDisp0+F_KeyDisp1);
		Flag.Run |= F_Disp;
		if( Flag.Power & F_PowerOn )
			Disp.BackLightTmr = 60;//按键时,背光点亮
		else
		{
			Flag.Disp &= ~F_DispStop;
			if( ((Disp.Key & F_KeyAwake) == 0) && (Flag.Disp & F_DispOff) )
			{
				Flag.Disp &= ~F_DispPause;
				Disp.Key |= F_KeyAwake;
				Flag.Disp &= ~F_DispOff;
				Disp.AutoPtr = 0xFF;
				Disp.AutoInv = 0;
                Init_LCD();
				return;
			}
		}

		if( (Disp.Key & F_KeyDispay) == 0 )
		{
			Disp.Key |= F_KeyDispay;
			Disp.KeyPtr = 0xFF;
		}
		else
		{
			//E2P_Read( (unsigned char*)&Disp.KeyNum, KeyDisp_Num, 1 );
			if( Disp.KeyNum == 0 )
				Disp.KeyNum = 1;
			else
				Disp.KeyNum = BCD_BYTE( Disp.KeyNum );
		}
		if( Disp.KeyPtr >= (Disp.KeyNum-1) )
			Disp.KeyPtr = 0;
        	else
			Disp.KeyPtr++;
		E2P_Read( (unsigned char*)&Disp.Page_ID, KeyDisp_Table+5*Disp.KeyPtr, 5 );
		if( Flag.Power & F_PowerOn )
		{
			E2P_Read( (unsigned char*)&Disp.KeyInv, KeyDisp_Inv, 1 );
			if( Disp.KeyInv == 0 )
				Disp.KeyInv = 5;
			else
				Disp.KeyInv = BCD_BYTE( Disp.KeyInv );
		}
		else
				Disp.KeyInv = 30;
	}
	if( (Disp.Key & F_KeyDispay) && (Disp.KeyInv == 0) )//退出键显
	{
		Disp.Key &= ~F_KeyDispay;
		Disp.AutoPtr = 0xFF;
		Disp.AutoInv = 0;
		if( (Flag.Power & F_PowerOn) == 0 )//按键显示结束后,进入掉电暂显
		{
			Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
		}
	}
}

/***********************************************************************/
/*  函数名称：Get_DisplayPage                                          */
/*  函数功能：获取轮显的显示项ID                                       */
/***********************************************************************/
void Get_DisplayPage(void)
{
	if( (Disp.Key & F_KeyDispay) == 0 )//轮显
	{
		if( Disp.AutoInv == 0 )
		{
			if( ((Flag.Power & F_PowerOn) == 0) && ((Disp.AutoPtr == (Disp.AutoNum-1))) )//自动循环显示一遍后,进入掉电暂显
			{
				Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
				return;
			}
			//获取轮显显示时间
			E2P_Read( (unsigned char*)&Disp.AutoInv, AutoDisp_Inv, 1 );
			if( (Disp.AutoInv < 0x05) || (Disp.AutoInv > 0x20) )
				Disp.AutoInv = 5;
			else
				Disp.AutoInv = BCD_BYTE( Disp.AutoInv );
			//获取轮显显示屏数
			E2P_Read( (unsigned char*)&Disp.AutoNum, AutoDisp_Num, 1 );
			if( Disp.AutoNum == 0 )
				Disp.AutoNum = 1;
			else
				Disp.AutoNum = BCD_BYTE( Disp.AutoNum );
			if( Disp.AutoPtr >= (Disp.AutoNum-1) )
				Disp.AutoPtr = 0;
            		else
				Disp.AutoPtr++;
			E2P_Read( (unsigned char*)&Disp.Page_ID, AutoDisp_Table+5*Disp.AutoPtr, 5 );
//			PassDispNoNeed(Disp.AutoNum,(unsigned int*)&Disp.AutoPtr,1);
		}
	}
}
/***********************************************************************/
/*  函数名称：DispNum                                                  */
/*  函数功能：显示BCD数字                                              */
/*  输入参数：Addr：显示数据地址                                       */
/*            Lenth：数据长度                                          */
/***********************************************************************/
void DispNum( unsigned char* Addr, unsigned char Lenth )
{
	unsigned char i=11;

	if( Lenth >5 ) return;
	while( Lenth > 0 )
	{
		Lcdcon[i] = NumSeg[*Addr & 0x0F];
		i--;
		Lcdcon[i] = NumSeg[((*Addr)>>4) & 0x0F];
		i--;
		Addr++;
		Lenth--;
	}
}
/***********************************************************************/
/*  函数名称：GetDispAddrInfo                                          */
/*  函数功能：根据给定的ID，找出其对应的数据地址和信息                 */
/*  输入参数：ID:显示ID                                                */
/*            Addr：数据地址                                           */
/*            Info：数据信息                                           */
/*  输出参数：0为ID存在，1为ID不存在                                   */
/***********************************************************************/
unsigned char GetDispAddrInfo( unsigned long ID, unsigned int* Addr, unsigned int* Info, unsigned char* SignBuf )
{
	unsigned char i,j;
	unsigned char IDL0,IDL1;
	unsigned char IDH0,IDH1;
	unsigned int IDH;
	unsigned char Info_Num,Info_Max;
	unsigned int Info_Addr,Info_Len,Info_BlockLen;

	IDH = (unsigned int)((ID & 0xFFFF0000)>>16);
	IDL1 = (unsigned char)((ID & 0x0000FF00)>>8);
	IDL0 = (unsigned char)((ID & 0x000000FF));
	IDH1 = (unsigned char)((ID & 0xFF000000)>>24);
	IDH0 = (unsigned char)((ID & 0x00FF0000)>>16);

	memcpy( SignBuf, SIGN_NULL, 8 );
	if(IDH1 == 0x00)//电能量类
	{
		if( IDL0 == 0x00 )//当前
		{
			*Info = DispData_BCD + DispData_RAM + 5;
			if(( IDH == 0x0000 ) || (IDH == 0x000B) || (IDH == 0x000C))
				(*Info) |= DispData_Sign;//组合有功电量最高位是符号位
			if((IDH0 == 0x0B)||(IDH0 == 0x0C))//当前一月
			{
				*SignBuf = Sign_dangqianyue;
				*(SignBuf+1) = Sign_1;
			}
			else
				*SignBuf = Sign_dangqian;//当前
		}
		else//历史
		{
			*Info = DispData_His + DispData_BCD + DispData_E2PROM + 5;
			if(( IDH == 0x0000 ) || (IDH == 0x000B) || (IDH == 0x000C))
				(*Info) |= DispData_Sign;//组合有功电量最高位是符号位
			*SignBuf = Sign_1 + j;
			*(SignBuf+1) = Sign_shangyue;
		}
		if((IDH0<0x03) || (IDH0 == 0X0B) || (IDH0 == 0x0C))
			*(SignBuf+2) = Sign_kWh;//注意：Sign_dot2一定要在Sign_kWh之后的寄存器
		else
			*(SignBuf+2) = Sign_kvarh;
		*(SignBuf+3) = Sign_dot2;
		*(SignBuf+4) = Sign_dianliang;
		switch(IDH0)
		{
			case 0x00:
				*(SignBuf+5) = Sign_Zuhe;//组合
				*(SignBuf+6) = Sign_YouGong;//有功
				break;
			case 0x01:
				*(SignBuf+5) = Sign_Zhengxiang;//正向
				*(SignBuf+6) = Sign_YouGong;//有功
				break;
			case 0x02:
				*(SignBuf+5) = Sign_Fanxiang;//反向
				*(SignBuf+6) = Sign_YouGong;//有功
				break;
			case 0x0B:
			case 0x0C://月净用组合有功电量
				*(SignBuf+5) = Sign_yong;
				*(SignBuf+6) = Sign_Zuhe;
				break;
			default:
				break;
		}
		if(((0x0B == IDH0) || (0x0C == IDH0))&&(0 == i))//净用电量显示为“当前一月用电量、上一月用电量，不显示总”
			*(SignBuf+8) = 0;
		else			
			*(SignBuf+8) = Sign_FeiLvNum;//T1~T12
	}
	return 0;
}

/***********************************************************************/
/*  函数名称：Display_Meter                                            */
/*  函数功能：显示电表相关指示信息                                     */
/***********************************************************************/
void Display_Meter(void)
{
	unsigned char i;
	unsigned long Bit=0x00000001;
	if( Disp.Page_ID == 0xFFFFFFFF ) return;//当前全屏显，则返回
	
	for(i=0;i<Card.KeyNum;i++)
	{
		if((Statue.SecretKey & (Bit<<i)) == 0 )
			break;
	}
	if( i < Card.KeyNum)
		Lcdcon[19] |= DS_Chr_Lab;	
	if( Flag.Card & F_CommMACErr )		
		Lcdcon[14] |= DS_Chr_Lock;
	
	if( Flag.Power & F_PowerOn )
	{
		if( Timer.BleAuthen_Tmr >0 )
			Lcdcon[14] |= DS_Chr_KeyProgram;
		//通讯指示
		if( Flag.CommDisp & F_CommDisp485 ) 
		{
			Flag.CommDisp &= ~F_CommDisp485;
			Lcdcon[15] |= DS_Chr_Comm;
		}
		if( Flag.CommDisp & F_CommDispIr ) 
		{
			Flag.CommDisp &= ~F_CommDispIr;
			Lcdcon[15] |= DS_Chr_Comm;
		}		
		
		if( Flag.CommDisp & F_CommDispPlc ) 
		{
			Flag.CommDisp &= ~F_CommDispPlc;
			Lcdcon[14] |= DS_Chr_CommPLC;
		}

		//电流反向
		if( Timer.RevCnt >= 5 ) Lcdcon[21] |= DS_Chr_Reverse;
		//电池欠压指示
		if( Flag.EletPar & F_Event_BatLow ) Lcdcon[20] |= DS_Chr_Bat;
		//密码验证闭锁指示
		if( (Timer.Pass03Err_Cnt >= 5) || (Timer.Pass04Err_Cnt >= 5) ) Lcdcon[14] |= DS_Chr_Lock;
		
		if(Flag.MeterRunCtr & F_Lcd12Mode)
		{
			//费率电价主、副表指示
			if( Flag.Fee & F_TOUFeeChangeTm ) Lcdcon[16] |= DS_Chr_TouTable1+DS_Chr_TouTable2;
			else  Lcdcon[16] |= DS_Chr_TouTable1;
		}
		else
		{
			//日时段主、副表指示
			if( Fee.DayTable == 0 ) Lcdcon[16] |= DS_Chr_TouTable1;
			else if( Fee.DayTable == 1 ) Lcdcon[16] |= DS_Chr_TouTable2;
		}

		//分时费率指示
		switch(Fee.TOUNo)
		{
			case 1:
                Lcdcon[21] |= DS_Chr_Jian2;
				break;
			case 2:
                Lcdcon[21] |= DS_Chr_Feng;
				break;
			case 3:
                Lcdcon[21] |= DS_Chr_Ping;
				break;
			case 4:
                Lcdcon[21] |= DS_Chr_Gu;
				break;
            default:
                if(Fee.TOUNo != 0)
                {
                    Lcdcon[16] |= DS_Chr_T;
                    if(Fee.TOUNo < 10)    Lcdcon[13] |= NumSeg[Fee.TOUNo];
                    else
                    {
                        Lcdcon[16] |= DS_Chr_P2;
                        Lcdcon[13] |= NumSeg[Fee.TOUNo-10];
                    }
                }                    
                    
                break;
		}
		//阶梯费率主、副表指示
        if(Flag.Fee & F_STEPModeOn)
        {
            if( Flag.Fee & F_STEPChangeTm )  Lcdcon[18] |= DS_Chr_StepTable1+DS_Chr_StepTable2;
            else Lcdcon[18] |= DS_Chr_StepTable1;
            //阶梯费率指示		
            if((Fee.STEPNo > 0) && (Fee.STEPNo < 9))
            {
                Lcdcon[17] |= DS_Chr_L2;
                Lcdcon[12] |= NumSeg[Fee.STEPNo];
            }
        }
		//闪烁
		if( (Flag.Disp & F_DispFlash) == 0 )
		{
			if( Flag.RemoteCmd & F_RemoteAlarm )
				Lcdcon[19] |=  (DS_Chr_GouDian);
			if(( Timer.RemoteRelayOff_DelayTmr > 0 ) || (Relay.CmdDtTmr>0))//跳闸延时过程中指示(闪烁"拉闸)
				Lcdcon[19] |= DS_Chr_LaZha;
            if(Common.Factory == Factory_Mode)
                Lcdcon[19] |= DS_Chr_Lab;
		}
		else
		{
			if( Flag.RemoteCmd & F_RemoteAlarm ) 
				Lcdcon[19] &= ~(DS_Chr_GouDian);
			if(( Timer.RemoteRelayOff_DelayTmr > 0 ) || (Relay.CmdDtTmr>0))
				Lcdcon[19] &= ~DS_Chr_LaZha;
            if(Common.Factory == Factory_Mode)
                Lcdcon[19] &= ~DS_Chr_Lab;

		}
		//继电器状态指示
		if( (Flag.Relay & F_StateOff) && (Flag.Relay & F_RelayOff) 
		&& (((Credit_Block.Credit.Amount+MeterPara_Block.MeterPara.OverdraftCredit) <= 0) || ( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.RelayOnCredit)&&(Flag.MeterState & F_RelayOffed) ) || (Flag.RemoteCmd & F_RemoteRelayOff))) 
			Lcdcon[19] |= DS_Chr_LaZha;
	}
} 

/***********************************************************************/
/*  函数名称：Disp_Value                                               */
/*  函数功能：显示数值                                                 */
/*  输入参数：Addr：数据地址                                           */
/*            Info：数据信息                                           */
/***********************************************************************/
void Disp_Value( unsigned int Addr, unsigned int Info )
{
	unsigned char Len,MemType;
	unsigned long Tmp;
	unsigned char Sign = 0;
	Len = (unsigned char)(Info & DispDataLen);
	MemType = (unsigned char)((Info>>6) & MEMVALUE);
	
	//获取显示数据
	
	if(Disp.Page_ID == 0x04000101)
    {
        Buffer[3] = 0x20;
        Len = 4;
    }
	if( Info & DispData_HEX )
	{
        memcpy( (unsigned char*)&Tmp, Buffer, 4 );
        HEX_BCD( Buffer, Tmp );
	}
	else//
	{
		if(Flag.Power & F_PowerOn)
        {
			if(( Info & DispData_Sign ) && ((Disp.Page_ID & 0x02000000) == 0x02000000) )//电能表潮流反向时,显示反向字符,正向时显示正向字符
            {
                if( Buffer[Len-1] & 0x80 )
                {
                    Buffer[Len-1] &= 0x7f;
                    Lcdcon[15] |= DS_Chr_Xiang;
                    Lcdcon[14] |= DS_Chr_Fan;
                }
                else
                {
                    Lcdcon[15] |= DS_Chr_Xiang;
                    Lcdcon[14] |= DS_Chr_Zheng;
                }
            }
		}
		if(( Info & DispData_Sign ) && ( ((Disp.Page_ID & 0xFFFF0000) == 0x00000000)
		||((Disp.Page_ID & 0xFFFF0000) == 0x000B0000)||((Disp.Page_ID & 0xFFFF0000) == 0x000C0000)) )
		{
		  	if( Buffer[Len-1] & 0x80 )
			{
				Buffer[Len-1] &= 0x7f;
				Lcdcon[21] |= DS_Chr_Fu;
			}	     		
		}
	}
 	DispNum( Buffer, Len );
	if( 0x5A == Sign )
	{
		for(Sign=0;Sign<5;Sign++)
		{
			if(Lcdcon[Sign] != NumSeg[0])
				break;
		}
		if(Sign == 0)
			Lcdcon[21] |= DS_Chr_Fu;
		else
			Lcdcon[Sign-1] = DS_Chr__;
	}
}

/***********************************************************************/
/*  函数名称：ZeroDark                                                 */
/*  函数功能：高位0不显示                                              */
/*  输入参数：i：最多不显示的个数                                      */
/***********************************************************************/
void ZeroDark( unsigned char i )
{
	unsigned char j;

	for( j=2;j<i;j++)
	{
		if( (Lcdcon[j] == NumSeg[0]) || (Lcdcon[j] == 0) )
			Lcdcon[j] = 0;
		else
			break;
	}
} 
/***********************************************************************/
/*  函数名称：Disp_Char                                                */
/*  函数功能：显示字符和单位                                           */
/*  输入参数：i：显示项在显示表格里对应的位置                          */
/***********************************************************************/
void Disp_Char( unsigned char* SignBuf )
{
	unsigned char j,Tmp;
	unsigned char ID1L;
	ID1L = (unsigned char)((Disp.Page_ID & 0x0000FF00)>>8);

	for( j=0;j<9;j++ )
	{
		switch( (*SignBuf) )
		{
			case Sign_kWh:
				Lcdcon[17] |= DS_Chr_kW;
				Lcdcon[18] |= DS_Chr_h;
				switch( DispPara_Block.DispPara.EnergyDot )
				{
					case 0:
						memset( Buffer, NumSeg[0], 8 );
						memcpy( (unsigned char*)&Buffer[0], (unsigned char*)&Lcdcon[4], 6 );
						memcpy( (unsigned char*)&Lcdcon[6], Buffer, 6 );
						break;
					case 1:
						memset( Buffer, NumSeg[0], 8 );
						memcpy( (unsigned char*)&Buffer[0], (unsigned char*)&Lcdcon[4], 6 );
						memcpy( (unsigned char*)&Lcdcon[6], Buffer, 6 );
						break;
					case 2:
						memset( Buffer, NumSeg[0], 8 );
						memcpy( (unsigned char*)&Buffer[0], (unsigned char*)&Lcdcon[2], 8 );
						memcpy( (unsigned char*)&Lcdcon[4], Buffer, 8 );
						*(SignBuf+1) = Sign_dot2;
						break;
					case 3:
						memset( Buffer, NumSeg[0], 8 );
						memcpy( (unsigned char*)&Buffer[(4-DispPara_Block.DispPara.EnergyDot)/2], (unsigned char*)&Lcdcon[DispPara_Block.DispPara.EnergyDot/2], 6+(DispPara_Block.DispPara.EnergyDot%2) );
						memcpy( Lcdcon, Buffer, 8 );
						if( DispPara_Block.DispPara.EnergyDot == 0 )
						{
							*(SignBuf+1) = 0;
							ZeroDark(7);
						}
						else
							*(SignBuf+1) = Sign_dot1 + (DispPara_Block.DispPara.EnergyDot-1);//注意：Sign_dot2一定要在Sign_kWh之后的寄存器		
						break;
					default:
						break;
				}
				break;
			case Sign_kW:
				Lcdcon[17] |= DS_Chr_kW;
				switch( DispPara_Block.DispPara.PowerDot )
				{
					case 0:
					case 1:
					case 2:
					case 3:
						memset( Buffer, NumSeg[0], 8 );
						memcpy( (unsigned char*)&Buffer[6-DispPara_Block.DispPara.PowerDot], (unsigned char*)&Lcdcon[2], 2+DispPara_Block.DispPara.PowerDot );
						memcpy( Lcdcon, Buffer, 8 );
						if( DispPara_Block.DispPara.PowerDot == 0 )
						{
							*(SignBuf+1) = 0;
							ZeroDark(7);
						}
						else
							*(SignBuf+1) = Sign_dot1 + (DispPara_Block.DispPara.PowerDot-1);//注意：Sign_dot2一定要在Sign_kWh之后的寄存器
						break;
					default:
						break;
				}
				break;
			case Sign_dot1:
				ZeroDark(10);
				Lcdcon[16] |= DS_Chr_Dot1;
				break;
			case Sign_dot2:
				ZeroDark(9);
				Lcdcon[17] |= DS_Chr_Dot2;
				break;
			case Sign_dot3:
				ZeroDark(8);
				Lcdcon[18] |= DS_Chr_Dot3;
				break;
			case Sign_dot4:
				ZeroDark(7);
				Lcdcon[15] |= DS_Chr_Dot4;
				break;
			case Sign_time:
				Lcdcon[17] |= DS_Chr_Dot22;
				Lcdcon[18] |= DS_Chr_Dot42;
			case Sign_date:
				Lcdcon[17] |= DS_Chr_Dot2;
				Lcdcon[15] |= DS_Chr_Dot4;
				break;
			case Sign_dangqian:
				Lcdcon[20] |= DS_Chr_DangQian;
				break;
			case Sign_shangyue://
				Lcdcon[14] |= DS_Chr_Shang;
				Lcdcon[20]|= DS_Chr_Yue;
				break;
			case Sign_dangqianyue:
				Lcdcon[20] |= DS_Chr_DangQian;
				Lcdcon[20]|= DS_Chr_Yue;
				break;
			case Sign_yue:
				Lcdcon[20]|= DS_Chr_Yue;
				break;
			case Sign_10:
			case Sign_11:
			case Sign_12:
			case Sign_13:
			case Sign_14:
			case Sign_15:
			case Sign_16:
			case Sign_17:
			case Sign_18:
				Lcdcon[14] |= DS_Chr_P0;
				(*SignBuf) -= 10;
			case Sign_1:
			case Sign_2:
			case Sign_3:
			case Sign_4:
			case Sign_5:
			case Sign_6:
			case Sign_7:
			case Sign_8:
			case Sign_9:
				Lcdcon[0] |= NumSeg[*SignBuf];
				break;
			case Sign_FeiLvNum://费率号
				if(ID1L >= 10)
				{
					Lcdcon[20] |= DS_Chr_P1;
					Lcdcon[1] |= NumSeg[ID1L&0x0F];
					Lcdcon[20] |= DS_Chr_FeiLv;
				}
				else if(ID1L == 0)//总
				{
					Lcdcon[16] |= DS_Chr_Zong;
				}
				else//费率5~9
				{
					Lcdcon[1] |= NumSeg[ID1L];
					Lcdcon[20] |= DS_Chr_FeiLv;
				}
				break;
			case Sign_changshu://常数
				Lcdcon[17] |= DS_Chr_ChangShu;
				break;
			case Sign_yong: //用
				Lcdcon[15] |= DS_Chr_Yong;
				break;
			case Sign_shi://时
				Lcdcon[15] |= DS_Chr_Shi;
				break;
			case Sign_jian2://间
				Lcdcon[15] |= DS_Chr_Jian;
				break;
			case Sign_duan://段
				Lcdcon[19] |= DS_Chr_Duan;
				break;
			case Sign_biaohao://表号
				Lcdcon[19] |= DS_Chr_Biao;
				Lcdcon[17] |= DS_Chr_Hao;
				break;
			case Sign_huhao://户号
				Lcdcon[18] |= DS_Chr_Hu;
				Lcdcon[17] |= DS_Chr_Hao;
				break;
			case Sign_Dianya://电压
				Lcdcon[17] |= DS_Chr_UV;
				break;
			case Sign_Dianliu://火线电流
				Lcdcon[21] |= DS_Chr_Lline;
				Lcdcon[17] |= DS_Chr_IA;
				break;
			case Sign_Gonglvyinsu://功率因数
				Lcdcon[14] |= DS_Chr_COS;
				break;
			case Sign_Linxiandianliu://零线电流
				Lcdcon[21] |= DS_Chr_Nline;
				Lcdcon[17] |= DS_Chr_IA;
				break;
			case Sign_Linxian://零线电流
				Lcdcon[21] |= DS_Chr_Nline;
				break;		
			case Sign_HuoXian://火线电流
				Lcdcon[21] |= DS_Chr_Lline;
				break;
			case Sign_Zuhe:
				//Lcdcon[20] |= DS_Chr_ZuHe;
				break;
			case Sign_Zhengxiang:
				Lcdcon[15] |= DS_Chr_Xiang;
				Lcdcon[14] |= DS_Chr_Zheng;
				break;
			case Sign_Fanxiang:
				Lcdcon[15] |= DS_Chr_Xiang;
				Lcdcon[14] |= DS_Chr_Fan;
				break;
			case Sign_doubledot:
				Lcdcon[17] |= DS_Chr_Dot22;
				Lcdcon[18] |= DS_Chr_Dot42;
				break;
			case Sign_Page_Sequ://时段表内容序号显示
				Tmp = Disp.Page_Sequ;
				BCDX_INC( (unsigned char*)&Tmp, 1 );//显示时从1到14序号
				Lcdcon[0] |= NumSeg[((Tmp)&0xF0)>>4];
				Lcdcon[1] |= NumSeg[(Tmp)&0x0F];
				break;
			case Sign_Jiesuanri:
				Lcdcon[17] |= DS_Chr_Dot2;
				break;
			case Sign_dianliang:
				Lcdcon[19] |= DS_Chr_DianLiang;
				break;
			case Sign_jieti://阶梯
				Lcdcon[20] |= DS_Chr_JieTi;
				break;	
			case Sign_YouGong://有功
				Lcdcon[19] |= DS_Chr_You;
				Lcdcon[19] |= DS_Chr_Gong;
				break;
			default:
				break;
		}
		SignBuf++;
	}
}
/***********************************************************************/
/*  函数名称：Display_Page                                             */
/*  函数功能：获取显示项的显示信息                                     */
/***********************************************************************/
void Display_Page(void)
{
	unsigned char i;
	unsigned int Addr,Info;
	unsigned char SignBuf[9];

	if( Disp.Page_ID == 0xFFFFFFFF ) return;//当前全屏显,则返回
	if( GetDispAddrInfo( Disp.Page_ID, (unsigned int*)&Addr, (unsigned int*)&Info, SignBuf ) )
	{
		for( i=0;i<DisplayTabNum;i++ )
		{
			if( (Disp.Page_ID == DisplayTab[i].ID) && (Disp.Page_Sequ == DisplayTab[i].Sequ) )
				break;
		}
		if( i >= DisplayTabNum )//如果显示项在表格里不存在,则显示第一项内容
			i = 0;
		Addr = DisplayTab[i].Addr;
		Info = DisplayTab[i].Info;
		memcpy( SignBuf, (unsigned char*)DisplayTab[i].Sign, 9 );
	}

	//清屏
	ClearLCD();
	//显示数据
	Disp_Value( Addr, Info );
	//显示字符和单位
	Disp_Char( SignBuf );
}
/***********************************************************************/
/*  函数名称：Display                                                  */
/*  函数功能：液晶显示                                                 */
/***********************************************************************/
void Display(void)
{
	//获取显示项ID
    if(Flag.Power & F_PowerOn)
    {
        Get_DisplayPage();
    }
	//获取显示项内容
	Display_Page();
	//显示电表指示信息
	Display_Meter();
	//送显示内容
	Send_DisplayData();
}













