#include "components/core/types/utils.h"
#include <stddef.h>
#include "application/adapters/dl698/meter_base_view.h"
#include "domain/common/time_format.h"
#include "645id_table.h"
#include "targets/stategrid_meter/app/tariff/tariff_applications.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "port.h"


unsigned char Comm485Baut_Check( unsigned char Baut )
{
/*	if(Flag.Comm & F_Comming)
	{
		if((Timer.IRAuthen_Tmr == 0) && (Common.Factory != Factory_Mode))//����ͨ�������ں�����֤��Ч�ڼ���в�����
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
/*  �������ƣ�GetTotalDay                                              */
/*  �������ܣ����������ռ���������                                     */
/*  ���������Addr�����������ݵ�ַ                                     */
/*  ���������������                                                   */
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
/*  �������ƣ�GetWeek                                                  */
/*  �������ܣ����������ռ�����                                         */
/*  ���������Addr�����������ݵ�ַ                                     */
/*  �����������                                                       */
/***********************************************************************/
unsigned char GetWeek645( unsigned char* Addr )
{
	unsigned char Week;

	Week = (unsigned char)(( GetTotalDay645( Addr ) + 5 ) % 7);
	return Week;
}

/***********************************************************************/
/*  �������ƣ�GetMonMaxDay                                             */
/*  �������ܣ��������¼�������������                                 */
/*  ���������Addr���������ݵ�ַ                                       */
/*  ������������������                                               */
/***********************************************************************/
unsigned char GetMonMaxDay645( unsigned char* Addr )
{
	unsigned char MaxDay;

	MaxDay = ((*Addr == 2) && ((BCD_BYTE(*(Addr+1)) & 0x03) == 0)) ? 0x29 : BYTE_BCD( MonTab[BCD_BYTE(*Addr)] );
    return MaxDay;
}

/***********************************************************************/
/*  �������ƣ�Date_Format_Check                                        */
/*  �������ܣ��ж��������ݸ�ʽ�Ƿ���ȷ                                 */
/*  ���������Addr���������ݵ�ַ                                       */
/*  ���������0Ϊ��ȷ������Ϊ���� 									   */
/***********************************************************************/
unsigned char Date_645Format_Check( unsigned char* Addr )
{
    if( (*Addr > 0x06) || ((*(Addr+1) < 0x01) || (*(Addr+1) > 0x31)) || ((*(Addr+2) < 0x01) || (*(Addr+2) > 0x12)) )
	    return RS_ERR_OTHERS;
    if( *(Addr+1) > GetMonMaxDay645( Addr+2 ) )//��С���ж�
  	    return RS_ERR_OTHERS;

    return 0;
}

/***********************************************************************/
/*  �������ƣ�Time_Format_Check                                        */
/*  �������ܣ��ж�ʱ�����ݸ�ʽ�Ƿ���ȷ                                 */
/*  ���������Addr��ʱ�����ݵ�ַ                                       */
/*  ���������0Ϊ��ȷ������Ϊ���� 									   */
/***********************************************************************/
unsigned char Time_645Format_Check( unsigned char* Addr )
{
    if( (*Addr > 0x59) || (*(Addr+1) > 0x59) || (*(Addr+2) > 0x23) )
	    return RS_ERR_OTHERS;
    else
   	    return 0;
}

/***********************************************************************/
/*  �������ƣ�SeasonTab_Format_Check                                   */
/*  �������ܣ��ж�ʱ�������ݸ�ʽ�Ƿ���ȷ                               */
/*  ���������Addr��ʱ�������ݵ�ַ                                     */
/*            Len��д��E�������ݳ���                                   */
/*  ���������0Ϊ��ȷ������Ϊ���� 									   */
/***********************************************************************/
unsigned char SeasonTab_Format_Check( void *instance, unsigned char* Addr, unsigned char* Len )
{
    unsigned char i,Num,Sign,buf[5]={0,};
    unsigned int Int1,Int2;
    uint32_t s_index;

    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);//��ȡʱ��������
    nd_get_index(instance, s_index, buf, 5);

	if( buf[1] > MaxTouTabNum ) buf[1] = MaxTouTabNum;
    Num = (*Len)/3;
    Sign = 0;
    for( i=0;i<Num;i++ )
    {
     	if( (*(Addr+3*i) == 0) || (*(Addr+3*i) > buf[1]) || (*(Addr+3*i+1) < 0x01) || (*(Addr+3*i+1) > 0x31) || (*(Addr+3*i+2) < 0x01) || (*(Addr+3*i+2) > 0x12) )
	    	return RS_ERR_OTHERS;

		//ʱ�������������ж�
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
/*  �������ƣ�TouTab_Format_Check                                      */
/*  �������ܣ��ж���ʱ�α����ݸ�ʽ�Ƿ���ȷ                             */
/*  ���������Addr����ʱ�α����ݵ�ַ                                   */
/*            Len��д��E�������ݳ���                                   */
/*  ���������0Ϊ��ȷ������Ϊ���� 									   */
/***********************************************************************/
unsigned char TouTab_Format_Check( void *instance, unsigned char* Addr, unsigned char* Len )
{
	unsigned char i,Num,Sign,Tmp,buf[5]={0,};
	unsigned int Int1,Int2;
    uint32_t s_index=0;

    s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);//��ȡʱ��������
    nd_get_index(instance, s_index, buf, 5);

   	if( buf[3] > MaxFeeNum ) buf[3] = MaxFeeNum;
	Num = (*Len)/3;
	Sign = 0;
	for( i=0;i<Num;i++ )
	{
		if( *(Addr+3*i) > 0x32 )//��������
			return RS_ERR_FEEOV;
		else if( (*(Addr+3*i) == 0) || (*(Addr+3*i+1) > 0x59) || (*(Addr+3*i+2) > 0x23) )
			return RS_ERR_OTHERS;

		//��ʱ�α����������ж�
		Int1 = 60*(unsigned int)BCD_BYTE(Addr[3*i+2]) + (unsigned int)BCD_BYTE(Addr[3*i+1]);
	 	Int2 = 60*(unsigned int)BCD_BYTE(Addr[3*((i+1)%Num)+2]) + (unsigned int)BCD_BYTE(Addr[3*((i+1)%Num)+1]);
		if( Int2 > Int1 )
        {
        	if( (Int2 - Int1) < 15 )
				return RS_ERR_OTHERS;//��ʱ�α�ʱ����Ҫ�����15����
		}
		else if( Int2 < Int1 )
		{
			if( Sign != 0x33 )
			{
			    Sign = 0x33;
			    if( (Int1 - Int2) > 1425 ) return RS_ERR_OTHERS;//��ʱ�α�ʱ����Ҫ�����15����
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
/*  �������ƣ�ChangeTime_Format_Check                                  */
/*  �������ܣ��ж��л�ʱ�����ݸ�ʽ�Ƿ���ȷ                             */
/*  ���������Addr���л�ʱ�����ݵ�ַ                                   */
/*  ���������0Ϊ��ȷ������Ϊ���� 									   */
/***********************************************************************/
unsigned char ChangeTime_Format_Check( unsigned char* Addr )
{
	if( 1 == BCD_Check(Addr,5) )
	    return RS_ERR_OTHERS;
	if( (*Addr > 0x59) || (*(Addr+1) > 0x23) )//ʱ����
	    return RS_ERR_OTHERS;
	if( ((*(Addr+2) < 0x01) || (*(Addr+2) > 0x31)) || ((*(Addr+3) < 0x01) || (*(Addr+3) > 0x12)) )//�ա���
	    return RS_ERR_OTHERS;
	if( *(Addr+2) > GetMonMaxDay645( Addr+3 ) )//��С���ж�
  	    return RS_ERR_OTHERS;

	return 0;
}


