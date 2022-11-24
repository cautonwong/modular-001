#include "nidavellir.h"
#include "645id_table.h"
#include "utils.h"
#include "gs_record.h"
#include "ctl_03_SafeAuthen.h"

extern unsigned char Cmp_RemoteCtlTime( struct instance_t* instance, uint8_t* Addr );

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


    if(( Cmp_MeterAddr( instance, p_rx+1 , 0 ) == 0 ))
    {
        maddr_type = SA_TYPE_SINGLE645;  //å®é™…åœ°å€
    }
    else if(( Cmp_MeterAddr( instance, p_rx+1 , 1 ) == 0 ))
    {
        maddr_type = SA_TYPE_GENERAL645;  //é€šé… 
    }
    else if((Addr_9_9( p_rx+1 ) == 0))
    {
        maddr_type = SA_TYPE_BROADECAST645;  //  99 å¹¿æ’­
    }
    else
    {
        return -1; //ä¸å“åº”
    }

    if((*(p_rx+9) == 6) )
    {
        if((mbase_cb->metertype & F_Password_En) == 0)   //Ã÷ÎÄÄ£Ê½²»×öÅĞ¶Ï
        {
            if(chl == chl_ir_698)    
            {
                if((mbase_cb->infrared_auth_timeout == 0) && (mbase_cb->factor_mode == false))//ºìÍâÍ¨µÀ±ØĞëÔÚºìÍâÈÏÖ¤ÓĞĞ§ÆÚ¼ä½øĞĞ²Ù×÷¡£
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
        if( ( Lng1 > 86100 ) || ( Lng1 < 301 ) )        //±ÜÃâÎçÒ¹ÁãÊ±Ç°ºó5·ÖÖÓĞ£Ê±
        {
            if(maddr_type == SA_TYPE_BROADECAST645)
                return -1;
            else
                return RS_ERR_OTHERS;
        }  
        
        for(i=0;i<3;i++)//±ÜÃâÔÂ½áËãÊ±Ğ£Ê±¡£
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
        for(i=0;i<4;i++)//±ÜÃâÔÂ½áËãÊ±Ğ£Ê±¡£
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
        Buf[0] = instance->time.tm_mday;  //ÈÕ
        Buf[1] = instance->time.tm_mon+1;  //ÔÂ
        Buf[2] = (uint8_t)((instance->time.tm_year + 1900)-2000);  //Äê
        Lng1 = GetTotalDay( Buf );  //µ±Ç°Ê±¼ä
        
        Buf[0] = BCD_BYTE(*(p_rx+13));  //ÈÕ
        Buf[1] = BCD_BYTE(*(p_rx+14));  //ÔÂ
        Buf[2] = BCD_BYTE(*(p_rx+15));  //Äê
        Lng2 = GetTotalDay( Buf );  //¹ã²¥Ğ£Ê±Ê±¼ä
        if( Lng1 == Lng2 )//Ö»ÓĞÔÚÈÕÆÚÒ»ÖÂµÄÇé¿öÏÂ£¬²ÅÔÊĞí½øĞĞ¹ã²¥Ğ£Ê±
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
                Lng2 = GetTotalSec( Buf );  //Ğ£Ê±Ê±¼ä
                
                Buf[0] = instance->time.tm_sec;
                Buf[1] = instance->time.tm_min;
                Buf[2] = instance->time.tm_hour;
                Buf[3] = instance->time.tm_mday;
                Buf[4] = instance->time.tm_mon+1;
                Buf[5] = (uint8_t)((instance->time.tm_year + 1900)-2000);
                
                Lng1 = GetTotalSec( Buf );//µ±Ç°Ê±¼ä
                
                Lng2 = Lng2 - Lng1;
                if( Lng2 & 0x80000000 )
                    Lng2 = ~Lng2 + 1;
                if( (Lng2 <= (5*60)) && (mbase_cb->boardadjtime != 0xAA) )//Ò»ÌìÖ»ÔÊĞí¹ã²¥Ğ£Ê±Ò»´Î£¬Ğ£Ê±·ù¶ÈĞ¡ÓÚ5·ÖÖÓ
                {


                    
                    //ä¿å­˜æ ¡æ—¶å‰æ—¶é—´ 
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


                    //ä¿å­˜æ ¡æ—¶åæ—¶é—´ 
                    rtc_get_from_tm(instance,temp);
                    mem_swap_cpy( &buffer[0], &temp[0], 7);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
                    nd_set_index(instance, s_index, buffer, 7);
                    
                    record_event_no_mesg_pro(instance, NULL, BroadcastTimeS, NULL, 1);// å¹¿æ’­æ ¡æ—¶è®°å½•
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
                if((mbase_cb->metertype & F_Password_En) == 0)   //Ã÷ÎÄÄ£Ê½²»×öÅĞ¶Ï
                {
                    if(chl == chl_ir_698)    
                    {
                        if((mbase_cb->infrared_auth_timeout != 0) && (mbase_cb->factor_mode == false))//ºìÍâÍ¨µÀ±ØĞëÔÚºìÍâÈÏÖ¤ÓĞĞ§ÆÚ¼ä½øĞĞ²Ù×÷¡£
                        {
                            mbase_cb->MulPortMode = (*(p_rx+10));
                            if(F_PortSec != mbase_cb->MulPortMode)    
                            {
                                opreate_second_pluse(3);    //åˆ‡æ¢è‡³å¤šåŠŸèƒ½å£
                            }
                            else
                            {
                                opreate_second_pluse(1);    //ç§’è„‰å†²
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
                        opreate_second_pluse(3);    //åˆ‡æ¢è‡³å¤šåŠŸèƒ½å£
                    }
                    else
                    {
                        opreate_second_pluse(1);    //ç§’è„‰å†²
                    }
                }
            }
            else
                return RS_ERR_OTHERS;
        }
        else
            return RS_ERR_LEN;
        if( Addr_9_9( p_rx+1 ) == 0 )//¹ã²¥²»ÒªÇóÓ¦´ğ
            return -1;
        else
            return 0;
    }
    else
        return -1;
}




//p_rx ÎªÖ¡Í·  ·Ç apdu
signed char meter_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx) 
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len=*(p_rx+9);
    int8_t ret = 0;
    uint8_t len_judge = 8;
    uint8_t TmpBuffer[10] ={0};
    if( Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0 )
    {

        //è·å–å¯†ç 
        memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);
      
        //å®‰å…¨éªŒè¯
        ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
        if(ret < 0)
        {
            return (uint8_t)-ret;
        }
        
        if(( ctx->password[0] != DL645_PA_SC_CIPHER) && ( ctx->password[0] != DL645_PA_PLAIN_02))  //æ”¯æŒå‚å¤–02çº§
        {
            if( mbase_cb->factor_mode != true )
            {
                return RS_ERR_PASS;
            }
        }
        // 68 11 11 11 11 11 11 68 1a 10 98 00 00 00 44 33 22 11 1a 00 99 12 31 23 59 59
        if(ctx->password[0] == DL645_PA_SC_CIPHER)
        {
            //æ—¶æ•ˆåˆ¤æ–­
            if( ctx->apdu_rx_len == 16 )
            {
                ndv_memcopy( TmpBuffer, p_rx+20, 6 );  //è§£å¯†åæ•°æ®  å·²ç»ä¸ºæ­£åº
                if( BCD_Check( TmpBuffer, 6 ) != 0 )
                {
                    return RS_ERR_PASS;
                }
                
                if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
                {
                    return RS_ERR_PASS;
                }           
              
              meter_clr(instance);
            }
                
            else
                return RS_ERR_LEN;
        }
        else  if(ctx->password[0] == DL645_PA_PLAIN_02)
        {
            if( ctx->apdu_rx_len == 8 )
                meter_clr(instance);
            else
                return RS_ERR_LEN;   
        }
        else
        {
            return RS_ERR_LEN;  
        }
        
        return 0;
    }
    else
        return -1;
    
}

signed char event_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len = *(p_rx+9);
    int8_t ret =0;
    uint8_t TmpBuffer[10] = {0};
    uint32_t id_645 = 0;
    
    if( Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0 )
    {
        //è·å–å¯†ç 
        memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);
      
        //å®‰å…¨éªŒè¯
        ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
        if(ret < 0)
        {
            return (uint8_t)-ret;
        }
        
        if(( ctx->password[0] != DL645_PA_SC_CIPHER) && ( ctx->password[0] != DL645_PA_PLAIN_02) && ( ctx->password[0] != DL645_PA_PLAIN_04))  //æ”¯æŒ å‚å¤–02ã€04çº§
        {
            if( mbase_cb->factor_mode != true )
            {
                return RS_ERR_PASS;
            }
        }


        if(ctx->password[0] == DL645_PA_SC_CIPHER)
        {
            //æ—¶æ•ˆåˆ¤æ–­
            if( ctx->apdu_rx_len == 20 )
            {       
                //p_rx+20//å¾…è°ƒæ•´ åˆ†ç›¸ æ¸…é›¶ID    ID å€’å™ ç¬¦åˆ645å¸§
                id_645 = buf_to_uint32_lsb(p_rx+20);
                if (id_645 !=  0xFFFFFFFF) //ä¸æ”¯æŒåˆ†é¡¹æ¸…é›¶
                {
                    return RS_ERR_OTHERS;
                }
                ndv_memcopy( TmpBuffer, p_rx+20+4, 6 );  //è§£å¯†åæ•°æ®  å·²ç»ä¸ºæ­£åº
                if( BCD_Check( TmpBuffer, 6 ) != 0 )     
                {
                    return RS_ERR_PASS;
                }
                
                if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
                {
                    return RS_ERR_PASS;
                }           
              
                event_clr(instance);
            }
            else
                return RS_ERR_LEN;
        }
        else 
        {
            if( ctx->apdu_rx_len == 12 )
                event_clr(instance);
            else
                return RS_ERR_LEN;   
        }

        return 0;
    }
    else    
        return -1;
}

/***********************************************************************/
/*  º¯ÊıÃû³Æ£ºCmp_RemoteCtlTime                                        */
/*  º¯Êı¹¦ÄÜ£ºÅĞ¶ÏÔ¶³Ì¿ØÖÆÃüÁîÊÇ·ñÔÚÃüÁîÓĞĞ§Ê±¼äÄÚÏÂ·¢                 */
/*  ÊäÈë²ÎÊı£ºAddr£ºÊı¾İµØÖ·                                           */
/*  Êä³ö²ÎÊı£º0ÎªÕıÈ·£¬ÆäÓàÎª´íÎó                                      */
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
	Lng1 = GetTotalSec( Buf );  //µ±Ç°Ê±¼ä
    
	mem_swap_cpy( Buf, Addr, 6 );    //ÃüÁîÓĞĞ§½ØÖÁÊ±¼ä
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




signed char CommRemoteCtl_Process(struct instance_t* instance,  uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx,  uint16_t RemotCtrResult)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
	signed char Err_Byte=0;
	unsigned long Lng1,s_index=0;
	unsigned char DataLen, Cmd=0, TmpBuffer[20]={0,};
    uint8_t chl = 0;
    int8_t ret = 0;
    
    chl =  ctx->channel;

    //è·å–å¯†ç 
    memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);
    
    //å®‰å…¨éªŒè¯
    ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
    if(ret < 0)
    {
        return (uint8_t)-ret;
    }

	RemotCtrResult = 0;
	
	if(((*(p_rx+18)==0x1B) || (*(p_rx+18)==0x1C))) //æ˜æ–‡åˆé—¸
	{
	    if( *(p_rx+10) == 0x98 )
	    {
            ndv_memcopy( TmpBuffer, p_rx+20, 6 );  
	    }
	    else
	    {
            mem_swap_cpy( TmpBuffer, p_rx+20, 6 );
	    }

		if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )    //ÅĞ¶ÏÀ­ºÏÕ¢ÃüÁîÊÇ·ñÔÚÃüÁîÓĞĞ§Ê±¼äÄÚÏÂ·¢
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
            if((mbase_cb->metertype & F_Password_En) == 0)   //Ã÷ÎÄÄ£Ê½²»×öÅĞ¶Ï
            {
                if(chl == chl_ir_698)    
                {
                    if((mbase_cb->infrared_auth_timeout == 0))//ºìÍâÍ¨µÀ±ØĞëÔÚºìÍâÈÏÖ¤ÓĞĞ§ÆÚ¼ä½øĞĞ²Ù×÷¡£
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
            if(((mbase_cb->metertype & F_Password_En) == 0) && (mbase_cb->factor_mode == false))   //Ã÷ÎÄÄ£Ê½
            {
                RemotCtrResult = ERR_OTHERR;
                return RS_ERR_PASS;
            }
            mem_swap_cpy( TmpBuffer, p_rx+20, 6 );
            Cmd = *(p_rx+18);
        }
        else if( *(p_rx+10) == 0x98 )
        {
            ndv_memcopy( TmpBuffer, p_rx+20, 6 );  //è§£å¯†åæ•°æ®  å·²ç»ä¸ºæ­£åº
            
            Cmd = *(p_rx+18);
            /*
            if( AuthenTime_Invalid() )//Éí·İÈÏÖ¤Ê§Ğ§
            {
                    RemotCtrResult = ERR_AUTHENTM_OVER;
                    return RS_ERR_PASS;
            }
            //ÃÜÎÄ+MACĞ£Ñé
            HBF_LOOP_LBF( Card.Buffer, (unsigned char*)&Comm.Buff[18], 20 );

            if( ISO7816_WriteData_MAC( CtrFile, 0, 16, ISO7816_ESAM ) )
            {
                Comm_MACWrite_Err();
                RemotCtrResult = ERR_ERRMAC;
                return RS_ERR_PASS;
            }
            DataLen = 8;
            //»ñÈ¡Ã÷ÎÄÊı¾İ
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

		//BCDÂëÅĞ¶Ï
		if( BCD_Check( TmpBuffer, 6 ) != 0 )
		{
			RemotCtrResult = ERR_OTHERR;
			return RS_ERR_PASS;
		}

		//ÅĞ¶ÏÀ­ºÏÕ¢ÃüÁîÊÇ·ñÔÚÃüÁîÓĞĞ§Ê±¼äÄÚÏÂ·¢
		if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
		{
			RemotCtrResult = ERR_CMDTM_INVALID;
			return RS_ERR_PASS;
		}
	}

	//¿ØÖÆÃüÁî½âÎö
	switch( Cmd )
	{
		case 0x1A://ÌøÕ¢
				if( (mbase_cb->sta & F_RemoteRelayOn) == 0 )
				{
                        mbase_cb->sta |= F_RemoteRelayOff;
						mbase_cb->sta &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
						
						mbase_cb->alarmmin = 0;
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Remote_RelayOffTime);//ÌøÕ¢¸æ¾¯
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

		case 0x1B://ºÏÕ¢ÔÊĞí
				mbase_cb->sta &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				mbase_cb->sta &= ~F_RemoteRelayOff;
                
				if(mbase_cb->imaxmin > 0)   mbase_cb->imaxmin = 0;   //´óµçÁ÷ÑÓÊ±£¬Çå³ı´óµçÁ÷ÑÓÊ±Ê±¼ä
                if(mbase_cb->alarmmin > 0)  mbase_cb->alarmmin = 0;     //ÌøÕ¢ÑÓÊ±£¬ÇåÁã
                    
				RemotCtrResult = CTRL_STATUE_RELAYON_EN;
                
				if(mbase_cb->metertype & F_RelayonAuto_En)  //ºÏÕ¢ÔÊĞí×Ô¶¯ºÏÕ¢
                {
					mbase_cb->sta |= F_RemoteRelayOnDirect; //Ö±½ÓºÏÕ¢
                    mbase_cb->sta &= ~F_RemoteRelayOnAllow;
                }
				else
                {
				    mbase_cb->sta |= F_RemoteRelayOnAllow;  //ºÏÕ¢ÔÊĞí
                }
				#if 0//(CPU_CARD_EN) 
				//ÓÃ»§¿¨È¡ÏûÀ­Õ¢±¨¾¯
				if( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.AlarmCredit2) && (MeterPara_Block.MeterPara.AlarmCredit2 != 0) && ((Flag.MeterState & F_Alarmed) == 0) )
				{
					Flag.MeterState |= F_Alarmed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				//ÔÊĞíÓÃ»§¼ÌĞøÍ¸Ö§
				if( (Credit_Block.Credit.Amount <= 0) && ((Flag.MeterState & F_ZeroRelayOffed)==0) )
				{
					Flag.MeterState |= F_ZeroRelayOffed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				#endif
				if(mbase_cb->metertype & F_ExternRelay_En)  //ÍâÖÃ¼ÌµçÆ÷
				{
					mbase_cb->sta |= F_RemoteRelayOnDirect; //Ö±½ÓºÏÕ¢
                    mbase_cb->sta &= ~F_RemoteRelayOnAllow;
				}
				break;
		case 0x1C://Ö±½ÓºÏÕ¢
				mbase_cb->sta &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				mbase_cb->sta &= ~F_RemoteRelayOff;
				
				#if 0//(CPU_CARD_EN) 
				//ÓÃ»§¿¨È¡ÏûÀ­Õ¢±¨¾¯
				if( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.AlarmCredit2) && (MeterPara_Block.MeterPara.AlarmCredit2 != 0) && ((Flag.MeterState & F_Alarmed) == 0) )
				{
					Flag.MeterState |= F_Alarmed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				//ÔÊĞíÓÃ»§¼ÌĞøÍ¸Ö§
				if( (Credit_Block.Credit.Amount <= 0) && ((Flag.MeterState & F_ZeroRelayOffed)==0) )
				{
					Flag.MeterState |= F_ZeroRelayOffed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				#endif
				
				if(mbase_cb->imaxmin > 0)   mbase_cb->imaxmin = 0;   //´óµçÁ÷ÑÓÊ±£¬Çå³ı´óµçÁ÷ÑÓÊ±Ê±¼ä
                if(mbase_cb->alarmmin > 0)  mbase_cb->alarmmin = 0;     //ÌøÕ¢ÑÓÊ±£¬ÇåÁã
                
                mbase_cb->sta |= F_RemoteRelayOnDirect; //Ö±½ÓºÏÕ¢
                mbase_cb->sta &= ~F_RemoteRelayOnAllow;
				RemotCtrResult = CTRL_STATUE_RELAYON;
				break;
		case 0x1D://Ô¤ÌøÕ¢
		case 0x1E:
				if (mbase_cb->sta & F_RemoteRelayOn)    //±£µç
				{
					RemotCtrResult = ERR_RELAYOFF_ALARM_BAODIAN;
					return RS_ERR_PASS;
				}
				else if((mbase_cb->sta & F_RemoteRelayOff) || ((mbase_cb->sta & F_RemoteRelayOnAllow) && (mbase_cb->sta != (F_RemoteGaoJing0+F_RemoteGaoJing1))))   //À­Õ¢×´Ì¬ÏÂ
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
		case 0x2A://±¨¾¯
				mbase_cb->sta |= F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_ON;
				break;
		case 0x2B://±¨¾¯½â³ı
				mbase_cb->sta &= ~F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_OFF;
				break;
		case 0x3A://±£µç
          
                if((mbase_cb->sta & F_RemoteRelayOff) && (mbase_cb->alarmmin > 0))//À­Õ¢ÑÓÊ±¹ı³ÌÖĞ£¬±£µçºóÀ­Õ¢×Ö·ûÏûÊ§£¬µÆÃğ
                {
                    mbase_cb->alarmmin = 0;
                    mbase_cb->sta &= ~F_RemoteRelayOff;
                }
                mbase_cb->sta |= F_RemoteRelayOn;
                mbase_cb->sta &= ~(F_RemoteGaoJing1 + F_RemoteGaoJing0);
                mbase_cb->alarmmin = 0;     //ÌøÕ¢ÑÓÊ±£¬ÇåÁã    
                mbase_cb->imaxmin = 0;   //´óµçÁ÷ÑÓÊ±£¬Çå³ı´óµçÁ÷ÑÓÊ±Ê±¼ä

				if(mbase_cb->metertype & F_RelayonAuto_En)  //ºÏÕ¢ÔÊĞí×Ô¶¯ºÏÕ¢
                {
					mbase_cb->sta |= F_RemoteRelayOnDirect; //Ö±½ÓºÏÕ¢
                    mbase_cb->sta &= ~F_RemoteRelayOnAllow;
                }
				else
                {
				    mbase_cb->sta |= F_RemoteRelayOnAllow;  //ºÏÕ¢ÔÊĞí
                }
                
				RemotCtrResult = CTRL_STATUE_BAODIAN_ON;
				break;
		case 0x3B://±£µç½â³ı
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
    
    if((mbase_cb->metertype & F_Password_En) == 0)   //Ã÷ÎÄÄ£Ê½²»×öÅĞ¶Ï
    {
        if(chl == chl_ir_698)    
        {
            if((mbase_cb->infrared_auth_timeout == 0) && (mbase_cb->factor_mode == false))//ºìÍâÍ¨µÀ±ØĞëÔÚºìÍâÈÏÖ¤ÓĞĞ§ÆÚ¼ä½øĞĞ²Ù×÷¡£
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
					//HisRecord_Write( InstantFroze, START );//Ë²Ê±¶³½á
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

	//¶¨Ê±¶³½á
	return 0;
}

signed char ctl_freeze(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
{
    uint8_t Err = 0;
    
    if( (Cmp_MeterAddr( instance, p_rx+1, 0 ) == 0) || (Addr_9_9( p_rx+1 ) == 0) )
    {
        Err = FrozeCmd_Check(instance, chl, p_rx);
        if( Addr_9_9( p_rx+1 ) == 0 )//¹ã²¥²»ÒªÇóÓ¦´ğ
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
            mbase_cb->baud_chg = 0x01;  //¸üĞÂ´®¿Ú²¨ÌØÂÊ
        }
        return Err;
    }
    else
        return -1;
}
