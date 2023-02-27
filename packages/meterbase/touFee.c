
#include "nidavellir.h"
#include "applications.h"
#include "fr_698.h"
#include "time_link.h"
#include "gs_record.h"
//dependencies
    //1.user define
    //2.mesg[秒]

const unsigned char  MonTab[13] = {	0,31,28,31,30,31,30,31,31,30,31,30,31};

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

//tm 转 698 rtc  (hex)   ssmmhhDDMMYYYY 7字节  hex
void rtc_format_change_from_tm(struct tm *time, uint8_t* p)
{
    unsigned long temp = 0;
    *p = time->tm_sec;
    *(p+1) = time->tm_min;
    *(p+2) = time->tm_hour;
    *(p+3) = time->tm_mday;
    *(p+4)= time->tm_mon + 1;
    temp = time->tm_year + 1900;
    memcpy((uint8_t*)(p+5), (uint8_t*)&temp, 2);
}

//ss mm hh DD MM YY    6 字节BCD
void rtc_format_change_from_tm_BCD(struct tm *time, uint8_t* p)
{
    unsigned long temp = 0;
    *p = BYTE_BCD((uint8_t)time->tm_sec);
    *(p+1) = BYTE_BCD((uint8_t)time->tm_min);
    *(p+2) = BYTE_BCD((uint8_t)time->tm_hour);
    *(p+3) = BYTE_BCD((uint8_t)time->tm_mday);
    *(p+4)= BYTE_BCD((uint8_t)time->tm_mon + 1);
    temp = time->tm_year + 1900;
    if (temp > 2000)
    {
        temp -= 2000;
    }
    else
    {
        temp = 0;
    }
    *(p+5)= BYTE_BCD((uint8_t)temp);
}

void FeeTable_Change(struct instance_t* instance, struct pipeline_t* pipeline)
{
	unsigned char i,j;
	unsigned long CurrentTime,ChangeTime,addr=0,s_index;
    unsigned int year;
    struct mbase_control_block_t* mbase_cb;
    uint8_t Buf[50];
    uint8_t tmp[10] = {0}; 
    uint8_t payload[20] = {0};
    struct message_t mesg_tmp;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

//	ptr = (unsigned char*)&mbase_cb->SeasonTable;
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
		if(( Time_Format_Check( (uint8_t*)&Buf[4] ) ) || (Date_Format_Check((uint8_t*)&Buf[9])))//切换时间合法性判断
			continue;
		if( CurrentTime >= ChangeTime )
		{
			switch( i )
			{
				case 0:					
                    //记录 时区表切换冻结  
                    mesg_tmp.id = SeasonTabCutFreeze;
                    uint32_to_buf_msb(0x20210200, &payload[0]);
                    payload[4] = 8; //data len
                    payload[5] = 0x1C;//TYPE_date_time_s;
                    rtc_format_change_from_tm(&instance->time,tmp);
                    tmp[0] = 0; //sec = 0;
                    mem_swap_cpy(&payload[6], &tmp[0], 7);
                    recv_record_process(instance, NULL, &mesg_tmp, payload);  
                    //send_event_mesg_pro(instance, NULL, SeasonTabCutFreeze, NULL, 1);
					
					mbase_cb->Fee |= (F_TOUChange+F_DAYTABChange);
                    for(j=0; j<14; j++)
                    {
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table1+j);  //读取备用套时区表
                        nd_get_index(instance, s_index, (uint8_t*)&Buf[j*3], 3);
                        
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table0+j);  //覆盖当前时区表
                        nd_set_index(instance, s_index, (uint8_t*)&Buf[j*3], 3);
                    }
                    mbase_cb->PortTouCut = 1;
					break;
				case 1:
				    //记录 日时段表切换冻结
                    mesg_tmp.id = DayTabCutFreeze;
                    uint32_to_buf_msb(0x20210200, &payload[0]);
                    payload[4] = 8; //data len
                    payload[5] = 0x1C;//TYPE_date_time_s;
                    rtc_format_change_from_tm(&instance->time,tmp);
                    tmp[0] = 0; //sec = 0;
                    mem_swap_cpy(&payload[6], &tmp[0], 7);
                    recv_record_process(instance, NULL, &mesg_tmp, payload);  
					//send_event_mesg_pro(instance, NULL, , NULL, 1);
					
					mbase_cb->Fee |= F_TOUChange;
                    for(j=0; j<8; j++)
                    {
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table10 + j);
                        nd_get_index(instance, s_index, Buf, 42);//读取备用套日时段表
                        
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table00 + j);
                        nd_set_index(instance, s_index, Buf, 42);//读取备用套日时段表
                    }
					break;
                default:
					break;
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


unsigned char FeeTable_CmpFind( struct instance_t* instance, struct pipeline_t* pipeline, unsigned char Num, unsigned char Table ,uint8_t * Buf)
{
	unsigned char i;
	unsigned int Now,Int,Int1;
    struct mbase_control_block_t* mbase_cb;
    static uint8_t LastDayTouNum = 0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	//获取当前时间
	if( Table == 0 )
		Now = ((unsigned int)(instance->time.tm_mon+1)<<8) + (unsigned int)instance->time.tm_mday;
	else 
		Now = ((unsigned int)instance->time.tm_hour<<8) + (unsigned int)instance->time.tm_min;

	//比较查找
	for( i=0;i<Num;i++ )
	{
		Int = ((unsigned int)Buf[3*i]<<8) + (unsigned int)Buf[3*i+1];
		Int1 = ((unsigned int)Buf[3*((i+1)%Num)]<<8) + (unsigned int)Buf[3*((i+1)%Num)+1];
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
		//时段发生变化
		if( ((LastDayTouNum != i) || (Now == Int) || (mbase_cb->TouBefNo != mbase_cb->TOUNo)) && (mbase_cb->MulPortMode == F_PortTouCut))
		{
            mbase_cb->PortTouCut = 1;
		}
		LastDayTouNum = i;
	}
	
	if( Num == 1 ) return 0;
	else return i;
}


void GetFeeTab(struct instance_t* instance, struct pipeline_t* pipeline)
{
	uint8_t i,Weekstate,Num0, Num_DayTable = 0;
	uint32_t Num=0,Num1=0;
    uint32_t s_index= 0;
    uint16_t year=0;
    struct mbase_control_block_t* mbase_cb;
    uint8_t Buf[200];
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

	//时段表个数
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 1);
    nd_get_index(instance, s_index, (uint8_t*)&Num_DayTable, 1); 


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
            mem_swap_cpy((uint8_t*)&year, (uint8_t*)&Buf[6*i+0], 2);
			if( (year == (instance->time.tm_year+1900)) && (Buf[6*i+2] == (instance->time.tm_mon+1)) && (Buf[6*i+3] == instance->time.tm_mday) )
			{

                if ((Buf[6*i+5] == 0) || (Buf[6*i+5] > MaxTouTabNum) || (Buf[6*i+5] > Num_DayTable))
                {
                    Buf[6*i+5] = 1;
                } 
                
			    if(mbase_cb->DayTableNo != Buf[6*i+5])
			    {
                    mbase_cb->PortTouCut = 1;
			    }
				mbase_cb->DayTableNo = Buf[6*i+5];//年月日相同，获取日时段表号
                mbase_cb->Fee |= F_TOUChange;
				return;
			}
		}
		Num -= Num0;
		Num1 += Num0;
	}

	//周休日表
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Weekend);
    nd_get_index(instance, s_index, (uint8_t*)&Buf[0], 1);
	Weekstate = 0x80;
	Weekstate >>= instance->time.tm_wday;
	if( (Weekstate & Buf[0]) == 0 )//0表示休息日
	{
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + WeekendFee);//周休日表
        nd_get_index(instance, s_index, (uint8_t*)&Buf[1], 1);

        if ((Buf[1] == 0) || (Buf[1] > MaxTouTabNum) || (Buf[1] > Num_DayTable))
        {
            Buf[1] = 1;
        } 

        if(mbase_cb->DayTableNo != Buf[1])
        {
            mbase_cb->PortTouCut = 1;
        }

	  	mbase_cb->DayTableNo = Buf[1];
        mbase_cb->Fee |= F_TOUChange;
	    return;
	}

    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para);//读取时区表个数
    nd_get_index(instance, s_index, (uint8_t*)&Num, 1);
    
    if(Num>MaxSeasonNum) Num = MaxSeasonNum;
	if( Num == 0 )
		mbase_cb->DayTableNo = 1;
	else
	{
		if( Num > MaxSeasonNum ) Num = MaxSeasonNum;
        
        for(i=0; i<Num; i++)
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table0+i);  //读取当前时区表
            nd_get_index(instance, s_index, (uint8_t*)&Buf[i*3], 3);
        }
        
		i = FeeTable_CmpFind( instance, pipeline, (unsigned char)Num, 0 ,&Buf[0]);//比较查找时区表		

        if ((Buf[3*i+2] == 0) || (Buf[3*i+2] > MaxTouTabNum) || (Buf[3*i+2] > Num_DayTable))
        {
            Buf[3*i+2] = 1;
        } 	

        if(mbase_cb->DayTableNo != Buf[3*i+2])
        {
            mbase_cb->PortTouCut = 1;
        }

		mbase_cb->DayTableNo = Buf[3*i+2];
		
	}
}


void GetTOUFee(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint8_t i,Num[2]={0,};
    uint32_t s_index=0;
    struct mbase_control_block_t* mbase_cb;
    uint8_t Buf[50]={0}, temp = 1;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
	if( mbase_cb->DayTableNo == 0 ) mbase_cb->DayTableNo = 1;
	if( mbase_cb->DayTableNo > MaxTouTabNum ) mbase_cb->DayTableNo = MaxTouTabNum;
    
    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonDay_Para + 2);    //读取日时段个数
    nd_get_index(instance, s_index, (uint8_t*)Num, 2);
    
	if( Num[0] == 0 )
		mbase_cb->TOUNo = 1;
	else
	{
		if( Num[0] > MaxTouNum ) Num[0] = MaxTouNum;
        
        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table00 + (mbase_cb->DayTableNo-1));
        nd_get_index(instance, s_index, Buf, 3*Num[0]);//读取当前日时段表
        
		i = FeeTable_CmpFind( instance, pipeline, Num[0], 1, &Buf[0] );//比较查找日时段表

        temp = BCD_BYTE(Buf[3*i+2]);
		if( (temp == 0) || (temp > MaxFeeNum) || (temp > Num[1]) ) //若时段表中费率号超出，则按照尖费率处理
			temp = 1;    
			
        if(mbase_cb->TOUNo != temp)
        {
            mbase_cb->PortTouCut = 1;
        }

        mbase_cb->TOUNo = temp;

	}
}

void TouFeePro(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    mbase_cb->PortTouCut = 0;
    
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

   
    if( (mbase_cb->PortTouCut) && (mbase_cb->MulPortMode == F_PortTouCut) )
    {
        opreate_second_pluse(2);
        ndv_timer_start_one(MULTIFUNCTION_PORT, 8);
    }
}

void Rtc_Prossecc(struct instance_t* instance, struct pipeline_t* pipeline)
{
//    uint8_t buf[7];
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    
    if( instance->ac_power_statue == AC_POWER_ON )
    {
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
            mbase_cb->Fee |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
        }
    }
    else
    {
        instance->time.tm_sec++;
        rtc_tm_normalize((struct tm*)&instance->time.tm_sec);
    }
    
    if((instance->time.tm_hour == 0) && (instance->time.tm_min == 0) && (instance->time.tm_sec == 0))
    {
        mbase_cb->boardadjtime = 0;
        if(instance->time.tm_year == 200) //2100年时立即归为2000年
        {
            instance->time.tm_year = 100;
        }
    }
/*    if(instance->time.tm_min == 0)
    {
        mbase_cb->Fee |= F_ChangeTmOn + F_DAYTABChange;
    }*/
}


