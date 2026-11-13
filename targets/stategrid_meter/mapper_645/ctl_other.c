#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "application/adapters/dl645/fr_645_context.h"
#include "645id_table.h"
#include "utils.h"
#include "gs_record.h"
#include "ctl_03_SafeAuthen.h"
#include "cpu_surport.h"
#include "application/apps/app_config.h"
#include "domain/events/legacy/gs_record.h"
#include "domain/tariff/legacy/tariff_applications.h"
#include "infra/kernel/resource_blocks.h"
#include "kernel/legacy_control_block.h"

extern unsigned char Cmp_RemoteCtlTime( struct instance_t* instance, uint8_t* Addr );
extern void save_relay_state(struct instance_t* instance);

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
        if((nd_mbase_metertype(mbase_cb) & F_Password_En) == 0)   //明文模式不做判断
        {
            if(chl == chl_ir_698)    
            {
                if(((nd_mbase_metertype(mbase_cb) & F_Password_En) == 0) && (nd_mbase_infrared_auth_timeout(mbase_cb) == 0) && (nd_mbase_factor_mode(mbase_cb) == false))//红外通道必须在红外认证有效期间进行操作。
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
        for(i=0;i<4;i++)//避免月结算时校时。
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
        Buf[0] = instance->time.tm_mday;  //日
        Buf[1] = instance->time.tm_mon+1;  //月
        Buf[2] = (uint8_t)((instance->time.tm_year + 1900)-2000);  //年
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
                
                Buf[0] = instance->time.tm_sec;
                Buf[1] = instance->time.tm_min;
                Buf[2] = instance->time.tm_hour;
                Buf[3] = instance->time.tm_mday;
                Buf[4] = instance->time.tm_mon+1;
                Buf[5] = (uint8_t)((instance->time.tm_year + 1900)-2000);
                
                Lng1 = GetTotalSec( Buf );//当前时间
                
                Lng2 = Lng2 - Lng1;
                if( Lng2 & 0x80000000 )
                    Lng2 = ~Lng2 + 1;
                if( (Lng2 <= (5*60)) && (nd_mbase_boardadjtime(mbase_cb) != 0xAA) )//一天只允许广播校时一次，校时幅度小于5分钟
                {
                    //保存校时前时间 
                    rtc_get_from_tm(instance,temp);
                    mem_swap_cpy( &buffer[0], &temp[0], 7);
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
                    nd_set_index(instance, s_index, buffer, 7);
                    
                    nd_mbase_boardadjtime(mbase_cb) = 0xAA;
                    Buf[0] = (*(p_rx+10));
                    Buf[1] = (*(p_rx+11));
                    Buf[2] = (*(p_rx+12));
                    Buf[3] = (*(p_rx+13));
                    Buf[4] = (*(p_rx+14));
                    Buf[5] = (*(p_rx+15));
                    write_mcu_rtc(0, Buf, 7);

                    read_mcu_rtc(0x00, instance);

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

signed char meteraddr_write(struct instance_t* instance, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint32_t s_index = 0;    
    
    if( (nd_mbase_factor_mode(mbase_cb) == true) || (nd_mbase_CoverOpenS_status(mbase_cb) == EVENT_SET) )
    {
        if( (fr_645_addr_all( p_rx+1, 0xAA ) == 0) && (*(p_rx+9) == 6) && (BCD_Check( p_rx+10, 6 ) == 0) )
        {
            memcpy(nd_mbase_tsa(mbase_cb), p_rx+10, 6);
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                    nd_mbase_offset_tsa());
            nd_set_offset(instance, s_index, nd_mbase_tsa(mbase_cb), 6); 
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
    
    if( (fr_645_addr_match(p_rx+1, nd_fr645_tsa(instance), 0) == 0) || (fr_645_addr_all(p_rx+1, 0x99) == 0) )
    {
        if( len == 1 )
        {
            if( (*(p_rx+10) == 0) || (*(p_rx+10) == 2) )
            {
                if(chl == chl_ir_698)   //红外
                {
                    if(((nd_mbase_metertype(mbase_cb) & F_Password_En) == 0) && (nd_mbase_infrared_auth_timeout(mbase_cb) == 0) && (nd_mbase_factor_mode(mbase_cb) == false))   //明文模式未开启
                    {
                        return RS_ERR_PASS;
                    }
                }

                nd_mbase_MulPortMode(mbase_cb) = (*(p_rx+10));
                if(F_PortSec != nd_mbase_MulPortMode(mbase_cb))    
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
signed char dl645_meter_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx) 
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
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
            if( nd_mbase_factor_mode(mbase_cb) != true )
            {
                return RS_ERR_PASS;
            }
        }
        // 68 11 11 11 11 11 11 68 1a 10 98 00 00 00 44 33 22 11 1a 00 99 12 31 23 59 59
        if(ctx->password[0] == DL645_PA_SC_CIPHER)
        {
#if(DEF_645_PRIVATE_KEY_METERCLR_EN == 0)//私钥不允许清零
            if (( nd_mbase_key_state(mbase_cb) == false ) && (nd_mbase_factor_mode(mbase_cb) == false))//0私钥，1公钥，
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
                
                if(nd_mbase_factor_mode(mbase_cb) == false) 
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
                if(nd_mbase_factor_mode(mbase_cb) == false) 
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

signed char dl645_event_clear(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
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
            if( nd_mbase_factor_mode(mbase_cb) != true )
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
                    nd_mbase_flag_event_clear(mbase_cb) = 0; 
                    return RS_ERR_OTHERS;
                }
                else
                {
                    nd_mbase_flag_event_clear(mbase_cb) = 0x5a; //总清
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
                    nd_mbase_flag_event_clear(mbase_cb) = 0; 
                    return RS_ERR_OTHERS;
                }
                else
                {
                    nd_mbase_flag_event_clear(mbase_cb) = 0x5a; //总清
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

    //获取密码
    memcpy((unsigned char*)&ctx->password[0], p_rx+10, 4);
    
    //安全验证
    ret = security_judge_645(instance, (p_rx+10), p_tx, tx_len, ctx);
    if(ret < 0)
    {
        if(nd_mbase_auth_timeout_645(mbase_cb) == 0)
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
	    if(nd_mbase_factor_mode(mbase_cb) == false)
	    {
            if((nd_mbase_metertype(mbase_cb) & F_Password_En) == 0)   //明文模式不做判断
            {
                if(chl == chl_ir_698)    
                {
                    if((nd_mbase_infrared_auth_timeout(mbase_cb) == 0))//红外通道必须在红外认证有效期间进行操作。
                    {
                        RemotCtrResult = ERR_AUTHENTM_OVER;
                        return RS_ERR_PASS;
                    }
                }
            }

    		if( nd_mbase_mac_suspend_state(mbase_cb) == true)
    		{
    				RemotCtrResult = ERR_HALT;
    				return RS_ERR_PASS;
    		}
	    }

        if( *(p_rx+10) == 0x02 )
        {
            if(((nd_mbase_metertype(mbase_cb) & F_Password_En) == 0) && (nd_mbase_factor_mode(mbase_cb) == false))   //明文模式
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
				if( (nd_mbase_sta(mbase_cb) & F_RemoteRelayOn) == 0 )
				{
                        nd_mbase_sta(mbase_cb) |= F_RemoteRelayOff;
						nd_mbase_sta(mbase_cb) &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
						nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOnAllow; //拉闸 取消合闸允许  GS 20221214
                        
						nd_mbase_alarmmin(mbase_cb) = 0;
                        s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Remote_RelayOffTime);//跳闸告警
                        nd_get_index(instance, s_index, TmpBuffer, 2);
                        mem_swap_cpy((uint8_t*)&nd_mbase_alarmmin(mbase_cb), TmpBuffer, 2);
                        
						nd_mbase_alarmmin(mbase_cb) = nd_mbase_alarmmin(mbase_cb)*60;

						if(nd_mbase_alarmmin(mbase_cb) > 0)
						{
							RemotCtrResult = CTRL_STATUE_RELAYOFF_DT;
							if((abs(nd_mbase_current_l(mbase_cb)) > (nd_mbase_I_Prot(mbase_cb) / 10)) && (nd_mbase_I_Prot(mbase_cb) != 0))
							{
								nd_mbase_imaxmin(mbase_cb) = nd_mbase_Prot_time(mbase_cb);   
                                nd_mbase_imaxmin(mbase_cb) = nd_mbase_imaxmin(mbase_cb)*60;
							}
						}
						else if((abs(nd_mbase_current_l(mbase_cb)) > (nd_mbase_I_Prot(mbase_cb) / 10)) && (nd_mbase_I_Prot(mbase_cb) != 0))
						{
							nd_mbase_imaxmin(mbase_cb) = nd_mbase_Prot_time(mbase_cb);   
                            nd_mbase_imaxmin(mbase_cb) = nd_mbase_imaxmin(mbase_cb)*60;
							RemotCtrResult = CTRL_STATUE_RELAYOFF_IMAX;
						}
						else
                        {
                            nd_mbase_imaxmin(mbase_cb) = 0;
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
				nd_mbase_sta(mbase_cb) &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOff;
                
				nd_mbase_imaxmin(mbase_cb) = 0;   //大电流延时，清除大电流延时时间
                nd_mbase_alarmmin(mbase_cb) = 0;     //跳闸延时，清零
                    
				RemotCtrResult = CTRL_STATUE_RELAYON_EN;
                
				if(nd_mbase_metertype(mbase_cb) & F_RelayonAuto_En)  //合闸允许自动合闸
                {
					nd_mbase_sta(mbase_cb) |= F_RemoteRelayOnDirect; //直接合闸
                    nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOnAllow;
                }
				else
                {
				    nd_mbase_sta(mbase_cb) |= F_RemoteRelayOnAllow;  //合闸允许
                }
				#if 0//(CPU_CARD_EN) 
				//用户卡取消拉闸报警
				if( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.AlarmCredit2) && (MeterPara_Block.MeterPara.AlarmCredit2 != 0) && ((Flag.MeterState & F_Alarmed) == 0) )
				{
					Flag.MeterState |= F_Alarmed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				//允许用户继续透支
				if( (Credit_Block.Credit.Amount <= 0) && ((Flag.MeterState & F_ZeroRelayOffed)==0) )
				{
					Flag.MeterState |= F_ZeroRelayOffed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				#endif
				if(nd_mbase_metertype(mbase_cb) & F_ExternRelay_En)  //外置继电器
				{
					nd_mbase_sta(mbase_cb) |= F_RemoteRelayOnDirect; //直接合闸
                    nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOnAllow;
				}
				break;
		case 0x1C://直接合闸
				nd_mbase_sta(mbase_cb) &= ~(F_RemoteGaoJing0+F_RemoteGaoJing1);
				nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOff;
				
				#if 0//(CPU_CARD_EN) 
				//用户卡取消拉闸报警
				if( (Credit_Block.Credit.Amount <= MeterPara_Block.MeterPara.AlarmCredit2) && (MeterPara_Block.MeterPara.AlarmCredit2 != 0) && ((Flag.MeterState & F_Alarmed) == 0) )
				{
					Flag.MeterState |= F_Alarmed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				//允许用户继续透支
				if( (Credit_Block.Credit.Amount <= 0) && ((Flag.MeterState & F_ZeroRelayOffed)==0) )
				{
					Flag.MeterState |= F_ZeroRelayOffed;
					E2P_Write( MeterState_Flag, (unsigned char*)&Flag.MeterState, 1 );
				}
				#endif
				
				if(nd_mbase_imaxmin(mbase_cb) > 0)   nd_mbase_imaxmin(mbase_cb) = 0;   //大电流延时，清除大电流延时时间
                if(nd_mbase_alarmmin(mbase_cb) > 0)  nd_mbase_alarmmin(mbase_cb) = 0;     //跳闸延时，清零
                
                nd_mbase_sta(mbase_cb) |= F_RemoteRelayOnDirect; //直接合闸
                nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOnAllow;
				RemotCtrResult = CTRL_STATUE_RELAYON;
				break;
		case 0x1D://预跳闸
		case 0x1E:
				if (nd_mbase_sta(mbase_cb) & F_RemoteRelayOn)    //保电
				{
					RemotCtrResult = ERR_RELAYOFF_ALARM_BAODIAN;
					return RS_ERR_PASS;
				}
				else if((nd_mbase_sta(mbase_cb) & F_RemoteRelayOff) || ((nd_mbase_sta(mbase_cb) & F_RemoteRelayOnAllow) && (nd_mbase_sta(mbase_cb) != (F_RemoteGaoJing0+F_RemoteGaoJing1))))   //拉闸状态下
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
							nd_mbase_sta(mbase_cb) |= F_RemoteGaoJing0;
							nd_mbase_sta(mbase_cb) &= ~F_RemoteGaoJing1;
						}
						else if(Cmd == 0x1E)
						{
							nd_mbase_sta(mbase_cb) |= F_RemoteGaoJing1;
							nd_mbase_sta(mbase_cb) &= ~F_RemoteGaoJing0;	
						}
						nd_mbase_limitmin(mbase_cb) = *(p_rx+19)*60*5;

						nd_mbase_alarmmin(mbase_cb) = 0;
						if((abs(nd_mbase_current_l(mbase_cb)) > (nd_mbase_I_Prot(mbase_cb) / 10)) && (nd_mbase_I_Prot(mbase_cb) != 0))
                        {
                            nd_mbase_imaxmin(mbase_cb) = nd_mbase_Prot_time(mbase_cb);   
                            nd_mbase_imaxmin(mbase_cb) = nd_mbase_imaxmin(mbase_cb)*60;
                            RemotCtrResult = CTRL_STATUE_RELAYOFF_IMAX;
                        }
						else
							RemotCtrResult = CTRL_STATUE_RELAYOFF_ALARM;
					}
				}
				break;
		case 0x2A://报警
				nd_mbase_sta(mbase_cb) |= F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_ON;
				break;
		case 0x2B://报警解除
				nd_mbase_sta(mbase_cb) &= ~F_RemoteAlarm;
				RemotCtrResult = CTRL_STATUE_ALARM_OFF;
				break;
		case 0x3A://保电
          
                nd_mbase_sta(mbase_cb) |= F_RemoteRelayOn;
                
                nd_mbase_alarmmin(mbase_cb) = 0;     //跳闸延时，清零    
                nd_mbase_imaxmin(mbase_cb) = 0;   //大电流延时，清除大电流延时时间
                nd_mbase_limitmin(mbase_cb) = 0;
				if(nd_mbase_metertype(mbase_cb) & (F_RelayonAuto_En + F_ExternRelay_En))  //合闸允许自动合闸   、外置表自动合闸
                {
					nd_mbase_sta(mbase_cb) |= F_RemoteRelayOnDirect; //直接合闸
                    nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOnAllow;
                }
				else
                {
                    if((nd_mbase_sta(mbase_cb) & (F_RemoteRelayOff + F_RemoteGaoJing1 + F_RemoteGaoJing0))) //只有在拉闸状态下才需要合闸允许
				        nd_mbase_sta(mbase_cb) |= F_RemoteRelayOnAllow;  //合闸允许
                }

                nd_mbase_sta(mbase_cb) &= ~(F_RemoteRelayOff + F_RemoteGaoJing1 + F_RemoteGaoJing0);//最后取消拉闸命令
				RemotCtrResult = CTRL_STATUE_BAODIAN_ON;
				break;
		case 0x3B://保电解除
				nd_mbase_sta(mbase_cb) &= ~F_RemoteRelayOn;
				RemotCtrResult = CTRL_STATUE_BAODIAN_OFF;
				break;

		default:
				return RS_ERR_PASS;
				break;
	}
    save_relay_state(instance);
	return Err_Byte;
}

unsigned char FrozeCmd_Check(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t len = *(p_rx+9);
    
    if((nd_mbase_metertype(mbase_cb) & F_Password_En) == 0)   //明文模式不做判断
    {
        if(chl == chl_ir_698)    
        {
            if((nd_mbase_infrared_auth_timeout(mbase_cb) == 0) && (nd_mbase_factor_mode(mbase_cb) == false))//红外通道必须在红外认证有效期间进行操作。
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
                    nd_mbase_trigger_insfreeze_delay_sec(mbase_cb) = 1;
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

signed char ctl_freeze(struct instance_t* instance, uint8_t chl, uint8_t* p_rx)
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

signed char chg_baut(struct instance_t* instance, uint8_t* p_rx, struct context_645_t* ctx)
{
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
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
                    nd_mbase_Baut_rs485(mbase_cb) = 1;
                    break;
                case 0x04:  //1200
                    nd_mbase_Baut_rs485(mbase_cb) = 2;
                    break;
                case 0x08:  //2400
                    nd_mbase_Baut_rs485(mbase_cb) = 3;
                    break;
                case 0x10:  //4800
                    nd_mbase_Baut_rs485(mbase_cb) = 4;
                    break;
                case 0x20:  //9600
                    nd_mbase_Baut_rs485(mbase_cb) = 6;
                    break;
                case 0x40:  //19200
                    nd_mbase_Baut_rs485(mbase_cb) = 7;
                    break;
            }
            s_index = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_static_eprom << 16) + 
                        nd_mbase_offset_Baut_rs485());
            nd_set_offset(instance, s_index, (uint8_t*)&nd_mbase_Baut_rs485(mbase_cb), 1);
            nd_mbase_baud_chg(mbase_cb) = 0x01;  //更新串口波特率
        }
        return Err;
    }
    else
        return -1;
}
