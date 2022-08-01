#include "nidavellir.h"
#include "fr_698.h"
#ifndef WIN32
#include "gs_record.h"
#endif // !WIN32


/***********************************************************************/
/*  函数名称：GetMonMaxDay                                             */
/*  函数功能：根据年月计算该月最大天数                                 */
/*  输入参数：Addr：年月数据地址                                       */
/*  输出参数：月最大天数                                               */
/***********************************************************************/
extern unsigned char GetMonMaxDay( unsigned char* Addr );

/***********************************************************************/
/*  函数名称：GetTotalDay                                              */
/*  函数功能：根据年月日计算总天数                                     */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：总天数                                                   */
/***********************************************************************/
extern unsigned int GetTotalDay( unsigned char* Addr );

/***********************************************************************/
/*  函数名称：GetTotalMin                                              */
/*  函数功能：根据年月日时分计算总分数                                 */
/*  输入参数：Addr：年月日时分数据地址                                 */
/*  输出参数：总分数                                                   */
/***********************************************************************/
extern unsigned long GetTotalMin( unsigned char* Addr );


/***********************************************************************/
/*  函数名称：GetTotalSec                                              */
/*  函数功能：根据年月日时分秒计算总秒数                               */
/*  输入参数：Addr：年月日时分秒数据地址                               */
/*  输出参数：总秒数                                                   */
/***********************************************************************/
unsigned long GetTotalSec( unsigned char* Addr )
{
	unsigned long Sec,Min;

	Min = GetTotalMin( Addr+1 );
	Sec = *Addr;
	Sec = Sec + Min*60;
	return Sec;
}

int ACTION_4_Parameter(struct instance_t* instance, uint32_t id, uint8_t* buf, void *ctx)
{
    struct context_698_t* pctx = (struct context_698_t*)ctx;
    struct mbase_control_block_t* mbase_cb;
    struct message_t mesg_tmp;
    struct pipeline_t* pipeline;
    uint8_t  method,i, tmpbuf[20] = {0};
    uint16_t oad,temp=0;
    uint8_t *p = buf;
    int ret;
    uint32_t Temp32A,Temp32B,Temp32C,s_index;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }
    
    pipeline = get_pipeline(instance, pid_meter_base);
    if(pipeline == NULL)
    {
        return -DAR_OTHERS_ERROR;
    }  
    
    switch (oad)
    {
        case 0x4000:
            if(method == 127)//广播校时
            {
                uint8_t timer_set[7],timer[7],time_js[7],timeout=0,year;
                pctx->resp_choice = 1;  //不做回复
                if( 0x1C != *p++ )
                {
                    return -DAR_TYPE_NOT_MATCH;	//类型不匹配
                }
                else
                {
                    if(pctx->security_choice == 0x01)
                    {
                        if( pctx->verification_choice != AUTH_SID_MAC) //密文+SID广播校时(程序不支持)
                        {
                            return -DAR_UNAUTHORIZED;
                        }
                    }
                    else if(pctx->security_choice == 0x00)  //明文广播校时(带安全)(程序不支持)
                    {
                        return -DAR_UNAUTHORIZED;
                    }

                    timer_set[0] = *(p+3); //day
                    timer_set[1] = *(p+2); //month
                    timer_set[3] = *(p+4);  //hour
                    timer_set[4] = *(p+5);  //min
                    timer_set[5] = *(p+6);  //sec
                    if(Time_Format_Check(&timer_set[3]) || Date_Format_Check(timer_set))  return -DAR_OTHERS_ERROR;  //时间非法
                    
                    if((pctx->security_choice != 0x01) && (mbase_cb->boardadjtime == 0xAA))//当日已校时且是明文校时
                    {
                        return -DAR_UNAUTHORIZED;
                    }
                    
                    rtc_get_from_tm(instance,timer);
                    temp = timer[6];
                    temp = (temp << 8) + timer[5];
                    timer[5] = (uint8_t)(temp - 2000);

                    Temp32A = GetTotalSec(timer);   //当前时间
                    
                    timer_set[0] = *(p+6);  //sec
                    timer_set[1] = *(p+5);  //min
                    timer_set[2] = *(p+4);  //hour
                    timer_set[3] = *(p+3);  //day
                    timer_set[4] = *(p+2);  //month
                    temp = *p;
                    temp = (temp << 8) + *(p+1);
                    timer_set[5] = (uint8_t)(temp - 2000);
                    Temp32B = GetTotalSec(timer_set);	//设置时间
                            
                    if( Temp32A >= Temp32B ) Temp32C= Temp32A-Temp32B;
                    else Temp32C = Temp32B-Temp32A;
                    
                    if(pctx->security_choice != 0x01)	//明文广播校时
                    {
                        //------ 广播校时不允许在跨天 -------
                        if(timer_set[3] != timer[3])    return -DAR_UNAUTHORIZED;
                        
                        //最小校时偏差：小于此范围所有校时均不响应，包括明文广播校时、密文广播校时，默认60秒；
                        //最大校时偏差：超过此范围的明文广播校时不响应，密文广播校时响应，默认300秒。
                        if(Temp32C < mbase_cb->Limit_T_Min)  return -DAR_UNAUTHORIZED;
                        
                        if(Temp32C > mbase_cb->Limit_T_Max)  
                        {
                            //记录时钟故障(时钟非法)
                            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBroadcastTimeERR);
                            nd_get_index(instance, s_index, (uint8_t*)&tmpbuf[0], 3);
                            if((tmpbuf[0] != (uint8_t)instance->time.tm_year)||(tmpbuf[1] != (uint8_t)instance->time.tm_mon)||(tmpbuf[2] != (uint8_t)instance->time.tm_mday))
                            {

                                tmpbuf[0] = instance->time.tm_year;
                                tmpbuf[1] = instance->time.tm_mon;
                                tmpbuf[2] = instance->time.tm_mday;
                                
                                s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + LastBroadcastTimeERR);
                                nd_get_index(instance, s_index, (uint8_t*)&tmpbuf[0], 3);
                                mbase_cb->RTCErrS_status = 1;
                            }
                            return -DAR_UNAUTHORIZED;
                        }
                        //------ 不允许在结算日的±5min内广播校时 -------
                        for(i=0; i<3; i++)
                        {
                            s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Balance_Time + i);
                            nd_get_index(instance, s_index, (uint8_t*)&time_js[0], 2);
                            time_js[2] = time_js[1];
                            time_js[3] = time_js[0];
                            time_js[0] = 0;//sec
                            time_js[1] = 0;//min
                            time_js[4] = timer[4];//month
                            time_js[5] = timer[5];//year
                            Temp32C = GetTotalSec(time_js);
                            if( ((Temp32A >= Temp32C)&&(Temp32C >= Temp32B)) || ((Temp32A <= Temp32C)&&(Temp32C <= Temp32B)) )
                            {
                                timeout = 1;
                                break;
                            }
                        }
                        if(timeout == 1)    return -DAR_UNAUTHORIZED;
                        
                    }
                    else    //密文广播校时
                    {
                        //最小校时偏差：小于此范围所有校时均不响应，包括明文广播校时、密文广播校时，默认60秒；
                        if(Temp32C < mbase_cb->Limit_T_Min)  return -DAR_UNAUTHORIZED;
                    }


                    //保存校时前时间 
                    rtc_get_from_tm(instance,tmpbuf);
                    timer_set[0] = tmpbuf[6];
                    timer_set[1] = tmpbuf[5];
                    timer_set[2] = tmpbuf[4];
                    timer_set[3] = tmpbuf[3];
                    timer_set[4] = tmpbuf[2];
                    timer_set[5] = tmpbuf[1];
                    timer_set[6] = tmpbuf[0];
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + BeforSetT_Time);
                    nd_set_index(instance, s_index, timer_set, 7);


                    mem_swap_cpy((uint8_t*)&oad, p, 2);
                    year = oad - 2000;
                    p = p + 2;
                    timer_set[5] = BYTE_BCD(year);//年
                    timer_set[4] = BYTE_BCD(*p++);//月
                    timer_set[3] = BYTE_BCD(*p++);//日
                    timer_set[2] = BYTE_BCD(*p++);//时
                    timer_set[1] = BYTE_BCD(*p++);//分
                    timer_set[0] = BYTE_BCD(*p++);//秒
                    timer_set[6] = GetWeek(&timer[0]);
                    write_mcu_rtc(0, timer_set, 7);
                    read_mcu_rtc(0x00, instance);
                    mbase_cb->boardadjtime = 0xAA;


                    //保存校时后时间 
                    rtc_get_from_tm(instance,tmpbuf);
                    timer_set[0] = tmpbuf[6];
                    timer_set[1] = tmpbuf[5];
                    timer_set[2] = tmpbuf[4];
                    timer_set[3] = tmpbuf[3];
                    timer_set[4] = tmpbuf[2];
                    timer_set[5] = tmpbuf[1];
                    timer_set[6] = tmpbuf[0];
                    s_index = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + AfterSetT_Time);
                    nd_set_index(instance, s_index, timer_set, 7);

                    send_event_mesg_pro(instance, NULL, BroadcastTimeS, NULL, 1);// 广播校时记录

                    record_rollback_after_set_time(instance, 0);
                    clr_last_bill_time(instance);
                    return 8;
                }
            }
            else 
            {
                return -DAR_OBJECT_NOT_EXIST;  //对象不存在
            }
            break;
        case 0x4300:
            switch(method)
            {
                case 0x04://出厂参数初始化
                    if(mbase_cb->factor_mode == false)  return -DAR_UNAUTHORIZED;
                    ret = para_init( instance );
                    meter_clr(instance);
                    if(ret < 0)
                    {
                        return -DAR_OTHERS_ERROR;
                    }
					p = p + 7;
                    return (p-buf);
                    break;
                case 0x03://数据初始化（电表清零）
                    if(mbase_cb->factor_mode == false)
                    {
                        if((1 != pctx->timeTag_valid) || (pctx->timeTag[0] == 0))
                        {
                            return -DAR_TIME_TAG_INVALID;
                        }
                    }

                    if (( mbase_cb->key_state == false ) && (mbase_cb->factor_mode == false))//0私钥，1公钥，私钥不允许清零
                    {
                        return (-DAR_UNAUTHORIZED);
                    }

                    if(mbase_cb->factor_mode == false) 
                    {
                        //set_activeflag(instance, RecordId);
                        mesg_tmp.id = MeterClrS;
                        recv_record_process(instance, NULL, &mesg_tmp, NULL);
                        //send_event_mesg_pro(instance, NULL, MeterClrS, NULL, 1);
                    }
                    meter_clr(instance);
                    p++;


                    return (p-buf);
                    break;
                case 0x05://事件初始化
                    if((1 != pctx->timeTag_valid) || (pctx->timeTag[0] == 0)){
                        return -DAR_TIME_TAG_INVALID;
                    }
                    event_clear_omd_add(instance, id, 0 ); 
                    event_clr(instance);// 事件清零
                    p++;                     
                    send_event_mesg_pro(instance, NULL, EventClrS, NULL, 1);
                    return (p-buf);
                    break;
                default:
                    return -DAR_OBJECT_UNDEF;
                    break;
            }
            break;
        default:
            break;
    }
    return -DAR_OBJECT_UNDEF;
}
