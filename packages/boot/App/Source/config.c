#include "Includes.H"
#include "string.h"
#include "target.h"


#define nop()		__NOP();
// 波特率
#define BAUD_RS485 115200
#define BAUD_PLC 115200
#define BAUD_IR 1200

unsigned char I2C_Buffer[41];

void HBF_LOOP_LBF( unsigned char* Dest, unsigned char* Src, unsigned char Len )
{
    unsigned char i;

    for( i=0;i<Len;i++ )
            *(Dest+i) = *(Src+Len-1-i);
}

void Delay_10uS( unsigned char n )
{
	unsigned char j,i;
	for(j=0;j<n;j++)
	{
		for(i=0;i<10;i++)
		{	nop();
			nop();
			nop();
			nop();
			nop();
			nop();
			nop();
			nop();
			nop();
			nop();
		}
	}
}
/***********************************************************************/
/*  函数名称：Delay_Xms                                                */
/*  函数功能：延时Xms子程序                                            */
/*  输入参数：X：延时时间，单位ms                                      */
/***********************************************************************/
void Delay_Xms(unsigned char X)
{
    unsigned char i;

    for( i=0;i<X;i++ )
    {
        Delay_10uS(116);
    }
}
/***********************************************************************/
/*  函数名称：Uart5Init                                                */
/*  函数功能：串口5初始化配置(红外)                                     */
/*  输入参数：Mode：复位、下电、上电三种状态                           */
/***********************************************************************/
void Uart5Init(void)
{
    unsigned long	ClkDiv;
    
    GPIOB->OEN &= ~BIT7;    //TX输出
    GPIOB->IE &= ~BIT7;    //TX禁止输入
    GPIOB->OEN |= BIT1;    //RX禁止输出
    GPIOB->IE |= BIT1;    //TX使能输入
        
    ClkDiv = CLK_GetPCLKFreq();
    ClkDiv=ClkDiv/BAUD_IR;
    
    UART5->BAUDDIV = ClkDiv;
    UART5->CTRL2 = 0x00000002;
    UART5->STATE = 0x3C;
	MISC2->PCLKEN |= MISC2_PCLKEN_MISC;
	MISC->IREN = 0x20;
	MISC->DUTYH = 344/2;
	MISC->DUTYL = 344/2;
    
    UART5->CTRL |= BIT1;
}
/***********************************************************************/
/*  函数名称：Uart1Init                                                */
/*  函数功能：串口1初始化配置(蓝牙)                                     */
/*  输入参数：Mode：复位、下电、上电三种状态                           */
/***********************************************************************/
void Uart4Init(void)
{
    unsigned long	ClkDiv;
    
    GPIOB->ATT |= BIT0;//plcrx
    GPIOB->ATT |= BIT6;//plctx
    GPIOB->OEN &= ~BIT6;    //TX输出
    GPIOB->IE &= ~BIT6;    //TX禁止输入
    GPIOB->OEN |= BIT0;    //RX禁止输出
    GPIOB->IE |= BIT0;    //TX使能输入
        
    ClkDiv = CLK_GetPCLKFreq();
    ClkDiv=ClkDiv/BAUD_PLC;
    
    UART4->BAUDDIV = ClkDiv;
    UART4->CTRL2 = 0x00000002;
    UART4->STATE = 0x3C;
    UART4->CTRL |= BIT1;
}
/***********************************************************************/
/*  函数名称：Uart0Init                                                */
/*  函数功能：串口0初始化配置(485)                                     */
/*  输入参数：Mode：复位、下电、上电三种状态                           */
/***********************************************************************/
void Uart0Init(void)
{
    unsigned long	ClkDiv;
    
    GPIOB->OEN &= ~BIT2;    //TX输出
    GPIOB->IE &= ~BIT2;    //TX禁止输入
    GPIOA->OEN |= BIT12;    //RX禁止输出
    GPIOA->IE |= BIT12;    //TX使能输入
    GPIOA->OEN &= ~BIT7;    //active输出
    GPIOA->IE &= ~BIT7;
    GPIOA->DAT |= BIT7;
    
    ClkDiv = CLK_GetPCLKFreq();
    ClkDiv=ClkDiv/BAUD_RS485;

    //UART0用于RS485口1通讯.
    UART0->BAUDDIV = ClkDiv;
    UART0->CTRL2 = 0x00000002;
    UART0->STATE = 0x3C;
    UART0->CTRL |= BIT1;
}
