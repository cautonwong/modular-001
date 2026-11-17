
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "domain/common/status.h"
#include "fr_698.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "domain/tariff/tariff_lookup.h"
#include "domain/common/timer.h"
#include "domain/common/time_format.h"
#include "targets/stategrid_meter/app/events/events_applications.h"
#include "targets/stategrid_meter/app/tariff/tariff_applications.h"
#include "infra/kernel/resource_blocks.h"
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

void rtc_get_from_tm(void *instance, uint8_t* p)
{
    unsigned long temp = 0;
    *p = nd_fr698_time(instance)->tm_sec;
    *(p+1) = nd_fr698_time(instance)->tm_min;
    *(p+2) = nd_fr698_time(instance)->tm_hour;
    *(p+3) = nd_fr698_time(instance)->tm_mday;
    *(p+4)= nd_fr698_time(instance)->tm_mon + 1;
    temp = nd_fr698_time(instance)->tm_year + 1900;
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

void FeeTable_Change(void *instance, void *pipeline)
{
	unsigned char i,j;
	unsigned long CurrentTime,ChangeTime,addr=0,s_index;
    unsigned int year;
    uint8_t Buf[50];
    uint8_t tmp[10] = {0}; 
    uint8_t payload[20] = {0};

//	ptr = (unsigned char*)&mbase_cb->SeasonTable;
    Buf[0] = nd_fr698_time(instance)->tm_min; //min,hour,day,mon,year
    Buf[1] = nd_fr698_time(instance)->tm_hour;
    Buf[2] = nd_fr698_time(instance)->tm_mday;
    Buf[3] = nd_fr698_time(instance)->tm_mon+1;
    Buf[4] = (unsigned char)(nd_fr698_time(instance)->tm_year+1900 - 2000);
	CurrentTime = GetTotalMin( Buf );

	for( i=0;i<2;i++ )
	{
        addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonTable_Changetime + i);
        nd_get_index(instance, addr, Buf, 7); 
        Buf[6] = 0;
        year = ((uint16_t)Buf[0]<<8) + Buf[1];
        Buf[1] = (uint8_t)(year-2000);
        
        mem_swap_cpy((uint8_t*)&Buf[7], (uint8_t*)&Buf[1], 5);
		ChangeTime = GetTotalMin( (uint8_t*)&Buf[7] );//min,hour,day,mon,year

        nd_mb_disp12(instance) = 0;
		if(( Time_Format_Check( (uint8_t*)&Buf[4] ) ) || (Date_Format_Check((uint8_t*)&Buf[9])))//切换时间合法性判断
			continue;
		if( CurrentTime >= ChangeTime )
		{
			switch( i )
			{
				case 0:					
                    //记录 时区表切换冻结
                    uint32_to_buf_msb(0x20210200, &payload[0]);
                    payload[4] = 8; //data len
                    payload[5] = 0x1C;//TYPE_date_time_s;
                    rtc_format_change_from_tm(nd_fr698_time(instance),tmp);
                    tmp[0] = 0; //sec = 0;
                    mem_swap_cpy(&payload[6], &tmp[0], 7);
                    recv_record_process(instance, SeasonTabCutFreeze, payload);
                    //send_event_mesg_pro(instance, NULL, SeasonTabCutFreeze, NULL, 1);
					
					nd_mb_fee(instance) |= (F_TOUChange+F_DAYTABChange);
                    for(j=0; j<14; j++)
                    {
                        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table1+j);  //读取备用套时区表
                        nd_get_index(instance, s_index, (uint8_t*)&Buf[j*3], 3);
                        
                        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Season_Table0+j);  //覆盖当前时区表
                        nd_set_index(instance, s_index, (uint8_t*)&Buf[j*3], 3);
                    }
                    nd_mb_port_tou_cut(instance) = 1;
					break;
				case 1:
				    //记录 日时段表切换冻结
                    uint32_to_buf_msb(0x20210200, &payload[0]);
                    payload[4] = 8; //data len
                    payload[5] = 0x1C;//TYPE_date_time_s;
                    rtc_format_change_from_tm(nd_fr698_time(instance),tmp);
                    tmp[0] = 0; //sec = 0;
                    mem_swap_cpy(&payload[6], &tmp[0], 7);
                    recv_record_process(instance, DayTabCutFreeze, payload);
					//send_event_mesg_pro(instance, NULL, , NULL, 1);
					
					nd_mb_fee(instance) |= F_TOUChange;
                    for(j=0; j<8; j++)
                    {
                        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table10 + j);
                        nd_get_index(instance, s_index, Buf, 42);//读取备用套日时段表
                        
                        s_index = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Day_Table00 + j);
                        nd_set_index(instance, s_index, Buf, 42);//读取备用套日时段表
                    }
					break;
                default:
					break;
			}
            
			memset( Buf, 0x00, 6 );
            Buf[6] = 0xFF;
            addr = ((DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SeasonTable_Changetime + i);
            nd_set_index(instance, addr, Buf, 7); 
		}
        else
        {
            if(i == 1)  
            {   
                nd_mb_disp12(instance) = 1;    //时段表切换时间是未来时
            }
        }
	}
}


void GetFeeTab(void *instance, void *pipeline)
{
    nd_datetime_t now;
    uint8_t day_table;

    tariff_now_datetime(nd_fr698_time(instance), &now);
    if (tariff_lookup_reload(instance) != 0)
    {
        return;
    }

    day_table = tariff_lookup_day_table(&now);
    if (nd_mb_day_table_no(instance) != day_table)
    {
        nd_mb_port_tou_cut(instance) = 1;
    }
    nd_mb_day_table_no(instance) = day_table;
    nd_mb_fee(instance) |= F_TOUChange;
}

void GetTOUFee(void *instance, void *pipeline)
{
    nd_datetime_t now;
    uint8_t fee, period_changed;

    if (nd_mb_day_table_no(instance) == 0) nd_mb_day_table_no(instance) = 1;
    if (nd_mb_day_table_no(instance) > MaxTouTabNum) nd_mb_day_table_no(instance) = MaxTouTabNum;

    tariff_now_datetime(nd_fr698_time(instance), &now);
    if (tariff_lookup_reload(instance) != 0)
    {
        return;
    }

    fee = tariff_lookup_fee(nd_mb_day_table_no(instance), &now, &period_changed);
    nd_mb_tou_bef_no(instance) = fee;

    if (nd_mb_tou_no(instance) != fee)
    {
        nd_mb_port_tou_cut(instance) = 1;
    }
    nd_mb_tou_no(instance) = fee;

    if (period_changed && (nd_mb_mul_port_mode(instance) == ND_MB_FLAG_PORT_TOU_CUT))
    {
        nd_mb_port_tou_cut(instance) = 1;
    }
}

void TouFeePro(void *instance, void *pipeline)
{
    
    nd_mb_port_tou_cut(instance) = 0;
    
    if( nd_mb_fee(instance) & F_ChangeTmOn )
	{
		nd_mb_fee(instance) &= ~F_ChangeTmOn;
		FeeTable_Change(instance, pipeline);
	}

	if( nd_mb_fee(instance) & F_DAYTABChange )
	{
		nd_mb_fee(instance) &= ~F_DAYTABChange;
		GetFeeTab(instance, pipeline);
	}

    if(nd_mb_fee(instance) & F_TOUChange)
    {
        nd_mb_fee(instance) &= ~F_TOUChange;
        GetTOUFee(instance, pipeline);
    }

   
    if( (nd_mb_port_tou_cut(instance)) && (nd_mb_mul_port_mode(instance) == ND_MB_FLAG_PORT_TOU_CUT) )
    {
        opreate_second_pluse(2);
        nd_timer_start_one(ND_TIMER_MULTIFUNCTION_PORT, 8);
    }
}

void Rtc_Prossecc(void *instance, void *pipeline)
{
//    uint8_t buf[7];
    
    
    if( *nd_fr698_power_state(instance) == ND_AC_POWER_ON )
    {
        if(nd_fr698_time(instance)->tm_sec != 20)//每分鐘的20秒對時
        {
            nd_fr698_time(instance)->tm_sec++;
            
            rtc_tm_normalize(nd_fr698_time(instance));
        }
        else
        {
            read_mcu_rtc(0x00, nd_fr698_time(instance));
        }
        
        if(nd_fr698_time(instance)->tm_sec == 0)
        {
            nd_mb_fee(instance) |= F_TOUChange + F_ChangeTmOn + F_DAYTABChange;
        }
    }
    else
    {
        nd_fr698_time(instance)->tm_sec++;
        rtc_tm_normalize(nd_fr698_time(instance));
    }
    
    if((nd_fr698_time(instance)->tm_hour == 0) && (nd_fr698_time(instance)->tm_min == 0) && (nd_fr698_time(instance)->tm_sec == 0))
    {
        nd_mb_boardadjtime(instance) = 0;
        if(nd_fr698_time(instance)->tm_year == 200) //2100年时立即归为2000年
        {
            nd_fr698_time(instance)->tm_year = 100;
        }
    }
/*    if(nd_fr698_time(instance)->tm_min == 0)
    {
        nd_mb_fee(instance) |= F_ChangeTmOn + F_DAYTABChange;
    }*/
}


