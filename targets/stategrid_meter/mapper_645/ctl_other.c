#include "targets/stategrid_meter/app/relay_control_adapter.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/app/channels.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "resource_ids.h"
#include "application/adapters/dl698/meter_base_view.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "application/adapters/dl645/fr_645_context.h"
#include "application/adapters/dl645/fr_645_env.h"
#include "application/ports/resource_access.h"
#include "645id_table.h"
#include "components/core/types/utils.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "ctl_03_SafeAuthen.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/app/app_config.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "targets/stategrid_meter/app/tariff/tariff_applications.h"
#include "domain/common/time_format.h"
#include "targets/stategrid_meter/app/resource_blocks.h"

static unsigned char Cmp_RemoteCtlTime( void *instance, uint8_t* Addr );

signed char Broadcast_timeadj(void *instance, uint8_t chl, uint8_t* p_rx)
{
    uint8_t Buf[7] = {0,},i;
    int32_t Lng1=0,Lng2=0;
    uint32_t s_index=0;
    uint16_t TimeSec=0;
    uint8_t maddr_type = 0xFF;
    uint8_t buffer[10]={0},temp[10]={0};


    if(( fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0 ))
    {
        maddr_type = SA_TYPE_SINGLE645;  //实际地址
    }
    else if(( fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 1) == 0 ))
    {
        maddr_type = SA_TYPE_GENERAL645;  //通配 
    }
    else if((fr_645_addr_all(p_rx+1, 0x99) == 0))
    {
        maddr_type = SA_TYPE_BROADECAST645;  //  99 广播
    }
    else
    {
        return -1; //不响应
    }

    if((*(p_rx+9) == 6) )
    {
        if((nd_mb_metertype(instance) & F_Password_En) == 0)   //明文模式不做判断
        {
            if(chl == chl_ir_698)    
            {
                if(((nd_mb_metertype(instance) & F_Password_En) == 0) && (nd_mb_infrared_auth_timeout(instance) == 0) && (nd_mb_factor_mode(instance) == false))//红外通道必须在红外认证有效期间进行操作。
                {
                    if(maddr_type == SA_TYPE_BROADECAST645)
                        return -1;
                    else
                        return RS_ERR_OTHERS;
                }   
            }
        }
        TimeSec = nd_fr698_time(instance)->tm_hour;
        TimeSec = TimeSec*60;
        Lng1 = TimeSec + nd_fr698_time(instance)->tm_min;
        Lng1 = Lng1*60 + nd_fr698_time(instance)->tm_sec;
        if( ( Lng1 > 86100 ) || ( Lng1 < 301 ) )        //避免午夜零时前后5分钟校时
        {
            if(maddr_type == SA_TYPE_BROADECAST645)
                return -1;
            else
                return RS_ERR_OTHERS;
        }  
        
        for(i=0;i<3;i++)//避免月结算时校时。
        {
            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
            nd_get_index(instance, s_index, (uint8_t*)&Buf[2], 2); 
            mem_swap_cpy(Buf, (uint8_t*)&Buf[2], 2);
            if( (Buf[0]>23) || (Buf[1]>28) )
                continue;
            
            if( Buf[1] == nd_fr698_time(instance)->tm_mday )
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
        Buf[0] = nd_fr698_time(instance)->tm_mday;  //日
        Buf[1] = nd_fr698_time(instance)->tm_mon+1;  //月
        Buf[2] = (uint8_t)((nd_fr698_time(instance)->tm_year + 1900)-2000);  //年
        Lng1 = GetTotalDay( Buf );  //当前时间
        
        Buf[0] = BCD_BYTE(*(p_rx+13));  //日
        Buf[1] = BCD_BYTE(*(p_rx+14));  //月
        Buf[2] = BCD_BYTE(*(p_rx+15));  //年
        Lng2 = GetTotalDay( Buf );  //广播校时时间
        if( Lng1 == Lng2 )//只有在日期一致的情况下，才允许进行广播校时
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
                Lng2 = GetTotalSec( Buf );  //校时时间
                
                Buf[0] = nd_fr698_time(instance)->tm_sec;
                Buf[1] = nd_fr698_time(instance)->tm_min;
                Buf[2] = nd_fr698_time(instance)->tm_hour;
                Buf[3] = nd_fr698_time(instance)->tm_mday;
                Buf[4] = nd_fr698_time(instance)->tm_mon+1;
                Buf[5] = (uint8_t)((nd_fr698_time(instance)->tm_year + 1900)-2000);
                
                Lng1 = GetTotalSec( Buf );//当前时间
                
                Lng2 = Lng2 - Lng1;
                if( Lng2 & 0x80000000 )
                    Lng2 = ~Lng2 + 1;
                if( (Lng2 <= (5*60)) && (nd_mb_boardadjtime(instance) != 0xAA) )//一天只允许广播校时一次，校时幅度小于5分钟
                {
                    //保存校时前时间 
                    rtc_get_from_tm(instance,temp);
                    mem_swap_cpy( &buffer[0], &temp[0], 7);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
                    nd_set_index(instance, s_index, buffer, 7);
                    
                    nd_mb_boardadjtime(instance) = 0xAA;
                    Buf[0] = (*(p_rx+10));
                    Buf[1] = (*(p_rx+11));
                    Buf[2] = (*(p_rx+12));
                    Buf[3] = (*(p_rx+13));
                    Buf[4] = (*(p_rx+14));
                    Buf[5] = (*(p_rx+15));
                    write_mcu_rtc(0, Buf, 7);

                    read_mcu_rtc(0x00, nd_fr698_time(instance));

                    //保存校时后时间 
                    rtc_get_from_tm(instance,temp);
                    mem_swap_cpy( &buffer[0], &temp[0], 7);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
                    nd_set_index(instance, s_index, buffer, 7);
                    
                    record_event_no_mesg_pro(instance, NULL, BroadcastTimeS, NULL, 1);// 骞挎挱鏍℃椂璁板綍
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

signed char meteraddr_write(void *instance, uint8_t* p_rx)
{
    uint32_t s_index = 0;    
    
    if( (nd_mb_factor_mode(instance) == true) || (nd_mb_cover_open_s_status(instance) == EVENT_SET) )
    {
        if( (fr_645_addr_all( p_rx+1, 0xAA ) == 0) && (*(p_rx+9) == 6) && (BCD_Check( p_rx+10, 6 ) == 0) )
        {
            memcpy(nd_mb_tsa(instance), p_rx+10, 6);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    nd_mb_offset_tsa());
            nd_set_offset(instance, s_index, nd_mb_tsa(instance), 6); 
            return 0;
        }
        else
            return -1;
    }
    else
        return -1;
}

signed char MulPort_set(void *instance, uint8_t chl, uint8_t* p_rx)
{
    uint8_t len = *(p_rx+9);
    
    if( (fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0) || (fr_645_addr_all(p_rx+1, 0x99) == 0) )
    {
        if( len == 1 )
        {
            if( (*(p_rx+10) == 0) || (*(p_rx+10) == 2) )
            {
                if(chl == chl_ir_698)   //红外
                {
                    if(((nd_mb_metertype(instance) & F_Password_En) == 0) && (nd_mb_infrared_auth_timeout(instance) == 0) && (nd_mb_factor_mode(instance) == false))   //明文模式未开启
                    {
                        return RS_ERR_PASS;
                    }
                }

                nd_mb_mul_port_mode(instance) = (*(p_rx+10));
                if(F_PortSec != nd_mb_mul_port_mode(instance))    
                {
                    opreate_second_pluse(3);   //切换至多功能口
                }
                else
                {
                    opreate_second_pluse(1);    //秒脉冲
                }
                
            }
            else
                return RS_ERR_OTHERS;
        }
        else
            return RS_ERR_LEN;
        if( fr_645_addr_all(p_rx+1, 0x99) == 0 )//广播不要求应答
            return -1;
        else
            return 0;
    }
    else
        return -1;
}




//p_rx 为帧头  非 apdu
signed char dl645_meter_clear(void *instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx) 
{
    uint8_t len=*(p_rx+9);
    int8_t ret = 0;
    uint8_t len_judge = 8;
    uint8_t TmpBuffer[10] ={0};
    if( fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0 )
    {
        if (false == Dl645_IR_chl_Check(instance, 0xFFFFFFFF, ctx->channel))
        {
            return RS_ERR_PASS;        
        }
        //获取密码
        memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);

        //安全验证
        ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
        if(ret < 0)
        {
            return (uint8_t)-ret;
        }
        
        if(( ctx->password[0] != DL645_PA_SC_CIPHER) && ( ctx->password[0] != DL645_PA_PLAIN_02))  //鏀寔鍘傚02绾?
        {
            if( nd_mb_factor_mode(instance) != true )
            {
                return RS_ERR_PASS;
            }
        }
        // 68 11 11 11 11 11 11 68 1a 10 98 00 00 00 44 33 22 11 1a 00 99 12 31 23 59 59
        if(ctx->password[0] == DL645_PA_SC_CIPHER)
        {
#if(DEF_645_PRIVATE_KEY_METERCLR_EN == 0)//私钥不允许清零
            if (( nd_mb_key_state(instance) == false ) && (nd_mb_factor_mode(instance) == false))//0私钥，1公钥，
            {
                return RS_ERR_PASS;
            }
#endif
            //时效判断
            if( ctx->apdu_rx_len == 16 )
            {
                ndv_memcopy( TmpBuffer, p_rx+20, 6 ); //解密后数据  已经为正序
                if( BCD_Check( TmpBuffer, 6 ) != 0 )
                {
                    return RS_ERR_PASS;
                }
                
                if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
                {
                    return RS_ERR_PASS;
                }         
                
                if(nd_mb_factor_mode(instance) == false) 
                {
                    record_event_no_mesg_pro(instance, NULL, MeterClrS, NULL, 1);
                }

                meter_clr(instance);
            }
                
            else
                return RS_ERR_LEN;
        }
        else  if(ctx->password[0] == DL645_PA_PLAIN_02)
        {
            if( ctx->apdu_rx_len == 8 )
            {
                if(nd_mb_factor_mode(instance) == false) 
                {
                    record_event_no_mesg_pro(instance, NULL, MeterClrS, NULL, 1);
                }
                meter_clr(instance);
            }
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

signed char dl645_event_clear(void *instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    uint8_t len = *(p_rx+9);
    int8_t ret =0;
    uint8_t TmpBuffer[10] = {0};
    uint32_t id_645 = 0;
    
    if( fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0 )
    {
        if (false == Dl645_IR_chl_Check(instance, 0xFFFFFFFF, ctx->channel))
        {
            return RS_ERR_PASS;        
        }
        //获取密码
        memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);
      
        //安全验证
        ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
        if(ret < 0)
        {
            return (uint8_t)-ret;
        }
        
        if(( ctx->password[0] != DL645_PA_SC_CIPHER) && ( ctx->password[0] != DL645_PA_PLAIN_02) && ( ctx->password[0] != DL645_PA_PLAIN_04))  //鏀寔 鍘傚02銆?4绾?
        {
            if( nd_mb_factor_mode(instance) != true )
            {
                return RS_ERR_PASS;
            }
        }


        if(ctx->password[0] == DL645_PA_SC_CIPHER)
        {
            //时效判断
            if( ctx->apdu_rx_len == 20 )
            {       
                //p_rx+20//待调整 分相 清零ID    ID 倒叙 符合645帧
                id_645 = buf_to_uint32_lsb(p_rx+20);
                if (id_645 !=  0xFFFFFFFF) //不支持分项清零
                {
                    nd_mb_flag_event_clear(instance) = 0; 
                    return RS_ERR_OTHERS;
                }
                else
                {
                    nd_mb_flag_event_clear(instance) = 0x5a; //总清
                }
                
                ndv_memcopy( TmpBuffer, p_rx+20+4, 6 ); //解密后数据  已经为正序
                if( BCD_Check( TmpBuffer, 6 ) != 0 )     
                {
                    return RS_ERR_PASS;
                }
                
                if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
                {
                    return RS_ERR_PASS;
                }           
                event_clear_omd_add(instance, id_645, 0 );
                event_clr(instance);
                record_event_no_mesg_pro(instance, NULL, EventClrS, NULL, 1);
            }
            else
                return RS_ERR_LEN;
        }
        else 
        {
            if( ctx->apdu_rx_len == 12 )
            {
                id_645 = buf_to_uint32_lsb(p_rx+18);
                if (id_645 !=  0xFFFFFFFF) //不支持分项清零
                {
                    nd_mb_flag_event_clear(instance) = 0; 
                    return RS_ERR_OTHERS;
                }
                else
                {
                    nd_mb_flag_event_clear(instance) = 0x5a; //总清
                }
                event_clear_omd_add(instance, id_645, 0 );
                event_clr(instance);
                record_event_no_mesg_pro(instance, NULL, EventClrS, NULL, 1);
            }    
            else
                return RS_ERR_LEN;   
        }

        return 0;
    }
    else    
        return -1;
}

/***********************************************************************/
/*  函数名称：Cmp_RemoteCtlTime                                        */
/*  函数功能：判断远程控制命令是否在命令有效时间内下发                 */
/*  输入参数：Addr：数据地址                                           */
/*  输出参数：0为正确，其余为错误                                      */
/***********************************************************************/
static unsigned char Cmp_RemoteCtlTime( void *instance, uint8_t* Addr )
{
	unsigned long Lng1,Lng2;
    unsigned char Buf[6]={0,},Buf1[6]={0,},i;
    
    Buf[0] = nd_fr698_time(instance)->tm_sec;
    Buf[1] = nd_fr698_time(instance)->tm_min; //min,hour,day,mon,year
    Buf[2] = nd_fr698_time(instance)->tm_hour;
    Buf[3] = nd_fr698_time(instance)->tm_mday;
    Buf[4] = nd_fr698_time(instance)->tm_mon+1;
    Buf[5] = (unsigned char)(nd_fr698_time(instance)->tm_year+1900 - 2000);
	Lng1 = GetTotalSec( Buf );  //当前时间
    
	mem_swap_cpy( Buf, Addr, 6 );    //命令有效截至时间
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




signed char CommRemoteCtl_Process(void *instance,  uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx,  uint16_t RemotCtrResult)
{
	signed char Err_Byte=0;
	unsigned long Lng1,s_index=0;
	unsigned char DataLen, Cmd=0, TmpBuffer[20]={0,};
    uint8_t chl = 0;
    int8_t ret = 0;
    
    chl =  ctx->channel;

    //获取密码
    memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);
    
    //安全验证
    ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
    if(ret < 0)
    {
        if(nd_mb_auth_timeout_645(instance) == 0)
        {
           RemotCtrResult = ERR_AUTHENTM_OVER;
           return RS_ERR_PASS;
        }
        return (uint8_t)-ret;
    }

	RemotCtrResult = 0;
	
	if(((*(p_rx+18)==0x1B) || (*(p_rx+18)==0x1C))) //明文合闸
	{
	    if( *(p_rx+10) == 0x98 )
	    {
            ndv_memcopy( TmpBuffer, p_rx+20, 6 );  
	    }
	    else
	    {
            mem_swap_cpy( TmpBuffer, p_rx+20, 6 );
	    }

		if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )    //判断拉合闸命令是否在命令有效时间内下发
		{
			RemotCtrResult = ERR_CMDTM_INVALID;
			return RS_ERR_PASS;
		}
        Cmd = *(p_rx+18);
	}
	else
	{
	    if(nd_mb_factor_mode(instance) == false)
	    {
            if((nd_mb_metertype(instance) & F_Password_En) == 0)   //明文模式不做判断
            {
                if(chl == chl_ir_698)    
                {
                    if((nd_mb_infrared_auth_timeout(instance) == 0))//红外通道必须在红外认证有效期间进行操作。
                    {
                        RemotCtrResult = ERR_AUTHENTM_OVER;
                        return RS_ERR_PASS;
                    }
                }
            }

    		if( nd_mb_mac_suspend_state(instance) == true)
    		{
    				RemotCtrResult = ERR_HALT;
    				return RS_ERR_PASS;
    		}
	    }

        if( *(p_rx+10) == 0x02 )
        {
            if(((nd_mb_metertype(instance) & F_Password_En) == 0) && (nd_mb_factor_mode(instance) == false))   //明文模式
            {
                RemotCtrResult = ERR_OTHERR;
                return RS_ERR_PASS;
            }
            mem_swap_cpy( TmpBuffer, p_rx+20, 6 );
            Cmd = *(p_rx+18);
        }
        else if( *(p_rx+10) == 0x98 )
        {
            ndv_memcopy( TmpBuffer, p_rx+20, 6 );  //解密后数据  已经为正序
            
            Cmd = *(p_rx+18);
            /*
            if( AuthenTime_Invalid() )//身份认证失效
            {
                    RemotCtrResult = ERR_AUTHENTM_OVER;
                    return RS_ERR_PASS;
            }
            //密文+MAC校验
            HBF_LOOP_LBF( Card.Buffer, (unsigned char*)&Comm.Buff[18], 20 );

            if( ISO7816_WriteData_MAC( CtrFile, 0, 16, ISO7816_ESAM ) )
            {
                Comm_MACWrite_Err();
                RemotCtrResult = ERR_ERRMAC;
                return RS_ERR_PASS;
            }
            DataLen = 8;
            //获取明文数据
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

		//BCD码判断
		if( BCD_Check( TmpBuffer, 6 ) != 0 )
		{
			RemotCtrResult = ERR_OTHERR;
			return RS_ERR_PASS;
		}

		//判断拉合闸命令是否在命令有效时间内下发
		if( Cmp_RemoteCtlTime( instance, TmpBuffer ) )
		{
			RemotCtrResult = ERR_CMDTM_INVALID;
			return RS_ERR_PASS;
		}
	}

	//控制命令解析
	switch( Cmd )
	{
		case 0x1A://跳闸
				if( (nd_mb_sta(instance) & F_RemoteRelayOn) == 0 )
				{
                        nd_mb_sta(instance) |= F_RemoteRelayOff;
						nd_mb_sta(instance) &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
						nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow; //拉闸 取消合闸允许  GS 20221214
                        
						nd_mb_alarmmin(instance) = 0;
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Remote_RelayOffTime);//跳闸告警
                        nd_get_index(instance, s_index, TmpBuffer, 2);
                        mem_swap_cpy((uint8_t*)&nd_mb_alarmmin(instance), TmpBuffer, 2);
                        
						nd_mb_alarmmin(instance) = nd_mb_alarmmin(instance)*60;

						if(nd_mb_alarmmin(instance) > 0)
						{
							RemotCtrResult = CTRL_STATUE_RELAYOFF_DT;
							if((abs(nd_mb_current_l(instance)) > (nd_mb_i_prot(instance) / 10)) && (nd_mb_i_prot(instance) != 0))
							{
								nd_mb_imaxmin(instance) = nd_mb_prot_time(instance);   
                                nd_mb_imaxmin(instance) = nd_mb_imaxmin(instance)*60;
							}
						}
						else if((abs(nd_mb_current_l(instance)) > (nd_mb_i_prot(instance) / 10)) && (nd_mb_i_prot(instance) != 0))
						{
							nd_mb_imaxmin(instance) = nd_mb_prot_time(instance);   
                            nd_mb_imaxmin(instance) = nd_mb_imaxmin(instance)*60;
							RemotCtrResult = CTRL_STATUE_RELAYOFF_IMAX;
						}
						else
                        {
                            nd_mb_imaxmin(instance) = 0;
							RemotCtrResult = CTRL_STATUE_RELAYOFF;
                        }
				}
				else
				{
					RemotCtrResult = ERR_RELAYOFF_BAODIAN;
					return RS_ERR_PASS;
				}
				break;

		case 0x1B://合闸允许
				nd_mb_sta(instance) &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				nd_mb_sta(instance) &= ~F_RemoteRelayOff;
                
				nd_mb_imaxmin(instance) = 0;   //大电流延时，清除大电流延时时间
                nd_mb_alarmmin(instance) = 0;     //跳闸延时，清零
                    
				RemotCtrResult = CTRL_STATUE_RELAYON_EN;
                
				if(nd_mb_metertype(instance) & F_RelayonAuto_En)  //合闸允许自动合闸
                {
					nd_mb_sta(instance) |= F_RemoteRelayOnDirect; //直接合闸
                    nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
                }
				else
                {
				    nd_mb_sta(instance) |= F_RemoteRelayOnAllow;  //合闸允许
                }
				if(nd_mb_metertype(instance) & F_ExternRelay_En)  //外置继电器
				{
					nd_mb_sta(instance) |= F_RemoteRelayOnDirect; //直接合闸
                    nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
				}
				break;
		case 0x1C://直接合闸
				nd_mb_sta(instance) &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				nd_mb_sta(instance) &= ~F_RemoteRelayOff;
				
				
				if(nd_mb_imaxmin(instance) > 0)   nd_mb_imaxmin(instance) = 0;   //大电流延时，清除大电流延时时间
                if(nd_mb_alarmmin(instance) > 0)  nd_mb_alarmmin(instance) = 0;     //跳闸延时，清零
                
                nd_mb_sta(instance) |= F_RemoteRelayOnDirect; //直接合闸
                nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
				RemotCtrResult = CTRL_STATUE_RELAYON;
				break;
		case 0x1D://预跳闸
		case 0x1E:
				if (nd_mb_sta(instance) & F_RemoteRelayOn)    //保电
				{
					RemotCtrResult = ERR_RELAYOFF_ALARM_BAODIAN;
					return RS_ERR_PASS;
				}
				else if((nd_mb_sta(instance) & F_RemoteRelayOff) || ((nd_mb_sta(instance) & F_RemoteRelayOnAllow) && (nd_mb_sta(instance) != (F_RemoteGaoJing0+F_RemoteGaoJing1))))   //拉闸状态下
				{
					RemotCtrResult = ERR_RELAYOFF_ALARM_RELAYOFF;
					return RS_ERR_PASS;
				}
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
							nd_mb_sta(instance) |= F_RemoteGaoJing0;
							nd_mb_sta(instance) &= ~F_RemoteGaoJing1;
						}
						else if(Cmd == 0x1E)
						{
							nd_mb_sta(instance) |= F_RemoteGaoJing1;
							nd_mb_sta(instance) &= ~F_RemoteGaoJing0;	
						}
						nd_mb_limitmin(instance) = *(p_rx+19)*60*5;

						nd_mb_alarmmin(instance) = 0;
						if((abs(nd_mb_current_l(instance)) > (nd_mb_i_prot(instance) / 10)) && (nd_mb_i_prot(instance) != 0))
                        {
                            nd_mb_imaxmin(instance) = nd_mb_prot_time(instance);   
                            nd_mb_imaxmin(instance) = nd_mb_imaxmin(instance)*60;
                            RemotCtrResult = CTRL_STATUE_RELAYOFF_IMAX;
                        }
						else
							RemotCtrResult = CTRL_STATUE_RELAYOFF_ALARM;
					}
				}
				break;
		case 0x2A://报警
				nd_mb_sta(instance) |= F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_ON;
				break;
		case 0x2B://报警解除
				nd_mb_sta(instance) &= ~F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_OFF;
				break;
		case 0x3A://保电
          
                nd_mb_sta(instance) |= F_RemoteRelayOn;
                
                nd_mb_alarmmin(instance) = 0;     //跳闸延时，清零    
                nd_mb_imaxmin(instance) = 0;   //大电流延时，清除大电流延时时间
                nd_mb_limitmin(instance) = 0;
				if(nd_mb_metertype(instance) & (F_RelayonAuto_En + F_ExternRelay_En))  //合闸允许自动合闸   、外置表自动合闸
                {
					nd_mb_sta(instance) |= F_RemoteRelayOnDirect; //直接合闸
                    nd_mb_sta(instance) &= ~F_RemoteRelayOnAllow;
                }
				else
                {
                    if((nd_mb_sta(instance) & (F_RemoteRelayOff + F_RemoteGaoJing1 + F_RemoteGaoJing0))) //只有在拉闸状态下才需要合闸允许
				        nd_mb_sta(instance) |= F_RemoteRelayOnAllow;  //合闸允许
                }

                nd_mb_sta(instance) &= ~(F_RemoteRelayOff + F_RemoteGaoJing1 + F_RemoteGaoJing0);//最后取消拉闸命令
				RemotCtrResult = CTRL_STATUE_BAODIAN_ON;
				break;
		case 0x3B://保电解除
				nd_mb_sta(instance) &= ~F_RemoteRelayOn;
				RemotCtrResult = CTRL_STATUE_BAODIAN_OFF;
				break;

		default:
				return RS_ERR_PASS;
				break;
	}
    save_relay_state(instance);
	return Err_Byte;
}

unsigned char FrozeCmd_Check(void *instance, uint8_t chl, uint8_t* p_rx)
{
    uint8_t len = *(p_rx+9);
    
    if((nd_mb_metertype(instance) & F_Password_En) == 0)   //明文模式不做判断
    {
        if(chl == chl_ir_698)    
        {
            if((nd_mb_infrared_auth_timeout(instance) == 0) && (nd_mb_factor_mode(instance) == false))//红外通道必须在红外认证有效期间进行操作。
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
					//HisRecord_Write( InstantFroze, START );//瞬时冻结
                    nd_mb_trigger_insfreeze_delay_sec(instance) = 1;
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

	//定时冻结
	return 0;
}

signed char ctl_freeze(void *instance, uint8_t chl, uint8_t* p_rx)
{
    uint8_t Err = 0;
    
    if( (fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0) || (fr_645_addr_all(p_rx+1, 0x99) == 0) )
    {
        Err = FrozeCmd_Check(instance, chl, p_rx);
        if( fr_645_addr_all(p_rx+1, 0x99) == 0 )//广播不要求应答
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

signed char chg_baut(void *instance, uint8_t* p_rx, struct context_645_t* ctx)
{
    uint8_t len = *(p_rx+9), Err=0;
    uint32_t s_index=0;
    
    if(fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0)
    {
        if (false == Dl645_IR_chl_Check(instance, 0xFFFFFFFF, ctx->channel))
        {
            return RS_ERR_PASS;        
        }

        
        if( len == 1 )
            Err = Comm485Baut_Check( *(p_rx+10) );
        else
            Err = RS_ERR_LEN;
        if( Err == 0 )
        {
            switch( *(p_rx+10) )
            {
                case 0x02:  //600
                    nd_mb_baut_rs485(instance) = 1;
                    break;
                case 0x04:  //1200
                    nd_mb_baut_rs485(instance) = 2;
                    break;
                case 0x08:  //2400
                    nd_mb_baut_rs485(instance) = 3;
                    break;
                case 0x10:  //4800
                    nd_mb_baut_rs485(instance) = 4;
                    break;
                case 0x20:  //9600
                    nd_mb_baut_rs485(instance) = 6;
                    break;
                case 0x40:  //19200
                    nd_mb_baut_rs485(instance) = 7;
                    break;
            }
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        nd_mb_offset_baut_rs485());
            nd_set_offset(instance, s_index, (uint8_t*)&nd_mb_baut_rs485(instance), 1);
            nd_mb_baud_chg(instance) = 0x01;  //更新串口波特率
        }
        return Err;
    }
    else
        return -1;
}
