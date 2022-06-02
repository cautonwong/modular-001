
#include "nidavellir.h"
#include "applications.h"
#include "fr_698.h"
//dependencies
    //1.user define
    //2.mesg[秒]

uint8_t Buf[60];

const unsigned char  MonTab[13] = {	0,31,28,31,30,31,30,31,31,30,31,30,31};
/*
const unsigned int  DayTab[13] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

unsigned char BYTE_BCD( unsigned char HEX )
{
	if( HEX > 99 )
		return 0x00;

	return( (HEX/10)*0x10 + HEX%10 );
}


unsigned char BCD_BYTE( unsigned char BCD )
{
     return( ((BCD>>4) & 0x0F)*10 + (BCD & 0x0F) );
}

unsigned int GetTotalDay( unsigned char* Addr )
{
  	unsigned int Day,Mon,Year;

	Year= BCD_BYTE( *(Addr+2) );
	Mon = BCD_BYTE( *(Addr+1) );
	Day = BCD_BYTE( *Addr );
	Day = Day + DayTab[Mon] + Year*365 + (Year>>2) + 1;
	if( ((Year&0x03) == 0) && (Mon <= 2) )
		Day--;
   	return Day;
}*/


unsigned long GetTotalMin( unsigned char* Addr )
{
    //min,hour,day,mon,year
	unsigned long Min,Hour,Day;

	Day = GetTotalDay( Addr+2 );
	Hour =  *(Addr+1);
	Min =  *Addr ;
	Min = Min + Hour*60 + Day*60*24;
	return Min;
}

unsigned char GetMonMaxDay( unsigned char* Addr )
{
	unsigned char MaxDay;

	MaxDay = ((*Addr == 2) && (((*(Addr+1)) & 0x03) == 0)) ? 29 : ( MonTab[*Addr] );
    return MaxDay;
}

unsigned char Date_Format_Check( unsigned char* Addr )
{
    if( ((*(Addr) < 1) || (*(Addr) > 31)) || ((*(Addr+1) < 1) || (*(Addr+1) > 12)) )//日、月
	    return 1;
	if( *(Addr) > GetMonMaxDay( Addr+1 ) )//大小月判断
  	    return 1;
    return 0;
}

unsigned char Time_Format_Check( unsigned char* Addr )
{
	if( (*Addr > 23) || (*(Addr+1) > 59) || (*(Addr+2) > 59))//时、分/秒
	    return 1;
	return 0;
}

void FeeTable_Change(struct instance_t* instance, struct pipeline_t* pipeline)
{
	unsigned char i,*ptr;
	unsigned long CurrentTime,ChangeTime,addr=0;
    unsigned int year;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	ptr = (unsigned char*)&mbase_cb->SeasonTable;
    Buf[0] = instance->time.tm_min; //min,hour,day,mon,year
    Buf[1] = instance->time.tm_hour;
    Buf[2] = instance->time.tm_mday;
    Buf[3] = instance->time.tm_mon+1;
    Buf[4] = (unsigned char)(instance->time.tm_year+1900 - 2000);
	CurrentTime = GetTotalMin( Buf );

	for( i=0;i<2;i++ )
	{
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonTable_Changetime + i);
        nd_get_index(instance, addr, Buf, 7); 
        Buf[6] = 0;
        year = ((uint16_t)Buf[0]<<8) + Buf[1];
        Buf[1] = (uint8_t)(year-2000);
        
        mem_swap_cpy((uint8_t*)&Buf[7], (uint8_t*)&Buf[1], 5);
		ChangeTime = GetTotalMin( (uint8_t*)&Buf[7] );//min,hour,day,mon,year

        mbase_cb->Disp12 = 0;
		if(( Time_Format_Check( (uint8_t*)&Buf[4] ) ) || (Date_Format_Check((uint8_t*)&Buf[2])))//切换时间合法性判断
			continue;
		if( CurrentTime >= ChangeTime )
		{
			switch( i )
			{
				case 0:
					send_event_mesg_pro(instance, NULL, SeasonTabCutFreeze, NULL, 1);//时区表切换冻结
					mbase_cb->Fee |= (F_TOUChange+F_DAYTABChange);
					break;
				case 1:
					send_event_mesg_pro(instance, NULL, DayTabCutFreeze, NULL, 1);//日时段表切换冻结
					mbase_cb->Fee |= F_TOUChange;
					break;
                default:
					break;
			}
			if( *(ptr+i) == 0 )
				*(ptr+i) = 1;
			else
				*(ptr+i) = 0;
            
            if(i == 0)
            {
                addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                            offsetof(struct mbase_control_block_t, SeasonTable));
                nd_set_offset(instance, addr, (uint8_t*)&mbase_cb->SeasonTable, 1);//存储至EE
            }
            else
            {
                addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                            offsetof(struct mbase_control_block_t, DayTable));
                nd_set_offset(instance, addr, (uint8_t*)&mbase_cb->DayTable, 1);//存储至EE
            }
            
			memset( Buf, 0x00, 6 );
            Buf[6] = 0xFF;
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonTable_Changetime + i);
            nd_set_index(instance, addr, Buf, 7); 
		}
        else
        {
            if(i == 1)  
            {   
                mbase_cb->Disp12 = 1;    //时段表切换时间是未来时
            }
        }
	}
}


unsigned char FeeTable_CmpFind( struct instance_t* instance, struct pipeline_t* pipeline, unsigned char Num, unsigned char Table )
{
	unsigned char i;
	unsigned int Now,Int,Int1;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	//获取当前时间
	if( Table == 0 )
		Now = ((unsigned int)instance->time.tm_mon<<8) + (unsigned int)instance->time.tm_mday;
	else 
		Now = ((unsigned int)instance->time.tm_hour<<8) + (unsigned int)instance->time.tm_min;

	//比较查找
	for( i=0;i<Num;i++ )
	{
		Int = ((unsigned int)Buf[3*i]<<8) + (unsigned int)Buf[3*i+1];
		Int1 = ((unsigned int)Buf[3*((i+1)%Num)]<<8) + (unsigned int)Buf[3*((i+1)%Num)];
		if( Int < Int1 )
        {
            if( (Now >= Int) && (Now < Int1) )
                break;
        }
        else if( Int == Int1 )
        {
            if( Now >= Int )
                break;
        }
        else
        {
            if( (Now >= Int) || (Now < Int1) )//过0点判断
                break;
        }
	}
	if( i >= Num)
		i = Num - 1;	
	//时段投切信号
	if( Table == 1 )
	{
		if( Num == 1 )	
			mbase_cb->TouBefNo = Buf[2];
		else
			mbase_cb->TouBefNo = Buf[3*i+2];
		
		if( ((Now == Int) || (mbase_cb->TouBefNo != mbase_cb->TOUNo)) && (mbase_cb->MulPortMode == F_PortTouCut) )
		{
			//MulPort_Set( mbase_cb->MulPortMode );
			//Timer.MulPortCnt = 16;//80ms
            //QF_ON;
		}

	}
	
	if( Num == 1 ) return 0;
	else return i;
}


void GetFeeTab(struct instance_t* instance, struct pipeline_t* pipeline)
{
	uint8_t i,Weekstate,Num0;
	uint32_t Num=0,Num1=0;
    uint32_t s_index= 0,addr=0;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	//公假日表
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 4);
    nd_get_index(instance, s_index, (uint8_t*)&Num, 1); 
	if( Num > MaxHolidayNum ) Num = MaxHolidayNum;
	while( Num > 0 )
	{
		if( Num > 25 )
			Num0 = 25;
		else
			Num0 = (unsigned char)Num;
        
        for(i=0; i<Num0; i++)
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + HoliDay_Table + i);
            nd_get_index(instance, s_index, (uint8_t*)&Buf[i*6], 6); 
        }
        
		for( i=0;i<Num0;i++ )
		{
			if( (Buf[4*i+3] == instance->time.tm_year) && (Buf[4*i+2] == instance->time.tm_mon) && (Buf[4*i+1] == instance->time.tm_mday) )
			{
				mbase_cb->DayTableNo = Buf[4*i+0];//年月日相同，获取日时段表号
				return;
			}
		}
		Num -= Num0;
		Num1 += Num0;
	}

	//周休日表
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Weekend);
    nd_get_index(instance, s_index, (uint8_t*)&Buf[0], 1);
	Weekstate = 0x01;
	Weekstate <<= instance->time.tm_wday;
	if( (Weekstate & Buf[0]) == 0 )//0表示休息日
	{
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + WeekendFee);//周休日表
        nd_get_index(instance, s_index, (uint8_t*)&Buf[1], 1);
	  	mbase_cb->DayTableNo = Buf[1];
	    return;
	}

	//时区表
	if( mbase_cb->SeasonTable > 1 ) mbase_cb->SeasonTable = 0;
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);//读取时区表个数
    nd_get_index(instance, s_index, (uint8_t*)&Num, 1);
    
    if(Num>14) Num = 14;
	if( Num == 0 )
		mbase_cb->DayTableNo = 1;
	else
	{
		if( Num > MaxSeasonNum ) Num = MaxSeasonNum;
        
        if(mbase_cb->SeasonTable == 0) 
            addr = Season_Table0;
        else 
            addr = Season_Table1;
        
        for(i=0; i<Num; i++)
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr + i);  //读取当前时区表
            nd_get_index(instance, s_index, (uint8_t*)&Buf[i*3], 3);
        }
        
		i = FeeTable_CmpFind( instance, pipeline, (unsigned char)Num, 0 );//比较查找时区表
		mbase_cb->DayTableNo = Buf[3*i];
	}
}


void GetTOUFee(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint8_t i,Num=0;
    uint32_t s_index=0;
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
	if( mbase_cb->DayTable > 1 ) mbase_cb->DayTable = 0;
	if( mbase_cb->DayTableNo == 0 ) mbase_cb->DayTableNo = 1;
	if( mbase_cb->DayTableNo > MaxTouTabNum ) mbase_cb->DayTableNo = MaxTouTabNum;
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 2);    //读取日时段个数
    nd_get_index(instance, s_index, (uint8_t*)&Num, 1);
    
	if( Num == 0 )
		mbase_cb->TOUNo = 1;
	else
	{
		if( Num > MaxTouNum ) Num = MaxTouNum;
        
        if(mbase_cb->DayTable == 0)
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table00 + (mbase_cb->DayTableNo-1));
        else
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table10 + (mbase_cb->DayTableNo-1));
        nd_get_index(instance, s_index, Buf, 3*Num);//读取当前日时段表
        
		i = FeeTable_CmpFind( instance, pipeline, Num, 1 );//比较查找日时段表
		mbase_cb->TOUNo = BCD_BYTE(Buf[3*i+2]);
		if( (mbase_cb->TOUNo == 0) || (mbase_cb->TOUNo > MaxFeeNum) ) //若时段表中费率号超出，则按照尖费率处理
			mbase_cb->TOUNo = 1;
	}
}

void TouFeePro(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    
    if( mbase_cb->Fee & F_ChangeTmOn )
	{
		mbase_cb->Fee &= ~F_ChangeTmOn;
		FeeTable_Change(instance, pipeline);
	}

	if( mbase_cb->Fee & F_DAYTABChange )
	{
		mbase_cb->Fee &= ~F_DAYTABChange;
		GetFeeTab(instance, pipeline);
	}

    if(mbase_cb->Fee & F_TOUChange)
    {
        mbase_cb->Fee &= ~F_TOUChange;
        GetTOUFee(instance, pipeline);
    }

}

void rtc_get_from_tm(struct instance_t* instance, uint8_t* p)
{
    unsigned long temp = 0;
    *p = instance->time.tm_sec;
    *(p+1) = instance->time.tm_min;
    *(p+2) = instance->time.tm_hour;
    *(p+3) = instance->time.tm_mday;
    *(p+4)= instance->time.tm_mon + 1;
    temp = instance->time.tm_year + 1900;
    memcpy((uint8_t*)(p+5), (uint8_t*)&temp, 2);
}
    
void Rtc_Prossecc(struct instance_t* instance, struct pipeline_t* pipeline)
{
//    uint8_t buf[7];
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    if(instance->time.tm_sec != 20)//每分鐘的20秒對時
    {
        instance->time.tm_sec++;
        
        rtc_tm_normalize((struct tm*)&instance->time.tm_sec);
    }
    else
    {
        read_mcu_rtc(0x00, instance);
    }
    
    if(instance->time.tm_sec == 0)
    {
        mbase_cb->Fee |= F_TOUChange;
    }
    if(instance->time.tm_min == 0)
    {
        mbase_cb->Fee |= F_ChangeTmOn + F_DAYTABChange;
    }
}


