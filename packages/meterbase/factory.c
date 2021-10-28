#include "kernel\nidavellir.h"
#include "kernel\task.h"
#include "applications.h"
#include "pin_function.h"

#define addrstart	0x00000000
#define	addrend		0x00010000
#define	infactory	0xAA
#define	F_ALadj		0x01		//火线校准
#define	F_ANadj		0x08		//零线校准


const uint8 CODE *soft_version = "GW20DDZYXXXV01(211016_XX:XX)\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

//	1.掉电保存，且上电需要恢复的数据；			如：电量
//	2.上电需要恢复的数据；						如：参数
//	3.上电需要清零的数据，运行过程中会刷新

typedef struct 
{
	unsigned char	mode;	    //厂内模式：0xAA，厂外模式：其他
	unsigned char	hour;		//工厂模式时间，上电时间3天后自动退出
}FACTORY2;

typedef struct
{
	unsigned char	adjen;	//校表使能	0xAA
	unsigned char	adjsec;	//校表倒计时 2秒后校表
	unsigned char	adjflag;	//校表标志位
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
}FACTORY3;

extern FACTORY2	Factory2;
extern FACTORY3	Factory3;


/** 计算程序校验码 协议调用*/
unsigned short checksum_code(void)
{
	unsigned short check_sum = 0;
	unsigned long addr;

    for (addr = addrstart; addr < addrend; addr++)
	{
		check_sum += *addr;
		if (0x0000UL == ((unsigned long)addr % 0x100))
		{
			clear_watchdog();//清看门狗
		}
	}
	return check_sum;
}

//协议调用，返回0成功，1失败。
unsigned char in_factory(unsigned char mode)
{
	unsigned char result=0;
	
	if(mode == infactory)	//进厂内模式
	{
		if()//开盖状态
		{
			Factory2.mode = infactory;
		}
		else
		{
			result = 1;
		}
	}
	else
	{
		Factory2.mode = 0;
	}
	return result;
}

void meterchip_adj_self(void)
{
    unsigned char i=0, Tofst;
	signed short Pgain;
    unsigned long TempUrms=0, TempIrms=0, TempPw=0, CalcPw=0, Corretion_Pw=0, U_value=0, I1_value=0, I2_value=0, P_value=0;
	unsigned long BaseErr,hfconst,RN8209Kpq;
	float err=0, Ku, Ka, Kb, err;

    memset((unsigned char*)Buffer,0x00,16);

    //E2P_Read((unsigned char*)&hfconst, Meter_VHFconst, 4 );	//获取脉冲速率
    //E2P_Read((unsigned char*)&BaseErr, MeterBaseErr, 4);	//获取基准偏差值

    if (Factory3.adjflag & F_ALadj)
    {
        for (i = 0; i < 3; i++)
        {
    		//获取电压
    		//RN8209C_Read( (unsigned char*)&U_value, URMS, 3 );
    		if( U_value & 0x00800000 )//电压最高位为1时，数据无效
    			U_value = 0;
    		else
    			U_value &= 0x00FFFFFF;
    		TempUrms += U_value;

    		//获取电流
    		//RN8209C_Read( (unsigned char*)&I_value, IARMS, 3 );
    		if( I1_value & 0x00800000 )//电流最高位为1时，数据无效
    			I1_value = 0;
    		else
    			I1_value &= 0x00FFFFFF;
    		TempIrms += I1_value;

    		//获取功率
    		//RN8209C_Read( (unsigned char*)&P_value, PowerPA, 4 );
    		if(P_value >= 0x80000000)
    			P_value=~P_value;
            TempPw += P_value;

    		Delay_Xms(500);	//等待500ms
    		WatchDog_Clear;//清看门狗
        }
        TempUrms = TempUrms/3;
        TempIrms = TempIrms/3;
        TempPw   = TempPw/3;

        if (memcmp( &Factory3.Ua, Buffer, 12 ) != 0)
        {
            Corretion_Pw = (unsigned long)(((float)Factory3.Ua*(float)Factory3.Ia/1000000)/RN8209Kpq);//台体视在功率
            CalcPw = (unsigned long)(((float)TempUrms * (float)TempIrms)/32768); //表视在功率

     		Ku = (float)Factory3.Ua/TempUrms/10;
        	Ka = (float)Factory3.Ia/10/TempIrms;

            err = (CalcPw - (float)Corretion_Pw)/(float)Corretion_Pw;
            err = err + BaseErr*0.01;
            err = (-err/(1+err));
            if (err >= 0)
            {
                Pgain = (signed short)(err * 32768);
            }
            else
            {
                Pgain = (signed short)(65535 + err * 32768);
            }

            //MEM_DBWrite( ADJ_Pgain0, (unsigned char*)&Adj_Block[0].Adj.Pgain, 2, Mem_E2PROM );
            //MEM_DBWrite( Meter_Ku, (unsigned char*)&Fact.Ku, 8, Mem_E2PROM );
            //RN8209C_Write(GPQA, (unsigned char*)&Adj_Block[0].Adj.Pgain, 2);

            CalcPw = (unsigned long)((float)TempPw*(1+err));
            Corretion_Pw = (unsigned long)(((float)Factory3.Pa/100)/RN8209Kpq);
            err = ((float)CalcPw - Corretion_Pw)/(Corretion_Pw);
            err = (asin(-err/1.732))*100*57.29578;
            if (err >= 0)
        	{
        		Tofst = (unsigned char)((unsigned int)(err)>>1);
        	}
        	else
        	{
        		Tofst = (unsigned char)(((unsigned int)(512+err))>>1);
        	}
            //RN8209C_Write(PhsA, (unsigned char*)&Adj_Block[0].Adj.Tofst, 1);
            //MEM_DBWrite( ADJ_Tofst0, (unsigned char*)&Adj_Block[0].Adj.Tofst, 1, Mem_E2PROM );
        	//RN8209CInit();
        }
    }
	if (Factory3.AWFlag & F_ANadj)
	{
		if (memcmp( &Factory3.Ian, Buffer, 4 ) != 0)
		{
			//RN8209C_Read( (unsigned char*)&I2_value, IBRMS, 3 );
			Kb = (float)Factory3.Ian/10/I2_value;
			//MEM_DBWrite( Meter_Kb, (unsigned char*)&Fact.Kb, 4, Mem_E2PROM );
			//RN8209CInit();
		}
	}
}

//定时任务调用,定时器
void factory_task(void)
{
	if(Factory2.hour > 0)		//上电3天后，退出工厂模式
	{
		Factory2.hour--;
		if(Factory2.hour == 0)	
		{
			Factory2.mode = 0;	
		}
	}
	
	if(Factory3.adjen == 0xAA)	//自动校表使能后，2秒后校表
	{
		if(Factory3.adjsec > 0)
		{
			Factory3.adjsec --;
			if(Factory3.adjsec == 0)
			{
				meterchip_adj_self();	//自动校表
			}
		}
	}
}

//协议调用
unsigned short board_check(void)
{
	unsigned short checkbyte = 0;
	
	if()//开盖键检测
	{
		checkbyte |= 0x0001;
	}
	else
	{
		checkbyte &= ~0x0001;
	}
	
	if()//按键检测
	{
		checkbyte |= 0x0002;
	}
	else
	{
		checkbyte &= ~0x0002;
	}
	
	if( == 0x820900)//计量芯片
	{
		checkbyte &= ~0x0004;
	}
	else
	{
		checkbyte |= 0x0004;
	}
	
	if()//电池欠压
	{
		checkbyte |= 0x0008;
	}
	else
	{
		checkbyte &= ~0x0008;
	}
	
	if()//EEPROM
	{
		checkbyte |= 0x0010;
	}
	else
	{
		checkbyte &= ~0x0010;
	}
	
	if()//FLASH
	{
		checkbyte |= 0x0020;
	}
	else
	{
		checkbyte &= ~0x0020;
	}
	
	if()//ESAM
	{
		checkbyte |= 0x0040;
	}
	else
	{
		checkbyte &= ~0x0040;
	}
	
	if()//继电器检测口
	{
		checkbyte |= 0x0080;
	}
	else
	{
		checkbyte &= ~0x0080;
	}
	
	return checkbyte;
}


