#include "target.h"
#include "string.h"
#include "nidavellir.h"
#include "applications.h"
#include "pin_function_def.h"
#include "lib_conf.h"

extern int read_record_data_form_698_protocol(struct instance_t* instance, uint8_t* buf, uint32_t Record_oad, uint32_t capture_oad, uint8_t last_no);

/**********************************************************************/
/*  液晶显示程序                                                       */
/***********************************************************************/

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
	
/*************** CON12 ***************/
#define     DS_Chr_Xiang       		0x80 	//向
#define     DS_Chr_Liang       		0x40 	//量
#define     DS_Chr_Shibai			0x20  	//失败
#define     DS_Chr_GouDian			0x10  	//请购电
#define     DS_Chr_Dot4				0x08  	//P1
#define     DS_Chr_Dot42			0x04  	//P5
#define     DS_Chr_Jia				0x02  	//价
#define     DS_Chr_Zong				0x01	//总

/*************** CON13 ***************/
#define     DS_Chr_LaZha			0x80	//拉闸
#define     DS_Chr_Hu				0x40	//户
#define     DS_Chr_FeiLv			0x20	//费率
#define     DS_Chr_Shi				0x10	//时
#define     DS_Chr_P2				0x08	//费率-10
#define     DS_Chr_h              	0x04  	//kwh-h
#define     DS_Chr_Yuan				0x02  	//元
#define     DS_Chr_L18              0x01  	//P2-费率10

/*************** CON14 ***************/
#define     DS_Chr_T				0x80  	//T
#define     DS_Chr_kW            	0x40
#define     DS_Chr_TouTable2		0x20  	//L16
#define     DS_Chr_UV              	0x10	//V
#define     DS_Chr_IA               0x08	//A
#define     DS_Chr_COS				0x04	//COS
#define     DS_Chr_Hao              0x02	//号
#define     DS_Chr_ChangShu			0x01	//常数

/*************** CON15 ***************/
#define     DS_Chr_TouTable1		0x80    //L15
#define     DS_Chr_Biao       		0x40	//表
#define     DS_Chr_Dot1				0x20	//
#define     DS_Chr_Jine				0x10	//金额
#define     DS_Chr_StepTable2		0x08    //L12
#define     DS_Chr_L13				0x04    //L13
#define     DS_Chr_StepTable1		0x02    //L11
#define     DS_Chr_Dot2				0x01    //P3

/*************** CON16 ***************/
#define     DS_Chr_Dot22            0x80    //P6
#define     DS_Chr_Duan          	0x40  	//段
#define     DS_Chr_ShengYu			0x20  	//剩余
#define     DS_Chr_Dot3             0x10
#define     DS_Chr_Jian            	0x08  	//间
#define     DS_Chr_ChengGong       	0x04  	//成功
#define     DS_Chr_Dian				0x02	//电
#define     DS_Chr_Fan				0x01	//反

/*************** CON17 ***************/
#define     DS_Chr_Yong				0x80	//用
#define     DS_Chr_Zheng			0x40	//正
#define     DS_Chr_Lock				0x20	//锁
#define     DS_Chr_TouZhi			0x10	//透支
#define     DS_Chr_ZuHe             0x08	//组合
#define     DS_Chr_JieTi            0x04	//阶梯
#define     DS_Chr_Yue              0x02	//月
#define     DS_Chr_KeyProgram		0x01	//

/*************** CON18 ***************/
#define     DS_Chr_P0               0x80 	//
#define     DS_Chr_Lab				0x40 	//小房子
#define     DS_Chr_Shang			0x20 	//上
#define     DS_Chr_Bat				0x10 	//电池欠压
#define     DS_Chr_Comm				0x08 	//485通讯符号
#define     DS_Chr_DangQian         0x04	//当前
#define     DS_Chr_Reverse			0x02	//反向符号
#define     DS_Chr_Nline            0x01	//N

/*************** CON19 ***************/
#define     DS_Chr_Fu               0x80	//负
#define     DS_Chr_Lline			0x40	//L
#define     DS_Chr_CommPLC			0x20

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

#define		MaxLcd				30

//需要重新定义
#define		Init_LCD()			{;;}	//初始化LCD
#define		P_RELAY_ON			1;		

//按键
#define     KeyPortIn           {GPIOA->OEN |=BIT2; GPIOA->IE |=BIT2;}
#define     KeyPortCheck        (GPIOA->STS & BIT2)


//Info16bit定义
// 6 bits	数据长度
#define	    DispDataLen				0x003F
// 3 bits   数据存储类型
#define 	DispData_RAM			0x0040*Mem_RAM
#define 	DispData_FRAM			0x0040*Mem_FRAM
#define 	DispData_E2PROM			0x0040*Mem_E2PROM
// 2 bits	数据存储格式
#define 	DispData_BCD			0x0200
#define 	DispData_HEX			0x0400
// 1 bits   数据存储位置(按历史记录)
#define		DispData_His			0x1000
// 1 bits   数据正负标志
#define		DispData_Sign			0x2000

#define	    Mem_RAM                  1
#define	    Mem_FRAM                 2
#define	    Mem_FLASH                 2
#define	    Mem_E2PROM               3
#define     MEMVALUE                 0x03


typedef struct
{
    unsigned short  Id; //显示项ID
    unsigned long   Oad; //oad
    unsigned char   Sequ;//序号
	unsigned int    Info;//数据信息
	unsigned int    Sign;//显示内容信息
} DISPLAYTAB;

 

typedef struct
{
		unsigned char   Alarm;
		unsigned char   Disp;
		unsigned char   Key;
        unsigned char Run;
} FLAG;
FLAG				Flag;


unsigned char 		Lcdcon[MaxLcd];
unsigned char         Buffer[40];

//按键
#define KeyPortIn       {GPIOA->OEN |=BIT2; GPIOA->IE |=BIT2;}
#define KeyPortCheck    (GPIOA->STS & BIT2)

//空表格
const unsigned char  SIGN_NULL[9] = { Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
//瞬时量
const unsigned char  SIGN_DIANYA[9] = { Sign_dot1,Sign_Dianya, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_DIANLIU[9] = { Sign_dot3,Sign_Dianliu, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_LINGXIANDIANLIU[9] = { Sign_dot3,Sign_Linxiandianliu, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
const unsigned char  SIGN_GONGLV[9] = { Sign_kW, Sign_dot4, Sign_Gonglv, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null, Sign_Null };
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

struct instance_t* instance;
struct pipeline_t* pipeline;

//显示表格定义
const  DISPLAYTAB DisplayTab[] =
{
    //当前时间
    0xA001,     0x40000200,     0x00,   DispData_RAM + 3,					                    (unsigned int)SIGN_RIQI,//日期
    0xA002,     0x40000200,     0x00,   DispData_RAM + 3,					                    (unsigned int)SIGN_SHIJIAN,//时间
    0xA003,     0x20000201,     0x00,   DispData_RAM + DispData_HEX + 2,						(unsigned int)SIGN_DIANYA,//电压
    0xA006,     0x20010201,     0x00,   DispData_RAM + DispData_Sign + DispData_HEX + 3,		(unsigned int)SIGN_DIANLIU,//火线电流
    0xA009,     0x20010400,     0x00,   DispData_RAM + DispData_Sign + DispData_HEX + 3,		(unsigned int)SIGN_LINGXIANDIANLIU,//零线电流
    0xA00A,     0x20040201,     0x00,   DispData_RAM + DispData_Sign + DispData_HEX + 3,		(unsigned int)SIGN_GONGLV,//A相功率
    0xA00B,     0x20040202,     0x00,   DispData_RAM + DispData_Sign + DispData_HEX +3,		    (unsigned int)SIGN_GONGLV,//A相功率
    0xA012,     0x200A0201,     0x00,   DispData_RAM + DispData_Sign + DispData_HEX +2,		    (unsigned int)SIGN_GONGLVYINSU,//A相功率因数
    0xA013,     0x200A0202,     0x00,   DispData_RAM + DispData_Sign + DispData_HEX + 2,		(unsigned int)SIGN_GONGLVYINSU,//A相功率因数
    
    0xB001,		0x40010200,     0x01,   DispData_E2PROM + DispData_BCD +2,						(unsigned int)SIGN_NULL,//表地址高4位
    0xB002,		0x40010200,     0x02,   DispData_E2PROM + DispData_BCD +4,						(unsigned int)SIGN_NULL,//表地址低8位
    0xB003,		0x40020200,     0x01,   DispData_E2PROM + DispData_BCD +2,						(unsigned int)SIGN_BIAOHAO,//表号高4位
    0xB004,		0x40020200,     0x02,   DispData_E2PROM + DispData_BCD +4,						(unsigned int)SIGN_BIAOHAO,//表号低8位
    0xB005,		0x40030200,     0x01,   DispData_E2PROM + DispData_BCD +2,						(unsigned int)SIGN_HUHAO,//客户编号高4位
    0xB006,		0x40030200,     0x02,   DispData_E2PROM + DispData_BCD +4,						(unsigned int)SIGN_HUHAO,//客户编号低8位
    0xB007,		0x41090200,     0x00,   DispData_E2PROM + DispData_HEX +4,						(unsigned int)SIGN_CHANGSHU,//电表有功常数  
#if 0    
    //正向电能量
    //0xC0FF,     0xC000~0xC00C:总，费率1~12;   0xC000~0xC180:当前~上24月正向总电能
    //反向电能量
    //0xD0FF,     0xD000~0xD00C:总，费率1~12;   0xD000~0xD180:当前~上24月反向总电能  
    //组合电能量
    //0xE0FF,     0xE000~0xE00C:总，费率1~12;   0xE000~0xE180:当前~上24月组合总电能  
#endif
};

#define 	DisplayTabNum		sizeof(DisplayTab)/sizeof(DISPLAYTAB)

typedef struct
{
    unsigned short  Id;     //显示项ID
    unsigned char   oad[4]; //oad
    unsigned char   Sequ;   //序号
} DISPLAYLIST;

const  DISPLAYLIST   Display_List[] = 
{
    0xA001,     0x40,0x00,0x02,0x00,        0x01,   //日期
    0xA002,     0x40,0x00,0x02,0x00,        0x02,   //时间
    0xA003,     0x20,0x00,0x02,0x01,        0x00,   //电压
    0xA006,     0x20,0x01,0x02,0x01,        0x00,   //火线电流
    0xA009,     0x20,0x01,0x04,0x00,        0x00,   //零线电流
    0xA00A,     0x20,0x04,0x02,0x01,        0x00,   //A相功率
    0xA00B,     0x20,0x04,0x02,0x02,        0x00,   //A相功率
    0xA012,     0x20,0x0A,0x02,0x01,        0x00,   //A相功率因数
    0xA013,     0x20,0x0A,0x02,0x02,        0x00,   //A相功率因数
    
    0xB001,		0x40,0x01,0x02,0x00,        0x01,   //表地址高4位
    0xB002,		0x40,0x01,0x02,0x00,        0x02,   //表地址低8位
    0xB003,		0x40,0x02,0x02,0x00,        0x01,   //表号高4位
    0xB004,		0x40,0x02,0x02,0x00,        0x02,   //表号低8位
    0xB005,		0x40,0x03,0x02,0x00,        0x01,   //客户编号高4位
    0xB006,		0x40,0x03,0x02,0x00,        0x02,   //客户编号低8位
    0xB007,		0x41,0x09,0x02,0x00,        0x00,   //电表有功常数  
    
    /************************ 组合电能 ************************/
	//总电能   0xC1FF,     0xC000~0xC00C:总，费率1~12;   0xC000~0xC180:当前~上24月正向总电能
	
	/************************ 正向电能 ************************/
	
	/************************ 反向电能 ************************/
};

#define displaytablenum     sizeof(Display_List)/sizeof(DISPLAYLIST)

void HEX64_BCD( unsigned char* BCD, unsigned long long HEX )
{
	unsigned char i;

	if( HEX > 9999999999999999 )
	{
        memset(BCD, 0x00, 8);
	}
	else
	{
		for( i=0;i<8;i++ )
		{
			*BCD = (unsigned char)(HEX%10);
            HEX = HEX/10;
            *BCD += (unsigned char)((HEX%10)<<4) & 0xF0;
            HEX = HEX/10;
            BCD++;
		}
	}
}

unsigned char display_id_to_oad(unsigned short id, unsigned char *oad)
{
    unsigned char i,idmid,idl,len=0;
    unsigned long oad1,oad2;
    unsigned short idh;
    
    for(i=0; i<displaytablenum; i++)
    {
        if(id == Display_List[i].Id)
        {
            len = 5;
            mem_swap_cpy(oad, (unsigned char *)&Display_List[i].oad, 4);
            *(oad+4) = Display_List[i].Sequ;
            return len;
        }
    }
    oad1 = 0x50050200;//组合电能
    idh = id & 0xF800;
    idmid = (unsigned char)((id & 0x07F0) >> 4);
    idl = (unsigned char)(id & 0x000F);
    if(idl > 0x0D)  return 1;
    if(idmid > 0x18)    return 1;
    oad1 = oad1 + idmid;
    switch (idh)
    {
        case 0xE000://组合
            oad2 = 0x00000200 + idl;
            break;
        case 0xE800://组合
            oad2 = 0x00000400 + idl;
            break;
        case 0xC000://正向
            oad2 = 0x00100200 + idl;
            break;
        case 0xC800://正向
            oad2 = 0x00100400 + idl;
            break;
        case 0xD000://反向
            oad2 = 0x00200200 + idl;
            break;
        case 0xD800://反向
            oad2 = 0x00200400 + idl;
            break;
        default:
            return 1;
            break;
    }
    if(idmid == 0)
    {
        memcpy(oad, (unsigned char*)&oad2, 4);
        *(oad+4) = 0;
        len = 5;
    }
    else
    {
        memcpy(oad, (unsigned char*)&oad1, 4);
        memcpy(oad+4, (unsigned char*)&oad2, 4);
        *(oad+8) = 0;
        len = 9;
    }
    return len;
}

unsigned short display_oad_to_id(unsigned char *oad, unsigned char *oad_son, unsigned char Sequ, unsigned char len)
{
    unsigned char i,idmid,idl;
    unsigned short id = 0;
    unsigned long oad1,oad2,temp=0;
    
    if(len == 4)
    {
        for(i=0; i<displaytablenum; i++)
        {
            mem_swap_cpy((unsigned char*)&temp, (unsigned char*)&Display_List[i].oad[0], 4);
            if(( mem_compare( (unsigned char*)&temp, (unsigned char*)oad, 4 ) == 0 ) && (Sequ == Display_List[i].Sequ))
            {
                id = Display_List[i].Id;
                return id;
            }
        }
        if(Sequ > 0)    return 0;
        memcpy((unsigned char*)&oad2, (unsigned char*)oad, 4);
        if(((oad2 & 0xFFFFFF00) != 0x00000400) && ((oad2 & 0xFFFFFF00) != 0x00100400) && ((oad2 & 0xFFFFFF00) != 0x00200400)
               && ((oad2 & 0xFFFFFF00) != 0x00000200) && ((oad2 & 0xFFFFFF00) != 0x00100200) && ((oad2 & 0xFFFFFF00) != 0x00200200))   return 0;
        idl = (unsigned char)(oad2 & 0x000000FF);
        if(idl > 0x0D)  return 0;
        id |= idl;
        oad2 = oad2 & 0xFFFFFF00;
        switch(oad2)
        {
            case 0x00000200:
                id |= 0xE000;
                break;
            case 0x00000400:
                id |= 0xE800;
                break;
            case 0x00100200:
                id |= 0xC000;
                break;
            case 0x00100400:
                id |= 0xC800;
                break;
            case 0x00200200:
                id |= 0xD000;
                break;
            case 0x00200400:
                id |= 0xD800;
                break;  
            default:
                id = 0;
                break;
        }
    }
    else if(len == 8)
    {
        memcpy((unsigned char*)&oad1, (unsigned char*)oad, 4);
        if(Sequ > 0)    return 0;
        if((oad1 & 0xffffff00) == 0x50050200)//月结算
        {
            idmid = (unsigned char)(oad1 & 0x000000ff);
            if((idmid > 0x18) || (idmid == 0))    return 0;
            id |= (((unsigned short)(idmid)) << 4);
            memcpy((unsigned char*)&oad2, (unsigned char*)oad_son, 4);
            if(((oad2 & 0xFFFFFF00) != 0x00000400) && ((oad2 & 0xFFFFFF00) != 0x00100400) && ((oad2 & 0xFFFFFF00) != 0x00200400)
               && ((oad2 & 0xFFFFFF00) != 0x00000200) && ((oad2 & 0xFFFFFF00) != 0x00100200) && ((oad2 & 0xFFFFFF00) != 0x00200200))   return 0;
            idl = (unsigned char)(oad2 & 0x000000FF);
            if(idl > 0x0D)  return 0;
            id |= idl;
            oad2 = oad2 & 0xFFFFFF00;
            switch(oad2)
            {
                case 0x00000200:
                    id |= 0xE000;
                    break;
                case 0x00000400:
                    id |= 0xE800;
                    break;
                case 0x00100200:
                    id |= 0xC000;
                    break;
                case 0x00100400:
                    id |= 0xC800;
                    break;
                case 0x00200200:
                    id |= 0xD000;
                    break; 
                case 0x00200400:
                    id |= 0xD800;
                    break;  
                default:
                    id = 0;
                    break;
            }
        }
    }
    return id;
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

//送显示数据
void Send_DisplayData(struct instance_t* instance, struct pipeline_t* pipeline)	
{
    unsigned long bf[4];
    uint8_t i;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(mbase_control_block->Page_ID == 0xFFFF)
    {
        for( i=0;i<MaxLcd;i++ )
        {
            Lcdcon[i] = 0xFF;
        }
    }
//    memset((unsigned char*)&Lcdcon[0], 0x00, 20);
//    memset((unsigned char*)&Lcdcon[0], NumSeg[i], 12);

    //seg2 3 8 9 10 11 75 74 69 68 

    //BIT0  BIT1    BIT3    BIT4    BIT5    BIT6
    
    //SEG3	SEG2    XX  XX
    memset((unsigned char*)&bf[0], 0x00, 16);
    bf[3] = ((unsigned long)(((Lcdcon[3] & 0x0F) << 2)) | (((Lcdcon[12] & 0x40) >> 5)) | (((Lcdcon[12] & 0x80) >> 7))) << 24;// 4/量/向
    bf[2] = ((unsigned long)(((Lcdcon[8] & 0x01) << 5) | ((Lcdcon[8] & 0x02) << 2) | ((Lcdcon[8] & 0x04) >> 2) | (Lcdcon[8] & 0x10) | ((Lcdcon[8] & 0x20) >> 3) | ((Lcdcon[8] & 0x40) >> 5))) << 16;	//9
    LCD->FB[0] = bf[3] | bf[2];
    
    //SEG11   SEG10  SEG9    SEG8    
    memset((unsigned char*)&bf[0], 0x00, 16); 
    bf[0] = (unsigned long)(((Lcdcon[12] & 0x3F)));//失败、请购电、P1/P5/价、总
    bf[1] = ((unsigned long)(((Lcdcon[13] & 0x80)>>2) | ((Lcdcon[4] & 0x70)>>2) | ((Lcdcon[13] & 0x60)>>5))) << 8;//拉闸、5、户、费率
    bf[2] = ((unsigned long)(((Lcdcon[4] & 0x0F)<<2) | ((Lcdcon[13] & 0x18)>>3))) << 16;//5、时，L2
    bf[3] = ((unsigned long)(((Lcdcon[9] & 0x01)<<5) | ((Lcdcon[9] & 0x02)<<2) | ((Lcdcon[9] & 0x04)>>2) | ((Lcdcon[9] & 0x10)) | ((Lcdcon[9] & 0x20)>>3) | ((Lcdcon[9] & 0x40)>>5))) << 24;//10
    LCD->FB[2] = bf[3] | bf[2] | bf[1] | bf[0];
    
    //SEG75   SEG74  XX    XX    
    memset((unsigned char*)&bf[0], 0x00, 16); 
    bf[3] = ((unsigned long)(((Lcdcon[11] & 0x0F)<<2) | ((Lcdcon[13] & 0x06)>>1))) << 24;//12/h/元
    bf[2] = ((unsigned long)(((Lcdcon[11] & 0x70)>>1) | ((Lcdcon[13] & 0x01)<<2) | ((Lcdcon[14] & 0xC0)>>6))) << 16;//12/L18/L17/KW
    LCD->FB[18] = bf[3] | bf[2];
    
    //XXXX    XXXX   	XXXX	SEG68
    memset((unsigned char*)&bf[0], 0x00, 16); 
    bf[0] = (unsigned long)((Lcdcon[14] & 0x3F));//L16/V/A/COS/号、常数
    LCD->FB[17] = bf[0];
    
    //SEG67 66 61 60 57 56 55 54 53 
    
    //SEG67     SEG66   XXXX    XXXX
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[3] = ((unsigned long)(((Lcdcon[10] & 0x06)<<3) | ((Lcdcon[15] & 0x80)>>4) | ((Lcdcon[7] & 0x06)) | ((Lcdcon[15] & 0x40)>>6) )) << 24;//11/L15/8/表
    bf[2] = ((unsigned long)(((Lcdcon[10] & 0x08)<<2) | ((Lcdcon[10] & 0x20)>>1) | ((Lcdcon[10] & 0x01)<<3) | ((Lcdcon[7] & 0x08)>>1) | ((Lcdcon[7] & 0x20)>>4) | ((Lcdcon[7] & 0x01)))) << 16;//11/8
    LCD->FB[16] = bf[3] | bf[2];

    //XXXX    XXXX    SEG61   	SEG60
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[1] = ((unsigned long)(((Lcdcon[10] & 0x40)>>1) | ((Lcdcon[10] & 0x10)) | ((Lcdcon[15] & 0x20)>>2) | ((Lcdcon[7] & 0x40)>>4) | ((Lcdcon[7] & 0x10)>>3) | ((Lcdcon[15] & 0x10)>>4))) << 8;//11/P4/8/金额
    bf[0] = (unsigned long)(((Lcdcon[15] & 0x0C)<<2) | ((Lcdcon[6] & 0x0F)) );//L12/L13/7
    LCD->FB[15] = bf[1] | bf[0];
    
    //XXXX	XXXX   SEG57    SEG56
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[1] = ((unsigned long)(((Lcdcon[15] & 0x03)<<4) | ((Lcdcon[16] & 0x80)>>4) | ((Lcdcon[6] & 0x70)>>4)))<<8;//L11/P3/P6/7
    bf[0] = ((unsigned long)(((Lcdcon[5] & 0x0F)<<2) | ((Lcdcon[16] & 0x60)>>5)));//6/段、剩余
    LCD->FB[14] = bf[1] | bf[0];
    
    //SEG55	SEG54    SEG53   XXXX
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[3] = ((unsigned long)(((Lcdcon[16] & 0x10)<<1) | ((Lcdcon[5] & 0x70)>>2) | ((Lcdcon[16] & 0x08)>>2) | ((Lcdcon[9] & 0x08)>>3))) << 24;//p2/6、间、10
    bf[2] = ((unsigned long)(((Lcdcon[16] & 0x04)<<3) | ((Lcdcon[3] & 0x70)>>2) | ((Lcdcon[16] & 0x03)) )) << 16;//成功、4、电、反
    bf[1] = ((unsigned long)(((Lcdcon[2] & 0x0F)<<2) | ((Lcdcon[17] & 0xC0)>>6) )) << 8;//3/用、正
    LCD->FB[13] = bf[3] | bf[2] | bf[1];
    
    //seg49 48 47 42 41 40 
    
    //XXXX    XXXX	SEG49	SEG48
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[1] = ((unsigned long)(((Lcdcon[17] & 0x20)) | ((Lcdcon[2] & 0x70)>>2) | ((Lcdcon[17] & 0x18)>>3))) << 8;//L10/3/透支、组合
    bf[0] = (unsigned long)(((Lcdcon[1] & 0x0F)<<2) | ((Lcdcon[17] & 0x06)>>1))  ;//2、阶梯、月
    LCD->FB[12] = bf[1] | bf[0];
    
    //SEG47	XXXX	XXXX	XXXX
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[3] = ((unsigned long)(((Lcdcon[17] & 0x01)<<5) | ((Lcdcon[1] & 0x70)>>2) | ((Lcdcon[8] & 0x08)>>2) | ((Lcdcon[18] & 0x80)>>7)))<< 24;//L9/2/9/L1
    LCD->FB[11] = bf[3];
    
    //XXXX	SEG42	SEG41	SEG40
    memset((unsigned char*)&bf[0], 0x00, 16); 
	bf[2] = ((unsigned long)(((Lcdcon[0] & 0x0F)<<2) | ((Lcdcon[18] & 0x60)>>5))) << 16;//1、L5、上
    bf[1] = ((unsigned long)(((Lcdcon[18] & 0x10)<<1) | ((Lcdcon[0] & 0x70)>>2) | ((Lcdcon[18] & 0x0C)>>2))) <<8;//L8/1/L4/当前
    bf[0] = (unsigned long)(((Lcdcon[18] & 0x03)<<4) | ((Lcdcon[19] & 0xE0)>>4));//last
    LCD->FB[10] = bf[2] | bf[1] | bf[0];
    
}	
/***********************************************************************/
/*  函数名称：AllDisplayLCD                                            */
/*  函数功能：液晶全屏显示                                             */
/***********************************************************************/
void AllDisplayLCD(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	
    LCD_BACK_LIGHT_OPEN();
	if((mbase_control_block->lcd_poweron_all_dis_time > 30) || (mbase_control_block->lcd_poweron_all_dis_time == 0))
		mbase_control_block->AutoInv = 5;
    else
        mbase_control_block->AutoInv = mbase_control_block->lcd_poweron_all_dis_time;

    mbase_control_block->BackLightTmr = mbase_control_block->AutoInv;
    
	mbase_control_block->AutoPtr = 0xFF;
	mbase_control_block->Page_ID = 0xFFFF;
    
    mbase_control_block->KeyPtr = 0;
    mbase_control_block->KeyInv = 0;
    mbase_control_block->Key = 0;
    
    Send_DisplayData(instance, pipeline);
    
}

/***********************************************************************/
/*  函数名称：Key_Display                                              */
/*  函数功能：获取按键显的显示项ID                                     */
/***********************************************************************/
void Key_Display(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t ctr)
{
    uint16_t pageid=0;
    uint32_t    s_index = 0;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(ctr == MSG_PAGE_KEY_UP)    //按键
    {
        Flag.Run |= F_Disp;
        if( instance->ac_power_statue == AC_POWER_ON )
            mbase_control_block->BackLightTmr = mbase_control_block->backlight_key_time;//按键时,背光点亮
        else
        {
            if(ANA_GetStatus(ANA_STATUS_COMP1) == 0)
            {
                mbase_control_block->Key &= ~F_KeyAwake;
                Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
                return;
            }
            Flag.Disp &= ~F_DispStop;
            if( ((mbase_control_block->Key & F_KeyAwake) == 0))// && (Flag.Disp & F_DispOff) )
            {
                Flag.Disp &= ~F_DispPause;
                mbase_control_block->Key |= F_KeyAwake;
                Flag.Disp &= ~F_DispOff;
                mbase_control_block->AutoPtr = 0xFF;
                mbase_control_block->AutoInv = 0;
                lcd_init(AC_POWER_ON);
                return;
            }
        }

        if( (mbase_control_block->Key & F_KeyDispay) == 0 )
        {
            mbase_control_block->Key |= F_KeyDispay;
            mbase_control_block->KeyPtr = 0xFF;
        }

        if(mbase_control_block->lcd_keydis_list_num > 0)
        {
            if( mbase_control_block->KeyPtr >= (mbase_control_block->lcd_keydis_list_num-1) )
                mbase_control_block->KeyPtr = 0;
            else
                mbase_control_block->KeyPtr++;
        }
        else
        {
            mbase_control_block->KeyPtr = 0;
        }
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + KeyDisplayList + mbase_control_block->KeyPtr);
        nd_get_index(instance, s_index, (uint8_t*)&pageid, 2); 
        mbase_control_block->Page_ID = pageid;
        
        if( instance->ac_power_statue == AC_POWER_ON )
        {
            if( mbase_control_block->lcd_keydis_keep_sec == 0 )
                mbase_control_block->KeyInv = 5;
            else
                mbase_control_block->KeyInv = mbase_control_block->lcd_keydis_keep_sec;
        }
        else
            mbase_control_block->KeyInv = 30;
    }
    
    if( (mbase_control_block->Key & F_KeyDispay) && (mbase_control_block->KeyInv == 0) )//退出键显
	{
		mbase_control_block->Key &= ~F_KeyDispay;
		mbase_control_block->AutoPtr = 0xFF;
		if( instance->ac_power_statue != AC_POWER_ON )//按键显示结束后,进入掉电暂显
		{
			Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
            mbase_control_block->Key &= ~F_KeyAwake;
		}
	}
}

/***********************************************************************/
/*  函数名称：Get_DisplayPage                                          */
/*  函数功能：获取轮显的显示项ID                                       */
/***********************************************************************/
void Get_DisplayPage(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint16_t pageid;
    uint32_t s_index = 0;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
      
	if( (mbase_control_block->Key & F_KeyDispay) == 0 )//轮显
	{
		if( mbase_control_block->AutoInv == 0 )
		{
            if( (instance->ac_power_statue != AC_POWER_ON) && ((mbase_control_block->AutoPtr == (mbase_control_block->lcd_autodis_list_num-1))) )//自动循环显示一遍后，进入掉电暂显
			{
                memset((uint8_t *)&mbase_control_block->AutoPtr, 0x00, 10);
                mbase_control_block->Key &= ~F_KeyAwake;
				Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
				return;
			}
			//获取轮显显示时间
            mbase_control_block->AutoInv = mbase_control_block->lcd_autodis_keep_sec;
			if( (mbase_control_block->AutoInv < 0x05) || (mbase_control_block->AutoInv > 0x20) )
				mbase_control_block->AutoInv = 5;
            
			//获取轮显显示屏数
			if( mbase_control_block->lcd_autodis_list_num > 0 )
            {            
                if( mbase_control_block->AutoPtr >= (mbase_control_block->lcd_autodis_list_num-1) )
                    mbase_control_block->AutoPtr = 0;
                else
                    mbase_control_block->AutoPtr++;
            }
            else
            {
                mbase_control_block->AutoPtr = 0;
            }
            
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AutoDisplayList + mbase_control_block->AutoPtr);
            nd_get_index(instance, s_index, (uint8_t*)&pageid, 2);
            
            mbase_control_block->Page_ID = pageid;
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
	unsigned char i=7;

	if( Lenth >= 5 ) return;
	while( Lenth > 0 )
	{
		Lcdcon[i] = NumSeg[*Addr & 0x0F];
		if(i>0) i--;
		Lcdcon[i] = NumSeg[((*Addr)>>4) & 0x0F];
		if(i>0) i--;
		Addr++;
		Lenth--;
	}
}

//电能小数尾数及借位处理
void display_energy_pro(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t *data, uint8_t id)
{
    uint8_t EnergyDot,i,borrownum=0;
    unsigned long long energy_p=0,p_max=0;//正向，反向
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    EnergyDot = mbase_control_block->energy_point;
    
    mem_swap_cpy((uint8_t*)&energy_p, data, 8);

    switch(EnergyDot)
    {
        case 0:
            energy_p = energy_p/10000;
            break;
        case 1:
            energy_p = energy_p/1000;
            break;
        case 2:
            energy_p = energy_p/100;
            break;
        case 3:
            energy_p = energy_p/10;
            break;
        case 4:
            break;
        default:
            energy_p = energy_p/100;
            break;
    }
    

    p_max = 99999999;
    
    for(i=0; i<EnergyDot; i++)
    {
        if(energy_p > p_max)
        {
            energy_p = energy_p / 10;
            borrownum++;
        }
        else
        {
            break;
        }
    }
    
    if(borrownum > mbase_control_block->disp_borrow)
    {
        mbase_control_block->disp_borrow = borrownum;
    }
    
    mem_swap_cpy(data, (uint8_t*)&energy_p, 4);
}

/***********************************************************************/
/*  函数名称：GetDispAddrInfo                                          */
/*  函数功能：根据给定的ID，找出其对应的数据地址和信息                 */
/*  输入参数：ID:显示ID                                                */
/*            Addr：数据地址                                           */
/*            Info：数据信息                                           */
/*  输出参数：0为ID存在，1为ID不存在                                   */
/***********************************************************************/
unsigned long GetDispAddrInfo( struct instance_t* instance, struct pipeline_t* pipeline, unsigned short ID, unsigned char* data, unsigned long Info, unsigned char* SignBuf )
{
	unsigned char D1,D2,D3,len=0;
    unsigned long oad,temp=0;
    uint32_t Record_oad=0;

    D2 = (ID & 0x07F0)>>4;
    if(D2 > 0x18)   return 1;
    D3 = (ID & 0x000F);
    if(D3 > 0x0D)   return 1;    
    
    D1 = (ID & 0xF000)>>8;
	memcpy( SignBuf, SIGN_NULL, 8 );
    
    temp = DispData_BCD + 4;
    if(D2 == 0x00)    //当前
        *SignBuf = Sign_dangqian;//当前
    else
    {
        *SignBuf = D2;
        *(SignBuf+1) = Sign_shangyue;
    }
    *(SignBuf+2) = Sign_kWh;//注意：Sign_dot2一定要在Sign_kWh之后的寄存器
    *(SignBuf+3) = Sign_dot2;
    *(SignBuf+4) = Sign_dianliang;
    switch( D1 )
    {
        case 0xC0://正向
            temp |= DispData_HEX;
            *(SignBuf+5) = Sign_Zhengxiang;//正向
            if(D2 == 0x00)  
                oad = 0x00100400;
            else 
            {
                Record_oad = 0x50050200;
                oad = 0x00100400;
            }
            break;
        case 0xD0://反向
            temp |= DispData_HEX;
            *(SignBuf+5) = Sign_Fanxiang;//反向
            if(D2 == 0x00)  
                oad = 0x00200400;
            else
            {
                Record_oad = 0x50050200;
                oad = 0x00200400;
            }
            break;
        case 0xE0://组合
            temp |= DispData_Sign+DispData_HEX;//组合有功电量最高位是符号位
            *(SignBuf+5) = Sign_Zuhe;//组合
            if(D2 == 0x00)  
                oad = 0x00000400;
            else
            {
                Record_oad = 0x50050200;
                oad = 0x00000400;
            }
            break;
        default:
            return 1;
            break;
    }
    if(D2 == 0x00)//当前
    {
        nd_get(instance, oad + D3, data, 9, NULL);
        ndv_move_buffer(data+1, data, 8);
    }
    else
    {   
        len = read_record_data_form_698_protocol(instance, data, Record_oad, oad + D3, D2);
        if(len == 10)   //电量10字节
        {
            ndv_move_buffer(data+2, data, 8);   //历史电量返回成功01+TAG+8字节数据
        }
        else
        {
            memset(data, 0x00, 8);
        }
    }
    
    display_energy_pro(instance, pipeline, data, D1); //电能小数尾数处理，借位处理
    
    *(SignBuf+6) = Sign_FeiLvNum;//T1~T12
    Info = temp;
	return Info;
}

/***********************************************************************/
/*  函数名称：Display_Meter                                            */
/*  函数功能：显示电表相关指示信息                                     */
/***********************************************************************/
void Display_Meter(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	if( mbase_control_block->Page_ID == 0xFFFF ) return;//当前全屏显，则返回
	
    if( mbase_control_block->key_state == true )   //秘钥状态，小房子显示	
        Lcdcon[18] |= DS_Chr_Lab;  
    if(( mbase_control_block->mac_suspend_state == true ) || (mbase_control_block->password_err_cnt >= 3))   //MAC锁
        Lcdcon[17] |= DS_Chr_Lock;      
	
	if( instance->ac_power_statue == AC_POWER_ON )
	{
		if( mbase_control_block->infrared_auth_timeout > 0 )//红外认证有效
            Lcdcon[17] |= DS_Chr_KeyProgram;
		//通讯指示
		if( mbase_control_block->commdisp_ir485 > 0 ) 
		{
			Lcdcon[18] |= DS_Chr_Comm;
		}
		
		if( mbase_control_block->commdisp_plc > 0 ) 
		{
			Lcdcon[19] |= DS_Chr_CommPLC;
		}

        if( mbase_control_block->power_dir == true )  //电流反向
        {
            Lcdcon[18] |= DS_Chr_Reverse;
            if((mbase_control_block->Page_ID == 0xA006) || (mbase_control_block->Page_ID == 0xA00A) || (mbase_control_block->Page_ID == 0xA00B) 
              || (mbase_control_block->Page_ID == 0xA012) || (mbase_control_block->Page_ID == 0xA013)) 
                Lcdcon[19] |= DS_Chr_Fu;
        }
        if(mbase_control_block->current_n < 0)
        {
            if(mbase_control_block->Page_ID == 0xA009)
            {
                Lcdcon[19] |= DS_Chr_Fu;
            }
        }
                
		if( mbase_control_block->voltage_battery < Bat_Low_Level )//电池欠压指示
            Lcdcon[18] |= DS_Chr_Bat;
		//密码验证闭锁指示
		//Lcdcon[17] |= DS_Chr_Lock;
		
		if( mbase_control_block->lcd_12_mean == true )
		{
			//费率电价主、副表指示
			if( mbase_control_block->TouPriceTable == true ) 
			{
				Lcdcon[15] |= DS_Chr_TouTable1;
				Lcdcon[14] |= DS_Chr_TouTable2;
			}
			else  
              Lcdcon[15] |= DS_Chr_TouTable1;
		}
		else
		{
			//日时段主、副表指示
			//if( mbase_control_block->DayTable == 0 ) 
              Lcdcon[15] |= DS_Chr_TouTable1;
			//else 
              //Lcdcon[14] |= DS_Chr_TouTable2;
            if( mbase_control_block->Disp12 == 1)
            {
                Lcdcon[15] |= DS_Chr_TouTable1;
                Lcdcon[14] |= DS_Chr_TouTable2;
            }
		}

		//分时费率指示
        Lcdcon[14] |= DS_Chr_T;
        if(mbase_control_block->TOUNo < 10)    Lcdcon[11] |= NumSeg[mbase_control_block->TOUNo];
        else
        {
            Lcdcon[13] |= DS_Chr_L18;
            Lcdcon[11] |= NumSeg[mbase_control_block->TOUNo-10];
        }
                    
		//闪烁
		if( (Flag.Disp & F_DispFlash) == 0 )
		{
			if( mbase_control_block->sta & F_RemoteAlarm )
				Lcdcon[12] |=  (DS_Chr_GouDian);
			if((mbase_control_block->alarmmin == 0) || (mbase_control_block->imaxmin == 0))//跳闸延时过程中指示(闪烁"拉闸)
				Lcdcon[13] |= DS_Chr_LaZha;
            if(mbase_control_block->factor_mode == true)//工厂模式
                Lcdcon[17] |= DS_Chr_KeyProgram;
            Flag.Disp |= F_DispFlash;
		}
		else
		{
			if( mbase_control_block->sta & F_RemoteAlarm )
				Lcdcon[12] &= ~(DS_Chr_GouDian);
			if((mbase_control_block->alarmmin == 0) || (mbase_control_block->imaxmin == 0))
				Lcdcon[13] &= ~DS_Chr_LaZha;
            if(mbase_control_block->factor_mode == true)
                Lcdcon[17] &= ~DS_Chr_KeyProgram;
            Flag.Disp &= ~F_DispFlash;
		}
		//继电器状态指示
//		if( (Flag.Relay & F_StateOff) && (Flag.Relay & F_RelayOff) 
//		&& (((Credit_Block.Credit.Amount+MeterPara_Block.MeterPara.OverdraftCredit) <= 0) || ( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.RelayOnCredit)&&(Flag.MeterState & F_RelayOffed) ) || (Flag.RemoteCmd & F_RemoteRelayOff))) 
		if(((mbase_control_block->sta & F_RemoteRelayOff) || (mbase_control_block->sta & F_RemoteGaoJing0) || (mbase_control_block->sta & F_RemoteGaoJing1))
           && ((mbase_control_block->sta & F_RemoteRelayOn) != F_RemoteRelayOn))    //拉闸
        {
            if((mbase_control_block->alarmmin == 0) && (mbase_control_block->imaxmin == 0))   Lcdcon[13] |= DS_Chr_LaZha;
        }
        else
        {
            Lcdcon[13] &= ~DS_Chr_LaZha;
        }
	}
} 

/***********************************************************************/
/*  函数名称：Disp_Value                                               */
/*  函数功能：显示数值                                                 */
/*  输入参数：Addr：数据地址                                           */
/*            Info：数据信息                                           */
/***********************************************************************/
void Disp_Value( unsigned short id, unsigned char *data, unsigned int Info )
{
	unsigned char Len,i;
	unsigned long long Tmp=0;
	unsigned char Sign = 0;
	Len = (unsigned char)(Info & DispDataLen);
//	MemType = (unsigned char)((Info>>6) & MEMVALUE);
	
	switch(id)
    {
        case 0xA001://日期
            for(i=0;i<3;i++)    Buffer[i] = BYTE_BCD(*(data+i));
            break;
        case 0xA002://时间
            for(i=0;i<3;i++)    Buffer[i] = BYTE_BCD(*(data+i));
            break;
        case 0xB001://表地址高4位
        case 0xB003://表号高4位
        case 0xB005://客户号高4位
            memcpy( (unsigned char*)&Buffer, (unsigned char*)(data+4+2), 2 );
            break;
        case 0xB002://表地址低8位
        case 0xB004://表号低8位
        case 0xB006://客户号低8位
            memcpy( (unsigned char*)&Buffer, (unsigned char*)(data+2), 4 );
            break;
        case 0xB007://脉冲常数
            memcpy( (unsigned char*)&Buffer, (unsigned char*)(data+1), 4 );
            break;
        case 0xA012:
        case 0xA013:
        case 0xA00B:
        case 0xA00A:
        case 0xA009:
        case 0xA006:
        case 0xA003:
            mem_swap_cpy( (unsigned char*)Buffer, (unsigned char*)data, Len );
            break;
        default:
            memcpy( (unsigned char*)Buffer, (unsigned char*)data, Len );
            break;
        
    }
    if( Info & DispData_HEX )
	{
        mem_swap_cpy( (unsigned char*)&Tmp, (unsigned char*)Buffer, Len );
        HEX64_BCD( Buffer, Tmp );
	}
	else
	{
/*		if(( Info & DispData_Sign ) && ( ((mbase_control_block->Page_ID & 0xFFFF0000) == 0x00000000)
		||((mbase_control_block->Page_ID & 0xFFFF0000) == 0x000B0000)||((mbase_control_block->Page_ID & 0xFFFF0000) == 0x000C0000)) )
		{
		  	if( Buffer[Len-1] & 0x80 )
			{
				Buffer[Len-1] &= 0x7f;
				Lcdcon[19] |= DS_Chr_Fu;
			}	     		
		}*/
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
			Lcdcon[19] |= DS_Chr_Fu;
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

	for( j=0;j<i;j++)
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
void Disp_Char( struct instance_t* instance, struct pipeline_t* pipeline, unsigned char* SignBuf )
{
	unsigned char j,EnergyDot = 2, PowerDot = 4,i=0;
	unsigned char ID1L;
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    EnergyDot = mbase_control_block->energy_point;
    PowerDot = mbase_control_block->power_point;
    
    if(mbase_control_block->disp_borrow != 0)   //借位
    {
        if(EnergyDot >= mbase_control_block->disp_borrow)
            EnergyDot = EnergyDot - mbase_control_block->disp_borrow;
    }
    
	ID1L = (unsigned char)(mbase_control_block->Page_ID & 0x000F);
    if(ID1L >= 1)    ID1L = ID1L - 1;
    
	for( j=0;j<9;j++ )
	{
		switch( (*SignBuf) )
		{
			case Sign_kWh:
				Lcdcon[14] |= DS_Chr_kW;
				Lcdcon[13] |= DS_Chr_h;
				switch( EnergyDot )//电能小数点
				{
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        //memset( Buffer, NumSeg[0], 8 );
						//memcpy( (unsigned char*)&Buffer[0], (unsigned char*)&Lcdcon[0], 8 );
/*                        for(i=0;i<8;i++)
                        {
                            Lcdcon[7-i] = Lcdcon[7-(4-EnergyDot)-i];
                        }*/
						if( EnergyDot == 0 )
						{
							*(SignBuf+1) = 0;
							ZeroDark(7);
						}
						else
							*(SignBuf+1) = Sign_dot1 + (EnergyDot-1);//注意：Sign_dot2一定要在Sign_kWh之后的寄存器		
                        break;
					default:
						break;
				}
				break;
			case Sign_kW:
				Lcdcon[14] |= DS_Chr_kW;
				switch( PowerDot )//功率小数点
				{
					case 0:
					case 1:
					case 2:
					case 3:
						memset( Buffer, NumSeg[0], 8 );
						memcpy( (unsigned char*)&Buffer[6-PowerDot], (unsigned char*)&Lcdcon[2], 2+PowerDot );
						memcpy( Lcdcon, Buffer, 8 );
						if( PowerDot == 0 )
						{
							*(SignBuf+1) = 0;
							ZeroDark(7);
						}
						else
							*(SignBuf+1) = Sign_dot1 + (PowerDot-1);//注意：Sign_dot2一定要在Sign_kWh之后的寄存器
						break;
					default:
						break;
				}
				break;
			case Sign_dot1:
				ZeroDark(6);
				Lcdcon[15] |= DS_Chr_Dot1;
				break;
			case Sign_dot2:
				ZeroDark(5);
				Lcdcon[15] |= DS_Chr_Dot2;
				break;
			case Sign_dot3:
				ZeroDark(4);
				Lcdcon[16] |= DS_Chr_Dot3;
				break;
			case Sign_dot4:
				ZeroDark(3);
				Lcdcon[12] |= DS_Chr_Dot4;
				break;
			case Sign_time:
				Lcdcon[16] |= DS_Chr_Dot22;
				Lcdcon[12] |= DS_Chr_Dot42;
			case Sign_date:
				Lcdcon[15] |= DS_Chr_Dot2;
				Lcdcon[12] |= DS_Chr_Dot4;
				break;
			case Sign_dangqian:
				Lcdcon[18] |= DS_Chr_DangQian;
				break;
			case Sign_shangyue://
				Lcdcon[18] |= DS_Chr_Shang;
				Lcdcon[17]|= DS_Chr_Yue;
				break;
			case Sign_dangqianyue:
				Lcdcon[18] |= DS_Chr_DangQian;
				Lcdcon[17]|= DS_Chr_Yue;
				break;
			case Sign_yue:
				Lcdcon[17]|= DS_Chr_Yue;
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
				Lcdcon[18] |= DS_Chr_P0;
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
				Lcdcon[8] |= NumSeg[*SignBuf];
				break;
			case Sign_FeiLvNum://费率号
				if(ID1L >= 10)
				{
                    ID1L = ID1L - 10;
					Lcdcon[13] |= DS_Chr_P2;
					Lcdcon[9] |= NumSeg[ID1L&0x0F];
					Lcdcon[13] |= DS_Chr_FeiLv;
				}
				else if(ID1L == 0)//总
				{
					Lcdcon[12] |= DS_Chr_Zong;
				}
				else
				{
					Lcdcon[9] |= NumSeg[ID1L];
					Lcdcon[13] |= DS_Chr_FeiLv;
				}
				break;
			case Sign_changshu://常数
				Lcdcon[14] |= DS_Chr_ChangShu;
                ZeroDark(4);
				break;
			case Sign_yong: //用
				Lcdcon[17] |= DS_Chr_Yong;
				break;
			case Sign_shi://时
				Lcdcon[13] |= DS_Chr_Shi;
				break;
			case Sign_jian2://间
				Lcdcon[16] |= DS_Chr_Jian;
				break;
			case Sign_duan://段
				Lcdcon[16] |= DS_Chr_Duan;
				break;
			case Sign_biaohao://表号
				Lcdcon[15] |= DS_Chr_Biao;
				Lcdcon[14] |= DS_Chr_Hao;
				break;
			case Sign_huhao://户号
				Lcdcon[13] |= DS_Chr_Hu;
				Lcdcon[14] |= DS_Chr_Hao;
				break;
			case Sign_Dianya://电压
				Lcdcon[14] |= DS_Chr_UV;
				break;
			case Sign_Dianliu://火线电流
				Lcdcon[19] |= DS_Chr_Lline;
				Lcdcon[14] |= DS_Chr_IA;
				break;
			case Sign_Gonglvyinsu://功率因数
				Lcdcon[14] |= DS_Chr_COS;
				break;
			case Sign_Linxiandianliu://零线电流
				Lcdcon[18] |= DS_Chr_Nline;
				Lcdcon[14] |= DS_Chr_IA;
				break;
			case Sign_Linxian://零线电流
				Lcdcon[18] |= DS_Chr_Nline;
				break;		
			case Sign_HuoXian://火线电流
				Lcdcon[19] |= DS_Chr_Lline;
				break;
			case Sign_Zuhe:
				//Lcdcon[20] |= DS_Chr_ZuHe;
				break;
			case Sign_Zhengxiang:
				Lcdcon[12] |= DS_Chr_Xiang;
				Lcdcon[17] |= DS_Chr_Zheng;
				break;
			case Sign_Fanxiang:
				Lcdcon[12] |= DS_Chr_Xiang;
				Lcdcon[16] |= DS_Chr_Fan;
				break;
			case Sign_doubledot:
				Lcdcon[16] |= DS_Chr_Dot22;
				Lcdcon[12] |= DS_Chr_Dot42;
				break;
/*			case Sign_Page_Sequ://时段表内容序号显示
				//Tmp = Disp.Page_Sequ;
				//BCDX_INC( (unsigned char*)&Tmp, 1 );//显示时从1到14序号
				Lcdcon[0] |= NumSeg[((Tmp)&0xF0)>>4];
				Lcdcon[1] |= NumSeg[(Tmp)&0x0F];
				break;*/
			case Sign_Jiesuanri:
				Lcdcon[15] |= DS_Chr_Dot2;
				break;
			case Sign_dianliang:
				Lcdcon[16] |= DS_Chr_Dian;//电
				Lcdcon[12] |= DS_Chr_Liang;//量
				break;
			case Sign_jieti://阶梯
				Lcdcon[17] |= DS_Chr_JieTi;
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
void Display_Page(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    unsigned char i,buf[40]={0,},Len=0,temp[7];
	unsigned long oad,Info=0,Info1;
	unsigned char SignBuf[9]={0,};
    signed long value=0;
    uint16_t year;
    struct idmatch
    {
        unsigned short id;
        unsigned char* addr;
    };
    struct idmatch idbuf[] = 
    {
        0xA001, (unsigned char*)&instance->time.tm_mday, //日期
        0xA002, (unsigned char*)&instance->time.tm_sec,  //时间
        0xA003, (unsigned char*)&mbase_control_block->voltage_ac, //电压
        0xA006, (unsigned char*)&mbase_control_block->current_l, //火线电流
        0xA009, (unsigned char*)&mbase_control_block->current_n, //零线电流
        0xA00A, (unsigned char*)&mbase_control_block->active_power, //A相功率
        0xA00B, (unsigned char*)&mbase_control_block->active_power, //A相功率
        0xA012, (unsigned char*)&mbase_control_block->power_factor, //A相功率因数
        0xA013, (unsigned char*)&mbase_control_block->power_factor, //A相功率因数
    };
    
    //mbase_control_block->Page_ID = 0xA002;
    
	if( mbase_control_block->Page_ID == 0xFFFF ) return;//当前全屏显,则返回
    Info1 = GetDispAddrInfo( instance, pipeline, mbase_control_block->Page_ID, buf, Info, SignBuf);
    if(Info1 == 1)
    {
        for( i=0;i<DisplayTabNum;i++ )
        {
            if(mbase_control_block->Page_ID == DisplayTab[i].Id) 
                break;
        }
        if( i >= DisplayTabNum )//如果显示项在表格里不存在,则显示第一项内容
            i = 0;
        oad = DisplayTab[i].Oad;
        Info1 = DisplayTab[i].Info;
        Len = (unsigned char)(Info1 & DispDataLen);
        if((Info1 & DispData_E2PROM) == DispData_E2PROM)
        {
            nd_get(instance, oad, buf, 10, NULL);
        }   
        else if((Info1 & DispData_RAM) == DispData_RAM)
        {
            if((DisplayTab[i].Id == 0xA001) || (DisplayTab[i].Id == 0xA002))
            {
                rtc_get_from_tm(instance, temp);
                if(DisplayTab[i].Id == 0xA001)
                {
                    memcpy((uint8_t*)&year, (uint8_t*)&temp[5],2);
                    buf[0] = (temp[3]);
                    buf[1] = (temp[4]);
                    buf[2] = (year-2000);
                }
                else
                {
                    buf[0] = (temp[0]);
                    buf[1] = (temp[1]);
                    buf[2] = (temp[2]);
                }
            }
            else
            {
                if((Info1 & DispData_Sign) == DispData_Sign)
                {
                    memcpy((unsigned char*)&value, (unsigned char*)idbuf[i].addr, 4);
                    if(value<0)
                    {
                        value = ~value + 1;
                    }
                    memcpy(buf, (unsigned char*)&value, Len);
                }
                else
                    memcpy(buf, (unsigned char*)idbuf[i].addr, Len);
            }
        }
        memcpy( SignBuf, (unsigned char*)DisplayTab[i].Sign, 9 );
    }
	//清屏
	ClearLCD();
	//显示数据
	Disp_Value( DisplayTab[i].Id, buf, Info1 );
	//显示字符和单位
	Disp_Char( instance, pipeline, SignBuf );
}

/***********************************************************************/
/*  函数名称：Display                                                  */
/*  函数功能：液晶显示                                                 */
/***********************************************************************/
void Display(struct instance_t* instance, struct pipeline_t* pipeline)
{
  
    struct mbase_control_block_t* mbase_control_block;
    mbase_control_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(instance->ac_power_statue != AC_POWER_ON)
    {
        if(ANA_GetStatus(ANA_STATUS_COMP1)==0)
        {
            memset((uint8_t *)&mbase_control_block->AutoPtr, 0x00, 10);
            mbase_control_block->Key &= ~F_KeyAwake;
            Flag.Disp |= (F_DispPause+F_DispStop+F_DispOff);
            return;
        }
    }
	//获取显示项ID
    //if(instance->ac_power_statue == AC_POWER_ON)
    {
        Get_DisplayPage(instance,  pipeline);
    }
	//获取显示项内容
	Display_Page( instance,  pipeline);
	//显示电表指示信息
	Display_Meter(instance,  pipeline);
	//送显示内容
	Send_DisplayData(instance,  pipeline);
}



