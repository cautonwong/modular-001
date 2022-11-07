#include "nidavellir.h"
#include "645id_table.h"
#include "utils.h"
#include "gs_record.h"

enum meter_address_type645
{

    SA_TYPE_SINGLE645      = 0,    //0-ÂçïÂú∞ÂùÄ Êàñ ÈÄöÈÖçÂú∞ÂùÄ 
    SA_TYPE_GENERAL645     = 1,    //1-ÈÄöÈÖçÂú∞ÂùÄ 
    SA_TYPE_BROADECAST645  = 2,    //2-ÂπøÊí≠Âú∞ÂùÄ

};

signed char Broadcast_timeadj(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t Buf[7] = {0,},i;
    int32_t Lng1=0,Lng2=0;
    uint32_t s_index=0;
    uint16_t TimeSec=0;
    uint8_t maddr_type = 0xFF;
    uint8_t buffer[10]={0},temp[10]={0};


    if(( Cmp_MeterAddr( instance, p_rx+1 , 1 ) == 0 ))
    {
        maddr_type = SA_TYPE_SINGLE645;  //ÈÄöÈÖç ÂæÖË∞ÉÊï¥
    }
    else if((Addr_9_9( p_rx+1 ) == 0))
    {
        maddr_type = SA_TYPE_BROADECAST645;  //  99 ÂπøÊí≠
    }
    else
    {
        return -1; //‰∏çÂìçÂ∫î
    }

    if((*(p_rx+9) == 6) )
    {
        if((mbase_cb->metertype & F_Password_En) == 0)   //√˜Œƒƒ£ Ω≤ª◊ˆ≈–∂œ
        {
            if(chl == chl_ir_698)    
            {
                if((mbase_cb->infrared_auth_timeout == 0) && (mbase_cb->factor_mode == false))//∫ÏÕ‚Õ®µ¿±ÿ–Î‘⁄∫ÏÕ‚»œ÷§”––ß∆⁄º‰Ω¯––≤Ÿ◊˜°£
                {
                    if(maddr_type == SA_TYPE_BROADECAST645)
                        return -1;
                    else
                        return RS_ERR_OTHERS;
                }   
            }
        }
        TimeSec = instance->time.tm_hour;
        TimeSec = TimeSec*60;
        Lng1 = TimeSec + instance->time.tm_min;
        Lng1 = Lng1*60 + instance->time.tm_sec;
        if( ( Lng1 > 86100 ) || ( Lng1 < 301 ) )        //±‹√‚ŒÁ“π¡„ ±«∞∫Û5∑÷÷”–£ ±
        {
            if(maddr_type == SA_TYPE_BROADECAST645)
                return -1;
            else
                return RS_ERR_OTHERS;
        }  
        
        for(i=0;i<3;i++)//±‹√‚‘¬Ω·À„ ±–£ ±°£
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
            nd_get_index(instance, s_index, (uint8_t*)&Buf[2], 2); 
            mem_swap_cpy(Buf, (uint8_t*)&Buf[2], 2);
            if( (Buf[0]>23) || (Buf[1]>28) )
                continue;
            
            if( Buf[1] == instance->time.tm_mday )
            {
                Lng2 = Buf[0];
                Lng2 = Lng2*3600;
                Lng2 = Lng2 - Lng1;
                if( ((Lng2 <= 300)&&(Lng2>=0)) || ((Lng2 >= -300)&&(Lng2<=0)) )
                {
                    if(maddr_type == SA_TYPE_BROADECAST645)
                        return -1;
                    else
                        return RS_ERR_OTHERS;
                }
            }
        }
        #if 0//(CPU_CARD_EN) 
        for(i=0;i<4;i++)//±‹√‚‘¬Ω·À„ ±–£ ±°£
        {
            E2P_Read( Buffer, Step_Table_Balance0+3*i, 3 );
            if( Buffer[2] == Clock_Block.Clock.Mon)//
            {
                if( Buffer[1] == Clock_Block.Clock.Day )
                {
                    Lng2 = BCD_BYTE(Buffer[0]);
                    Lng2 = Lng2*3600;
                    Lng2 = Lng2 - Lng1;
                    if( ((Lng2 <= 300)&&(Lng2>=0)) || ((Lng2 >= -300)&&(Lng2<=0)) )
                    {
                        Rs_Head1 = 0xFF;
                        return;
                    }
                }
            }
        }
        #endif
        Buf[0] = instance->time.tm_mday;  //»’
        Buf[1] = instance->time.tm_mon+1;  //‘¬
        Buf[2] = (uint8_t)((instance->time.tm_year + 1900)-2000);  //ƒÍ
        Lng1 = GetTotalDay( Buf );  //µ±«∞ ±º‰
        
        Buf[0] = BCD_BYTE(*(p_rx+13));  //»’
        Buf[1] = BCD_BYTE(*(p_rx+14));  //‘¬
        Buf[2] = BCD_BYTE(*(p_rx+15));  //ƒÍ
        Lng2 = GetTotalDay( Buf );  //π„≤•–£ ± ±º‰
        if( Lng1 == Lng2 )//÷ª”–‘⁄»’∆⁄“ª÷¬µƒ«Èøˆœ¬£¨≤≈‘ –ÌΩ¯––π„≤•–£ ±
        {
            Buf[2] = BCD_BYTE(*(p_rx+10));
            Buf[1] = BCD_BYTE(*(p_rx+11));
            Buf[0] = BCD_BYTE(*(p_rx+12));
            Buf[3] = BCD_BYTE(*(p_rx+13));
            Buf[4] = BCD_BYTE(*(p_rx+14));
            if(( Time_Format_Check( Buf ) == 0 ) && (Date_Format_Check((uint8_t*)&Buf[3]) == 0 ))
            {
                Buf[0] = BCD_BYTE(*(p_rx+10));
                Buf[1] = BCD_BYTE(*(p_rx+11));
                Buf[2] = BCD_BYTE(*(p_rx+12));
                Buf[3] = BCD_BYTE(*(p_rx+13));
                Buf[4] = BCD_BYTE(*(p_rx+14));
                Buf[5] = BCD_BYTE(*(p_rx+15));
                Buf[6] = GetWeek(&Buf[3]);
                Lng2 = GetTotalSec( Buf );  //–£ ± ±º‰
                
                Buf[0] = instance->time.tm_sec;
                Buf[1] = instance->time.tm_min;
                Buf[2] = instance->time.tm_hour;
                Buf[3] = instance->time.tm_mday;
                Buf[4] = instance->time.tm_mon+1;
                Buf[5] = (uint8_t)((instance->time.tm_year + 1900)-2000);
                
                Lng1 = GetTotalSec( Buf );//µ±«∞ ±º‰
                
                Lng2 = Lng2 - Lng1;
                if( Lng2 & 0x80000000 )
                    Lng2 = ~Lng2 + 1;
                if( (Lng2 <= (5*60)) && (mbase_cb->boardadjtime != 0xAA) )//“ªÃÏ÷ª‘ –Ìπ„≤•–£ ±“ª¥Œ£¨–£ ±∑˘∂»–°”⁄5∑÷÷”
                {


                    
                    //‰øùÂ≠òÊ†°Êó∂ÂâçÊó∂Èó¥ 
                    rtc_get_from_tm(instance,temp);
                    mem_swap_cpy( &buffer[0], &temp[0], 7);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
                    nd_set_index(instance, s_index, buffer, 7);
                    

                
                    mbase_cb->boardadjtime = 0xAA;
                    Buf[0] = (*(p_rx+10));
                    Buf[1] = (*(p_rx+11));
                    Buf[2] = (*(p_rx+12));
                    Buf[3] = (*(p_rx+13));
                    Buf[4] = (*(p_rx+14));
                    Buf[5] = (*(p_rx+15));
                    write_mcu_rtc(0, Buf, 7);

                    read_mcu_rtc(0x00, instance);


                    //‰øùÂ≠òÊ†°Êó∂ÂêéÊó∂Èó¥ 
                    rtc_get_from_tm(instance,temp);
                    mem_swap_cpy( &buffer[0], &temp[0], 7);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
                    nd_set_index(instance, s_index, buffer, 7);
                    
                    record_event_no_mesg_pro(instance, NULL, BroadcastTimeS, NULL, 1);// ÂπøÊí≠Ê†°Êó∂ËÆ∞ÂΩï
                    after_Time_set_hook(instance);

                    if(maddr_type == SA_TYPE_BROADECAST645)
                        return -1;
                    else
                        return 0;
                }
            }
        }
        
    }
    
    if(maddr_type == SA_TYPE_BROADECAST645)
        return -1;
    else
        return RS_ERR_OTHERS;

}

signed char meteraddr_write(struct instance_t* instance, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_index = 0;    
    
    if( (mbase_cb->factor_mode == true) || (mbase_cb->CoverOpenS_status == EVENT_SET) )
    {
        if( (Addr_A_A( p_rx+1 ) == 0) && (*(p_rx+9) == 6) && (BCD_Check( p_rx+10, 6 ) == 0) )
        {
            memcpy(mbase_cb->tsa, p_rx+10, 6);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    offsetof(struct mbase_control_block_t, tsa));
            nd_set_offset(instance, s_index, mbase_cb->tsa, 6); 
            return 0;
        }
        else
            return -1;
    }
    else
        return -1;
}

signed char MulPort_set(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len = *(p_rx+9);
    
    if( (Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0) || (Addr_9_9( p_rx+1 ) == 0) )
    {
        if( len == 1 )
        {
            if( (*(p_rx+10) == 0) || (*(p_rx+10) == 2) )
            {
                if((mbase_cb->metertype & F_Password_En) == 0)   //√˜Œƒƒ£ Ω≤ª◊ˆ≈–∂œ
                {
                    if(chl == chl_ir_698)    
                    {
                        if((mbase_cb->infrared_auth_timeout != 0) && (mbase_cb->factor_mode == false))//∫ÏÕ‚Õ®µ¿±ÿ–Î‘⁄∫ÏÕ‚»œ÷§”––ß∆⁄º‰Ω¯––≤Ÿ◊˜°£
                        {
                            mbase_cb->MulPortMode = (*(p_rx+10));
                            if(F_PortSec != mbase_cb->MulPortMode)    
                            {
                                opreate_second_pluse(3);    //ÂàáÊç¢Ëá≥Â§öÂäüËÉΩÂè£
                            }
                            else
                            {
                                opreate_second_pluse(1);    //ÁßíËÑâÂÜ≤
                            }
                        }    
                        else
                            return RS_ERR_PASS;
                    }
                }
                else
                {
                    mbase_cb->MulPortMode = (*(p_rx+10));
                    if(F_PortSec != mbase_cb->MulPortMode)    
                    {
                        opreate_second_pluse(3);    //ÂàáÊç¢Ëá≥Â§öÂäüËÉΩÂè£
                    }
                    else
                    {
                        opreate_second_pluse(1);    //ÁßíËÑâÂÜ≤
                    }
                }
            }
            else
                return RS_ERR_OTHERS;
        }
        else
            return RS_ERR_LEN;
        if( Addr_9_9( p_rx+1 ) == 0 )//π„≤•≤ª“™«Û”¶¥
            return -1;
        else
            return 0;
    }
    else
        return -1;
}


signed char meter_clear(struct instance_t* instance, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len=*(p_rx+9);
    
    if( Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0 )
    {
        if( *(p_rx+10) == 0x98)
        {
          /*
            #if FEIKONG_MODE == 1
            Err = CommRemoteCtl_Process(0x94);
            #elif FEIKONG_MODE == 0
            Err = 0x04;
            #endif
            if( Err == 0 )
            {
                Rs_Ctl |= 0x80;
                Rs_Len = 0;
            }
            else
                Return_CommErr( Err );*/
        }
        else  if( mbase_cb->factor_mode == true )
        {
            if( len == 8 )
                meter_clr(instance);
            else
                return RS_ERR_LEN;

        }
        else
            return RS_ERR_PASS;
        
        return 0;
    }
    else
        return -1;
    
}

signed char event_clear(struct instance_t* instance, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len = *(p_rx+9);
    
    if( Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0 )
    {
        if( *(p_rx+10) == 0x98)
        {
            /*Err = CommRemoteCtl_Process(0x97);
            if( Err == 0 )
            {
                Rs_Ctl |= 0x80;
                Rs_Len = 0;
            }
            else
                Return_CommErr( Err );*/
        }
        else if(mbase_cb->factor_mode == true)
        {
            if( len == 12 )
            {
                event_clr(instance);//  ¬º˛«Â¡„
            }
            else
                return RS_ERR_LEN;
        }
        else
            return RS_ERR_PASS;
        
        return 0;
    }
    else    
        return -1;
}

/***********************************************************************/
/*  ∫Ø ˝√˚≥∆£∫Cmp_RemoteCtlTime                                        */
/*  ∫Ø ˝π¶ƒ‹£∫≈–∂œ‘∂≥Ãøÿ÷∆√¸¡Ó «∑Ò‘⁄√¸¡Ó”––ß ±º‰ƒ⁄œ¬∑¢                 */
/*   ‰»Î≤Œ ˝£∫Addr£∫ ˝æ›µÿ÷∑                                           */
/*   ‰≥ˆ≤Œ ˝£∫0Œ™’˝»∑£¨∆‰”‡Œ™¥ÌŒÛ                                      */
/***********************************************************************/
unsigned char Cmp_RemoteCtlTime( struct instance_t* instance, uint8_t* Addr )
{
	unsigned long Lng1,Lng2;
    unsigned char Buf[6]={0,},Buf1[6]={0,},i;
    
    Buf[0] = instance->time.tm_sec;
    Buf[1] = instance->time.tm_min; //min,hour,day,mon,year
    Buf[2] = instance->time.tm_hour;
    Buf[3] = instance->time.tm_mday;
    Buf[4] = instance->time.tm_mon+1;
    Buf[5] = (unsigned char)(instance->time.tm_year+1900 - 2000);
	Lng1 = GetTotalSec( Buf );  //µ±«∞ ±º‰
    
	mem_swap_cpy( Buf, Addr, 6 );    //√¸¡Ó”––ßΩÿ÷¡ ±º‰
	if(Buf[0]>0x59)
		return 1;

	if(0 != ChangeTime_Format_Check((unsigned char*)&Buf[1]))
		return 1;
    
    bcd_to_hex_array(Buf1, Buf, 6);
    
	Lng2 = GetTotalSec( Buf1 );

	if( Lng1 > Lng2 )
		return 1;
	else
		return 0;
}

signed char CommRemoteCtl_Process(struct instance_t* instance, uint8_t chl, uint8_t* p_rx, uint16_t RemotCtrResult)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	signed char Err_Byte=0;
	unsigned long Lng1,s_index=0;
	unsigned char DataLen, Cmd=0, TmpBuffer[20]={0,};
	
	RemotCtrResult = 0;
	
	if(((*(p_rx+18)==0x1B) || (*(p_rx+18)==0x1C)) && ( Cmp_Password( instance, p_rx+10 ) == 0) ) //√˜Œƒ∫œ’¢
	{
		mem_swap_cpy( TmpBuffer, p_rx+20, 6 );
		if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )    //≈–∂œ¿≠∫œ’¢√¸¡Ó «∑Ò‘⁄√¸¡Ó”––ß ±º‰ƒ⁄œ¬∑¢
		{
			RemotCtrResult = ERR_CMDTM_INVALID;
			return RS_ERR_PASS;
		}
        Cmd = *(p_rx+18);
	}
	else
	{
	    if(mbase_cb->factor_mode == false)
	    {
            if((mbase_cb->metertype & F_Password_En) == 0)   //√˜Œƒƒ£ Ω≤ª◊ˆ≈–∂œ
            {
                if(chl == chl_ir_698)    
                {
                    if((mbase_cb->infrared_auth_timeout == 0))//∫ÏÕ‚Õ®µ¿±ÿ–Î‘⁄∫ÏÕ‚»œ÷§”––ß∆⁄º‰Ω¯––≤Ÿ◊˜°£
                    {
                        RemotCtrResult = ERR_AUTHENTM_OVER;
                        return RS_ERR_PASS;
                    }
                }
            }

    		if( mbase_cb->mac_suspend_state == true)
    		{
    				RemotCtrResult = ERR_HALT;
    				return RS_ERR_PASS;
    		}
	    }

        if( *(p_rx+10) == 0x02 )
        {
            if(((mbase_cb->metertype & F_Password_En) == 0) && (mbase_cb->factor_mode == false))   //√˜Œƒƒ£ Ω
            {
                RemotCtrResult = ERR_OTHERR;
                return RS_ERR_PASS;
            }
            mem_swap_cpy( TmpBuffer, p_rx+20, 6 );
            Cmd = *(p_rx+18);
        }
        else if( *(p_rx+10) == 0x98 )
        {
            /*
            if( AuthenTime_Invalid() )//…Ì∑›»œ÷§ ß–ß
            {
                    RemotCtrResult = ERR_AUTHENTM_OVER;
                    return RS_ERR_PASS;
            }
            //√‹Œƒ+MAC–£—È
            HBF_LOOP_LBF( Card.Buffer, (unsigned char*)&Comm.Buff[18], 20 );

            if( ISO7816_WriteData_MAC( CtrFile, 0, 16, ISO7816_ESAM ) )
            {
                Comm_MACWrite_Err();
                RemotCtrResult = ERR_ERRMAC;
                return RS_ERR_PASS;
            }
            DataLen = 8;
            //ªÒ»°√˜Œƒ ˝æ›
            if( ISO7816_ReadData( CtrFile, 0, DataLen, ISO7816_ESAM ) )
            {
                RemotCtrResult = ERR_OTHERR;
                return RS_ERR_PASS;
            }
            */
        }
        else
        {
            RemotCtrResult = ERR_OTHERR;
            return RS_ERR_PASS;
        }

		//BCD¬Î≈–∂œ
		if( BCD_Check( TmpBuffer, 6 ) != 0 )
		{
			RemotCtrResult = ERR_OTHERR;
			return RS_ERR_PASS;
		}

		//≈–∂œ¿≠∫œ’¢√¸¡Ó «∑Ò‘⁄√¸¡Ó”––ß ±º‰ƒ⁄œ¬∑¢
		if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
		{
			RemotCtrResult = ERR_CMDTM_INVALID;
			return RS_ERR_PASS;
		}
	}

	//øÿ÷∆√¸¡ÓΩ‚Œˆ
	switch( Cmd )
	{
		case 0x1A://Ã¯’¢
				if( (mbase_cb->sta & F_RemoteRelayOn) == 0 )
				{
                        mbase_cb->sta |= F_RemoteRelayOff;
						mbase_cb->sta &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
						
						mbase_cb->alarmmin = 0;
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Remote_RelayOffTime);//Ã¯’¢∏ÊæØ
                        nd_get_index(instance, s_index, TmpBuffer, 2);
                        mem_swap_cpy((uint8_t*)&mbase_cb->alarmmin, TmpBuffer, 2);
                        
						mbase_cb->alarmmin = mbase_cb->alarmmin*60;

						if(mbase_cb->alarmmin > 0)
						{
							RemotCtrResult = CTRL_STATUE_RELAYOFF_DT;
							if((abs(mbase_cb->current_l) > (mbase_cb->I_Prot / 10)) && (mbase_cb->I_Prot != 0))
							{
								mbase_cb->imaxmin = mbase_cb->Prot_time;   
                                mbase_cb->imaxmin = mbase_cb->imaxmin*60;
							}
						}
						else if((abs(mbase_cb->current_l) > (mbase_cb->I_Prot / 10)) && (mbase_cb->I_Prot != 0))
						{
							mbase_cb->imaxmin = mbase_cb->Prot_time;   
                            mbase_cb->imaxmin = mbase_cb->imaxmin*60;
							RemotCtrResult = CTRL_STATUE_RELAYOFF_IMAX;
						}
						else
                        {
                            mbase_cb->imaxmin = 0;
							RemotCtrResult = CTRL_STATUE_RELAYOFF;
                        }
				}
				else
				{
					RemotCtrResult = ERR_RELAYOFF_BAODIAN;
					return RS_ERR_PASS;
				}
				break;

		case 0x1B://∫œ’¢‘ –Ì
				mbase_cb->sta &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				mbase_cb->sta &= ~F_RemoteRelayOff;
                
				if(mbase_cb->imaxmin > 0)   mbase_cb->imaxmin = 0;   //¥ÛµÁ¡˜—” ±£¨«Â≥˝¥ÛµÁ¡˜—” ± ±º‰
                if(mbase_cb->alarmmin > 0)  mbase_cb->alarmmin = 0;     //Ã¯’¢—” ±£¨«Â¡„
                    
				RemotCtrResult = CTRL_STATUE_RELAYON_EN;
                
				if(mbase_cb->metertype & F_RelayonAuto_En)  //∫œ’¢‘ –Ì◊‘∂Ø∫œ’¢
                {
					mbase_cb->sta |= F_RemoteRelayOnDirect; //÷±Ω”∫œ’¢
                    mbase_cb->sta &= ~F_RemoteRelayOnAllow;
                }
				else
                {
				    mbase_cb->sta |= F_RemoteRelayOnAllow;  //∫œ’¢‘ –Ì
                }
				#if 0//(CPU_CARD_EN) 
				//”√ªßø®»°œ˚¿≠’¢±®æØ
				if( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.AlarmCredit2) && (MeterPara_Block.MeterPara.AlarmCredit2 != 0) && ((Flag.MeterState & F_Alarmed) == 0) )
				{
					Flag.MeterState |= F_Alarmed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				//‘ –Ì”√ªßºÃ–¯Õ∏÷ß
				if( (Credit_Block.Credit.Amount <= 0) && ((Flag.MeterState & F_ZeroRelayOffed)==0) )
				{
					Flag.MeterState |= F_ZeroRelayOffed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				#endif
				if(mbase_cb->metertype & F_ExternRelay_En)  //Õ‚÷√ºÃµÁ∆˜
				{
					mbase_cb->sta |= F_RemoteRelayOnDirect; //÷±Ω”∫œ’¢
                    mbase_cb->sta &= ~F_RemoteRelayOnAllow;
				}
				break;
		case 0x1C://÷±Ω”∫œ’¢
				mbase_cb->sta &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				mbase_cb->sta &= ~F_RemoteRelayOff;
				
				#if 0//(CPU_CARD_EN) 
				//”√ªßø®»°œ˚¿≠’¢±®æØ
				if( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.AlarmCredit2) && (MeterPara_Block.MeterPara.AlarmCredit2 != 0) && ((Flag.MeterState & F_Alarmed) == 0) )
				{
					Flag.MeterState |= F_Alarmed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				//‘ –Ì”√ªßºÃ–¯Õ∏÷ß
				if( (Credit_Block.Credit.Amount <= 0) && ((Flag.MeterState & F_ZeroRelayOffed)==0) )
				{
					Flag.MeterState |= F_ZeroRelayOffed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				#endif
				
				if(mbase_cb->imaxmin > 0)   mbase_cb->imaxmin = 0;   //¥ÛµÁ¡˜—” ±£¨«Â≥˝¥ÛµÁ¡˜—” ± ±º‰
                if(mbase_cb->alarmmin > 0)  mbase_cb->alarmmin = 0;     //Ã¯’¢—” ±£¨«Â¡„
                
                mbase_cb->sta |= F_RemoteRelayOnDirect; //÷±Ω”∫œ’¢
                mbase_cb->sta &= ~F_RemoteRelayOnAllow;
				RemotCtrResult = CTRL_STATUE_RELAYON;
				break;
		case 0x1D://‘§Ã¯’¢
		case 0x1E:
				if (mbase_cb->sta & F_RemoteRelayOn)    //±£µÁ
				{
					RemotCtrResult = ERR_RELAYOFF_ALARM_BAODIAN;
					return RS_ERR_PASS;
				}
				else if((mbase_cb->sta & F_RemoteRelayOff) || ((mbase_cb->sta & F_RemoteRelayOnAllow) && (mbase_cb->sta != (F_RemoteGaoJing0+F_RemoteGaoJing1))))   //¿≠’¢◊¥Ã¨œ¬
				{
					RemotCtrResult = ERR_RELAYOFF_ALARM_RELAYOFF;
					return RS_ERR_PASS;
				}
				#if 0//(CPU_CARD_EN) 
				else if((Relay.Reason == RelayOff_Alarm) || (Relay.Reason == RelayOff_Zero) || (Relay.Reason == RelayOff_Touzhi))
				{
					Card.RemotCtrResult = ERR_RELAYOFF_ALARM_RELAYOFF;
					return RS_ERR_PASS;
				}
				#endif
				else
				{
					if(*(p_rx+19) == 0)
					{
						RemotCtrResult = ERR_RELAYOFF_ALARM_INVALID;
						return RS_ERR_PASS;						
					}
					else
					{
						if(Cmd == 0x1D)
						{
							mbase_cb->sta |= F_RemoteGaoJing0;
							mbase_cb->sta &= ~F_RemoteGaoJing1;
						}
						else if(Cmd == 0x1E)
						{
							mbase_cb->sta |= F_RemoteGaoJing1;
							mbase_cb->sta &= ~F_RemoteGaoJing0;	
						}
						mbase_cb->limitmin = *(p_rx+19)*60*5;

						mbase_cb->alarmmin = 0;
						if((abs(mbase_cb->current_l) > (mbase_cb->I_Prot / 10)) && (mbase_cb->I_Prot != 0))
                        {
                            mbase_cb->imaxmin = mbase_cb->Prot_time;   
                            mbase_cb->imaxmin = mbase_cb->imaxmin*60;
                            RemotCtrResult = CTRL_STATUE_RELAYOFF_IMAX;
                        }
						else
							RemotCtrResult = CTRL_STATUE_RELAYOFF_ALARM;
					}
				}
				break;
		case 0x2A://±®æØ
				mbase_cb->sta |= F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_ON;
				break;
		case 0x2B://±®æØΩ‚≥˝
				mbase_cb->sta &= ~F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_OFF;
				break;
		case 0x3A://±£µÁ
          
                if((mbase_cb->sta & F_RemoteRelayOff) && (mbase_cb->alarmmin > 0))//¿≠’¢—” ±π˝≥Ã÷–£¨±£µÁ∫Û¿≠’¢◊÷∑˚œ˚ ß£¨µ∆√
                {
                    mbase_cb->alarmmin = 0;
                    mbase_cb->sta &= ~F_RemoteRelayOff;
                }
                mbase_cb->sta |= F_RemoteRelayOn;
                mbase_cb->sta &= ~(F_RemoteGaoJing1 + F_RemoteGaoJing0);
                mbase_cb->alarmmin = 0;     //Ã¯’¢—” ±£¨«Â¡„    
                mbase_cb->imaxmin = 0;   //¥ÛµÁ¡˜—” ±£¨«Â≥˝¥ÛµÁ¡˜—” ± ±º‰

				if(mbase_cb->metertype & F_RelayonAuto_En)  //∫œ’¢‘ –Ì◊‘∂Ø∫œ’¢
                {
					mbase_cb->sta |= F_RemoteRelayOnDirect; //÷±Ω”∫œ’¢
                    mbase_cb->sta &= ~F_RemoteRelayOnAllow;
                }
				else
                {
				    mbase_cb->sta |= F_RemoteRelayOnAllow;  //∫œ’¢‘ –Ì
                }
                
				RemotCtrResult = CTRL_STATUE_BAODIAN_ON;
				break;
		case 0x3B://±£µÁΩ‚≥˝
				mbase_cb->sta &= ~F_RemoteRelayOn;
				RemotCtrResult = CTRL_STATUE_BAODIAN_OFF;
				break;

		default:
				return RS_ERR_PASS;
				break;
	}

	return Err_Byte;
}

unsigned char FrozeCmd_Check(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len = *(p_rx+9);
    
    if((mbase_cb->metertype & F_Password_En) == 0)   //√˜Œƒƒ£ Ω≤ª◊ˆ≈–∂œ
    {
        if(chl == chl_ir_698)    
        {
            if((mbase_cb->infrared_auth_timeout == 0) && (mbase_cb->factor_mode == false))//∫ÏÕ‚Õ®µ¿±ÿ–Î‘⁄∫ÏÕ‚»œ÷§”––ß∆⁄º‰Ω¯––≤Ÿ◊˜°£
            {
                return RS_ERR_PASS;
            }
        }
    }
    
	if( len != 4 )
		return RS_ERR_LEN;
	if( *(p_rx+10) == 0x99 )//MM
	{
		if( *(p_rx+11) == 0x99 )//DD
		{
			if( *(p_rx+12) == 0x99 )//hh
			{
				if( *(p_rx+13) == 0x99 )//ss
				{
					//HisRecord_Write( InstantFroze, START );//À≤ ±∂≥Ω·
					return 0;
				}
				else if( *(p_rx+10) >0x59 )
					return RS_ERR_OTHERS;
			}
			else if( (*(p_rx+11) > 0x23) || (*(p_rx+10) >0x59) )
				return RS_ERR_OTHERS;
		}
		else if( (*(p_rx+12) > 0x31) || (*(p_rx+12) < 0x01) || (*(p_rx+11) > 0x23) || (*(p_rx+10) >0x59) )
			return RS_ERR_OTHERS;
	}
	else if( (*(p_rx+13) > 0x12) || (*(p_rx+13) < 0x01) || (*(p_rx+12) > 0x31) || (*(p_rx+12) < 0x01) || (*(p_rx+11) > 0x23) || (*(p_rx+10) >0x59) )
		return RS_ERR_OTHERS;

	//∂® ±∂≥Ω·
	return 0;
}

signed char ctl_freeze(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
{
    uint8_t Err = 0;
    
    if( (Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0) || (Addr_9_9( p_rx+1 ) == 0) )
    {
        Err = FrozeCmd_Check(instance, chl, p_rx);
        if( Addr_9_9( p_rx+1 ) == 0 )//π„≤•≤ª“™«Û”¶¥
        {
            return -1;
        }
        else
        {
            if( Err == 0 )
                return 0;
            else
                return Err;
        }
    }
    return -1;
}

signed char chg_baut(struct instance_t* instance, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len = *(p_rx+9), Err=0;
    uint32_t s_index=0;
    
    if(Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0)
    {
        if( len == 1 )
            Err = Comm485Baut_Check( *(p_rx+10) );
        else
            Err = RS_ERR_LEN;
        if( Err == 0 )
        {
            switch( *(p_rx+10) )
            {
                case 0x02:  //600
                    mbase_cb->Baut_rs485 = 1;
                    break;
                case 0x04:  //1200
                    mbase_cb->Baut_rs485 = 2;
                    break;
                case 0x08:  //2400
                    mbase_cb->Baut_rs485 = 3;
                    break;
                case 0x10:  //4800
                    mbase_cb->Baut_rs485 = 4;
                    break;
                case 0x20:  //9600
                    mbase_cb->Baut_rs485 = 6;
                    break;
                case 0x40:  //19200
                    mbase_cb->Baut_rs485 = 7;
                    break;
            }
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        offsetof(struct mbase_control_block_t, Baut_rs485));
            nd_set_offset(instance, s_index, (uint8_t*)&mbase_cb->Baut_rs485, 1);
            mbase_cb->baud_chg = 0x01;  //∏¸–¬¥Æø⁄≤®Ãÿ¬ 
        }
        return Err;
    }
    else
        return -1;
}
