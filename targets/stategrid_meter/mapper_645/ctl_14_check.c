#include "nidavellir.h"
#include "645id_table.h"
#include "domain/tariff/legacy/tariff_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "port.h"

extern const uint8_t  MonTab[13];
extern const unsigned int  DayTab[13];

unsigned char Comm485Baut_Check( unsigned char Baut )
{
/*	if(Flag.Comm & F_Comming)
	{
		if((Timer.IRAuthen_Tmr == 0) && (Common.Factory != Factory_Mode))//红外通道必须在红外认证有效期间进行操作。
		{
			return RS_ERR_PASS;
		}
	}*/
	switch( Baut )
	{
		case 0x02:
		case 0x04:
		case 0x08:
		case 0x10:
		case 0x20:
		case 0x40:
				return 0;
				break;
		default:
				return RS_ERR_BAUT;
				break;
	}
}

/***********************************************************************/
/*  函数名称：GetTotalDay                                              */
/*  函数功能：根据年月日计算总天数                                     */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：总天数                                                   */
/***********************************************************************/
unsigned int GetTotalDay645( unsigned char* Addr )
{
  	unsigned int Day,Mon,Year;

	Year= BCD_BYTE( *(Addr+2) );
	Mon = BCD_BYTE( *(Addr+1) );
	Day = BCD_BYTE( *Addr );
	Day = Day + DayTab[Mon] + Year*365 + (Year>>2) + 1;
	if( ((Year&0x03) == 0) && (Mon <= 2) )
		Day--;
   	return Day;
}
/***********************************************************************/
/*  函数名称：GetWeek                                                  */
/*  函数功能：根据年月日计算周                                         */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：周                                                       */
/***********************************************************************/
unsigned char GetWeek645( unsigned char* Addr )
{
	unsigned char Week;

	Week = (unsigned char)(( GetTotalDay645( Addr ) + 5 ) % 7);
	return Week;
}

/***********************************************************************/
/*  函数名称：GetMonMaxDay                                             */
/*  函数功能：根据年月计算该月最大天数                                 */
/*  输入参数：Addr：年月数据地址                                       */
/*  输出参数：月最大天数                                               */
/***********************************************************************/
unsigned char GetMonMaxDay645( unsigned char* Addr )
{
	unsigned char MaxDay;

	MaxDay = ((*Addr == 2) && ((BCD_BYTE(*(Addr+1)) & 0x03) == 0)) ? 0x29 : BYTE_BCD( MonTab[BCD_BYTE(*Addr)] );
    return MaxDay;
}

/***********************************************************************/
/*  函数名称：Date_Format_Check                                        */
/*  函数功能：判断日期数据格式是否正确                                 */
/*  输入参数：Addr：日期数据地址                                       */
/*  输出参数：0为正确，其余为错误 									   */
/***********************************************************************/
unsigned char Date_645Format_Check( unsigned char* Addr )
{
    if( (*Addr > 0x06) || ((*(Addr+1) < 0x01) || (*(Addr+1) > 0x31)) || ((*(Addr+2) < 0x01) || (*(Addr+2) > 0x12)) )
	    return RS_ERR_OTHERS;
    if( *(Addr+1) > GetMonMaxDay645( Addr+2 ) )//大小月判断
  	    return RS_ERR_OTHERS;

    return 0;
}

/***********************************************************************/
/*  函数名称：Time_Format_Check                                        */
/*  函数功能：判断时间数据格式是否正确                                 */
/*  输入参数：Addr：时间数据地址                                       */
/*  输出参数：0为正确，其余为错误 									   */
/***********************************************************************/
unsigned char Time_645Format_Check( unsigned char* Addr )
{
    if( (*Addr > 0x59) || (*(Addr+1) > 0x59) || (*(Addr+2) > 0x23) )
	    return RS_ERR_OTHERS;
    else
   	    return 0;
}

/***********************************************************************/
/*  函数名称：SeasonTab_Format_Check                                   */
/*  函数功能：判断时区表数据格式是否正确                               */
/*  输入参数：Addr：时区表数据地址                                     */
/*            Len：写入E方的数据长度                                   */
/*  输出参数：0为正确，其余为错误 									   */
/***********************************************************************/
unsigned char SeasonTab_Format_Check( struct instance_t* instance, unsigned char* Addr, unsigned char* Len )
{
    unsigned char i,Num,Sign,buf[5]={0,};
    unsigned int Int1,Int2;
    uint32_t s_index;

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);//读取时区表个数
    nd_get_index(instance, s_index, buf, 5);

	if( buf[1] > MaxTouTabNum ) buf[1] = MaxTouTabNum;
    Num = (*Len)/3;
    Sign = 0;
    for( i=0;i<Num;i++ )
    {
     	if( (*(Addr+3*i) == 0) || (*(Addr+3*i) > buf[1]) || (*(Addr+3*i+1) < 0x01) || (*(Addr+3*i+1) > 0x31) || (*(Addr+3*i+2) < 0x01) || (*(Addr+3*i+2) > 0x12) )
	    	return RS_ERR_OTHERS;

		//时区表数据内容判断
		Int1 = *(Addr+3*i+2);
        Int1 <<= 8; 
        Int1 += *(Addr+3*i+1);
		Int2 = *(Addr+3*((i+1)%Num)+2);
        Int2 <<= 8; 
        Int2 += *(Addr+3*((i+1)%Num)+1);
		if( Int2 > Int1 ) continue;
		else if( Int2 < Int1 )
		{
			if( Sign != 0x33 )
				Sign = 0x33;
			else
				return RS_ERR_OTHERS;
		}
		else if( Int2 == Int1 )
		{
			if( *(Addr+3*i+0) != *(Addr+3*((i+1)%Num)+0) )
				return RS_ERR_OTHERS;
		}
    }
	for(i=0;i<(MaxSeasonNum-Num);i++)
		memcpy((unsigned char*)(Addr+(3*Num)+3*i),(unsigned char*)(Addr+(3*(Num-1))),3);
	(*Len) = 3*MaxSeasonNum;
    return 0;
}

/***********************************************************************/
/*  函数名称：TouTab_Format_Check                                      */
/*  函数功能：判断日时段表数据格式是否正确                             */
/*  输入参数：Addr：日时段表数据地址                                   */
/*            Len：写入E方的数据长度                                   */
/*  输出参数：0为正确，其余为错误 									   */
/***********************************************************************/
unsigned char TouTab_Format_Check( struct instance_t* instance, unsigned char* Addr, unsigned char* Len )
{
	unsigned char i,Num,Sign,Tmp,buf[5]={0,};
	unsigned int Int1,Int2;
    uint32_t s_index=0;

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);//读取时区表个数
    nd_get_index(instance, s_index, buf, 5);

   	if( buf[3] > MaxFeeNum ) buf[3] = MaxFeeNum;
	Num = (*Len)/3;
	Sign = 0;
	for( i=0;i<Num;i++ )
	{
		if( *(Addr+3*i) > 0x32 )//费率数超
			return RS_ERR_FEEOV;
		else if( (*(Addr+3*i) == 0) || (*(Addr+3*i+1) > 0x59) || (*(Addr+3*i+2) > 0x23) )
			return RS_ERR_OTHERS;

		//日时段表数据内容判断
		Int1 = 60*(unsigned int)BCD_BYTE(Addr[3*i+2]) + (unsigned int)BCD_BYTE(Addr[3*i+1]);
	 	Int2 = 60*(unsigned int)BCD_BYTE(Addr[3*((i+1)%Num)+2]) + (unsigned int)BCD_BYTE(Addr[3*((i+1)%Num)+1]);
		if( Int2 > Int1 )
        {
        	if( (Int2 - Int1) < 15 )
				return RS_ERR_OTHERS;//日时段表时间间隔要求大于15分钟
		}
		else if( Int2 < Int1 )
		{
			if( Sign != 0x33 )
			{
			    Sign = 0x33;
			    if( (Int1 - Int2) > 1425 ) return RS_ERR_OTHERS;//日时段表时间间隔要求大于15分钟
		    }
		    else
				return RS_ERR_OTHERS;
		}
		else if( Int2 == Int1 )
		{
			if( *(Addr+3*i+0) != *(Addr+3*((i+1)%Num)+0) )
				return RS_ERR_OTHERS;
		}
	}
	for(i=0;i<(MaxTouNum-Num);i++)
		memcpy((unsigned char*)(Addr+(3*Num)+3*i),(unsigned char*)(Addr+(3*(Num-1))),3);
	(*Len) = 3*MaxTouNum;
    return 0;
}

/***********************************************************************/
/*  函数名称：ChangeTime_Format_Check                                  */
/*  函数功能：判断切换时间数据格式是否正确                             */
/*  输入参数：Addr：切换时间数据地址                                   */
/*  输出参数：0为正确，其余为错误 									   */
/***********************************************************************/
unsigned char ChangeTime_Format_Check( unsigned char* Addr )
{
	if( 1 == BCD_Check(Addr,5) )
	    return RS_ERR_OTHERS;
	if( (*Addr > 0x59) || (*(Addr+1) > 0x23) )//时、分
	    return RS_ERR_OTHERS;
	if( ((*(Addr+2) < 0x01) || (*(Addr+2) > 0x31)) || ((*(Addr+3) < 0x01) || (*(Addr+3) > 0x12)) )//日、月
	    return RS_ERR_OTHERS;
	if( *(Addr+2) > GetMonMaxDay645( Addr+3 ) )//大小月判断
  	    return RS_ERR_OTHERS;

	return 0;
}


