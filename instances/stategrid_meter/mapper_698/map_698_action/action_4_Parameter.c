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
    struct mbase_control_block_t* mbase_cb;
    struct pipeline_t* pipeline;
    uint8_t  method,dar=0,i;
    uint16_t oad,temp=0;
    uint8_t *p = buf;
    int ret;
    uint8_t Hour, Min, Sec;
    uint32_t Temp32A,Temp32B,Temp32C;
    
    oad = (unsigned short)((id & 0xFFFF0000) >> 16);
    method = (unsigned char)((id & 0x0000FF00) >> 8);    //方法
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    if(mbase_cb == NULL)
    {
        return -1;
    }
    
    pipeline = get_pipeline(instance, pid_meter_base);
    if(pipeline == NULL)
    {
        return -1;
    }  
    
    switch (oad)
    {
        case 0x4000:
            if(method == 127)//广播校时
            {
                uint8_t timer[7];
                if( 0x1C != *p++ )
                {
                    return -DAR_TYPE_NOT_MATCH;	//类型不匹配
                }
                else
                {
                  /*
                    if(Security_Type == 0x01)
                    {
                        if(Security_Mode == 0x80) Uart[Ch].Buf[ 8] = 0x88; //密文+SID_MAC广播校时
                        else Uart[Ch].Buf[ 8] = 0x89;//密文+SID广播校时(程序不支持)
                    }
                    else if(Security_Type == 0x00) Uart[Ch].Buf[ 8] = 0x89; //明文广播校时(带安全)(程序不支持)
                    else Uart[Ch].Buf[ 8] = 0x08;
*/
                    timer[0] = *(p+3); //day
                    timer[1] = *(p+2); //month
                    timer[3] = *(p+4);  //hour
                    timer[4] = *(p+5);  //min
                    timer[5] = *(p+6);  //sec
                    if(Time_Format_Check(&timer[3]) || Date_Format_Check(timer))  return -1;  //时间非法
                    
                    //if()//当日已校时且是明文校时
                            
                    Hour = ((mbase_cb->Limit_T_Max)/60)/60;
                    Min = ((mbase_cb->Limit_T_Max)/60)%60;
                    Sec = (mbase_cb->Limit_T_Max)%60;
                    
                    rtc_get_from_tm(instance,timer);
                    temp = timer[6];
                    temp = (temp << 8) + timer[5];
                    timer[5] = (uint8_t)(temp - 2000);

                    Temp32A = GetTotalSec(timer);   //当前时间
                    
                    timer[0] = *(p+6);  //sec
                    timer[1] = *(p+5);  //min
                    timer[2] = *(p+4);  //hour
                    timer[3] = *(p+3);  //day
                    timer[4] = *(p+2);  //month
                    temp = *p;
                    temp = (temp << 8) + *(p+1);
                    timer[5] = (uint8_t)(temp - 2000);
                    Temp32B = GetTotalSec(timer);	//设置时间
                            
                    if( Temp32A >= Temp32B ) Temp32C= Temp32A-Temp32B;
                    else Temp32C = Temp32B-Temp32A;
                    /*
                    if( Uart[Ch].Buf[8] == 0x08 )	//明文广播校时
                    {
                    
                    }
                    else
                    {
                    
                    }*/
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
                    ret = para_init( instance );
                    meter_clr(instance);
                    if(ret < 0)
                    {
                        return -1;
                    }
					p = p + 7;
                    return (p-buf);
                    break;
                case 0x03://数据初始化（电表清零）
        
                    // 时间标签 标志  待调整
#if 0
                    if ((mbase_cb->time_tag == false) && (mbase_cb->factor_mode == false))
                    {
                        return (1-DAR_TIME_TAG_INVALID);
                    }
#endif
#if 0
                    if ((mbase_cb->SecretKey_state == 0) && (mbase_cb->factor_mode == false))//0私钥，1公钥，私钥不允许清零
                    {
                        return (1 - DAR_UNAUTHORIZED);
                    }
#endif
                    
                    meter_clr(instance);
                    p++;
                    send_event_mesg_pro(instance, NULL, MeterClrS, NULL, 1);
                    return (p-buf);
                    break;
                case 0x05://事件初始化
#if 0
                    if ((mbase_cb->time_tag == false) && (mbase_cb->factor_mode == false))
                    {
                        return (1 - DAR_TIME_TAG_INVALID);
                    }
#endif
                    event_clear_omd_add(instance, id, 0 /*req->channel */); //待调整    如何在map  case中  取到通道号
                    event_clr(instance);// 事件清零
					p++;
                    send_event_mesg_pro(instance, NULL, EventClrS, NULL, 1);
                    return (p-buf);
                    break;
                default:
                    dar = DAR_OBJECT_NOT_EXIST;
                    break;
            }
            break;
        default:
            break;
    }
    return 0x01;
}
