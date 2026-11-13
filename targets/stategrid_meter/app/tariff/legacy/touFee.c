
#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "domain/common/status.h"
#include "fr_698.h"
#include "gs_record.h"
#include "domain/tariff/tariff_lookup.h"
#include "domain/common/timer.h"
#include "domain/events/legacy/events_applications.h"
#include "domain/tariff/legacy/tariff_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"
//dependencies
    //1.user define
    //2.mesg[秒]

static void tariff_now_datetime(const struct tm* time, nd_datetime_t* out)
{
    out->second = time->tm_sec;
    out->minute = time->tm_min;
    out->hour = time->tm_hour;
    out->day = time->tm_mday;
    out->month = (uint8_t)(time->tm_mon + 1);
    out->year = (uint16_t)(time->tm_year + 1900);
}

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

        nd_mbase_Disp12(mbase_cb) = 0;
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
					
					nd_mbase_Fee(mbase_cb) |= (F_TOUChange+F_DAYTABChange);
                    for(j=0; j<14; j++)
                    {
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table1+j);  //读取备用套时区表
                        nd_get_index(instance, s_index, (uint8_t*)&Buf[j*3], 3);
                        
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table0+j);  //覆盖当前时区表
                        nd_set_index(instance, s_index, (uint8_t*)&Buf[j*3], 3);
                    }
                    nd_mbase_PortTouCut(mbase_cb) = 1;
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
					
					nd_mbase_Fee(mbase_cb) |= F_TOUChange;
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
                nd_mbase_Disp12(mbase_cb) = 1;    //时段表切换时间是未来时
            }
        }
	}
}


void GetFeeTab(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    nd_datetime_t now;
    uint8_t day_table;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    tariff_now_datetime(&instance->time, &now);
    if (tariff_lookup_reload(instance) != 0)
    {
        return;
    }

    day_table = tariff_lookup_day_table(&now);
    if (nd_mbase_DayTableNo(mbase_cb) != day_table)
    {
        nd_mbase_PortTouCut(mbase_cb) = 1;
    }
    nd_mbase_DayTableNo(mbase_cb) = day_table;
    nd_mbase_Fee(mbase_cb) |= F_TOUChange;
}

void GetTOUFee(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    nd_datetime_t now;
    uint8_t fee, period_changed;

    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    if (nd_mbase_DayTableNo(mbase_cb) == 0) nd_mbase_DayTableNo(mbase_cb) = 1;
    if (nd_mbase_DayTableNo(mbase_cb) > MaxTouTabNum) nd_mbase_DayTableNo(mbase_cb) = MaxTouTabNum;

    tariff_now_datetime(&instance->time, &now);
    if (tariff_lookup_reload(instance) != 0)
    {
        return;
    }

    fee = tariff_lookup_fee(nd_mbase_DayTableNo(mbase_cb), &now, &period_changed);
    nd_mbase_TouBefNo(mbase_cb) = fee;

    if (nd_mbase_TOUNo(mbase_cb) != fee)
    {
        nd_mbase_PortTouCut(mbase_cb) = 1;
    }
    nd_mbase_TOUNo(mbase_cb) = fee;

    if (period_changed && (nd_mbase_MulPortMode(mbase_cb) == F_PortTouCut))
    {
        nd_mbase_PortTouCut(mbase_cb) = 1;
    }
}

void TouFeePro(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    nd_mbase_PortTouCut(mbase_cb) = 0;
    
    if( nd_mbase_Fee(mbase_cb) & F_ChangeTmOn )
	{
		nd_mbase_Fee(mbase_cb) &= ~F_ChangeTmOn;
		FeeTable_Change(instance, pipeline);
	}

	if( nd_mbase_Fee(mbase_cb) & F_DAYTABChange )
	{
		nd_mbase_Fee(mbase_cb) &= ~F_DAYTABChange;
		GetFeeTab(instance, pipeline);
	}

    if(nd_mbase_Fee(mbase_cb) & F_TOUChange)
    {
        nd_mbase_Fee(mbase_cb) &= ~F_TOUChange;
        GetTOUFee(instance, pipeline);
    }

   
    if( (nd_mbase_PortTouCut(mbase_cb)) && (nd_mbase_MulPortMode(mbase_cb) == F_PortTouCut) )
    {
        opreate_second_pluse(2);
        nd_timer_start_one(ND_TIMER_MULTIFUNCTION_PORT, 8);
    }
}

void Rtc_Prossecc(struct instance_t* instance, struct pipeline_t* pipeline)
{
//    uint8_t buf[7];
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    
    if( instance->ac_power_statue == ND_AC_POWER_ON )
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
            nd_mbase_Fee(mbase_cb) |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
        }
    }
    else
    {
        instance->time.tm_sec++;
        rtc_tm_normalize((struct tm*)&instance->time.tm_sec);
    }
    
    if((instance->time.tm_hour == 0) && (instance->time.tm_min == 0) && (instance->time.tm_sec == 0))
    {
        nd_mbase_boardadjtime(mbase_cb) = 0;
        if(instance->time.tm_year == 200) //2100年时立即归为2000年
        {
            instance->time.tm_year = 100;
        }
    }
/*    if(instance->time.tm_min == 0)
    {
        nd_mbase_Fee(mbase_cb) |= F_ChangeTmOn + F_DAYTABChange;
    }*/
}


