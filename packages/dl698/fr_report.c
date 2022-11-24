#include "nidavellir.h"
#include "fr_698.h"
#include "pin_function_def.h"
#include "cpu_surport.h"
#include "lib_clk.h"
#include "lib_uart.h"


extern uint8_t DEVICE_ERR;
extern uint8_t report_2015state_update(struct instance_t* instance, uint32_t mask);
extern void report_3320list_add(struct instance_t* instance,uint32_t oad);
extern uint8_t active_report_3320list_confirmed(struct instance_t* instance);
extern uint8_t report_3320list_confirmed(struct instance_t* instance, uint8_t cnl, uint32_t oad, uint8_t confirm_SUB_event);
extern uint8_t report_3320list_confirmed_All_channel(struct instance_t* instance, uint32_t oad);
extern int fr_handle_report_response(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern void EventOut_698_check(struct instance_t* instance);

extern uint8_t report_channel_check(struct instance_t* instance, uint16_t channel);
extern uint32_t follow_report_3320List_Latest_oad_get(struct instance_t* instance, uint8_t chn);
extern void report_port_open(struct instance_t* instance);


extern uint8_t read_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t mode);
#if 0
uint8_t rep_data[72] = {
0x88, 0x02, 0x00, 0x01, 0x30, 0x11, 0x02, 0x01, 0x07, 0x00, 
0x20, 0x22, 0x02, 0x00, 0x00, 0x20, 0x1e, 0x02, 0x00, 0x00, 
0x20, 0x20, 0x02, 0x00, 0x00, 0x20, 0x24, 0x02, 0x00, 0x00, 
0x33, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x22, 0x01, 0x00, 
0x00, 0x20, 0x22, 0x01, 0x01, 0x01, 0x06, 0x00, 0x00, 0x00, 
0x04, 0x1c, 0x07, 0xe5, 0x04, 0x18, 0x14, 0x27, 0x20, 0x00, 
0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 
0x00, 0x00
};

int _fr_active_report_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t get_data)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    if(get_data == 0){
        return 72;
    }
    if(get_data == 1){
        ringbuffer_clear(ctx->apdu);
        fifoBuf_putData(ctx->apdu, rep_data, 72);
        return 72;
    }
    return 0;
}
#else
#define _fr_active_report_pro  fr_active_report_pro
#endif



//每秒调用
void report_2015state_judge(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb= (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    static uint8_t   rtc_cnt = 0;


    if( mbase_cb->voltage_battery < 300 ) //电池电压低
    {
        
        if(rtc_cnt > 2)
        {
            if (!(mbase_cb->report_2015state_cache & RPT_2015_RTC_BATTERY_LOW))
            {
                mbase_cb->report_2015state_cache |= RPT_2015_RTC_BATTERY_LOW;
                report_2015state_update(instance, RPT_2015_RTC_BATTERY_LOW);
            }
        }
        else
        {
            rtc_cnt++;
        }
    }
    else if( mbase_cb->voltage_battery > 305 ) //电池电压低
    {
        rtc_cnt = 0;
        mbase_cb->report_2015state_cache &=~RPT_2015_RTC_BATTERY_LOW;
    }

    //ESAM故障
    if ( mbase_cb->esam_chip_abnormal == EVENT_SET )
    {
        if (!(mbase_cb->report_2015state_cache & RPT_2015_ESAM_ERR))
        {
            mbase_cb->report_2015state_cache |= RPT_2015_ESAM_ERR;
            report_2015state_update(instance, RPT_2015_ESAM_ERR);
        }
    }
    else
    {
        mbase_cb->report_2015state_cache &=~RPT_2015_ESAM_ERR;
    }


    
    //存储器故障
    if ( DEVICE_ERR & BIT1 )
    {
        if (!(mbase_cb->report_2015state_cache & RPT_2015_MEMORIZER_ERR))
        {
            mbase_cb->report_2015state_cache |= RPT_2015_MEMORIZER_ERR;
            report_2015state_update(instance, RPT_2015_MEMORIZER_ERR);
        }
    }
    else
    {
        mbase_cb->report_2015state_cache &=~RPT_2015_MEMORIZER_ERR;
    }

      
#if 0  ////拉合闸成功
    if ( mbase_cb->switch_on_succees == 0x5A   ) 
    {
        mbase_cb->switch_on_succees = 0;
        report_2015state_update(instance, RPT_2015_SWITCH_ON_SUCCEES);
    }

    if ( mbase_cb->switch_off_succees == 0x5A   )
    {
        mbase_cb->switch_off_succees = 0;
        report_2015state_update(instance, RPT_2015_SWITCH_OFF_SUCCEES);
    }
#endif


}





//获取跟随上报使能开关
uint8_t follow_report_enable_get(struct instance_t* instance)
{
    uint32_t addr, ret;
    uint8_t buf[5];

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + FollowReport_en);
    ret = nd_get_index(instance, addr, buf, 1); 
    if (1 == buf[0])//是否打开跟随上报
    {
        return 1;
    }
    return 0;
}



//跟随上报相关
int fr_follow_report_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct fifo_t* cache = ctx->apdu;
    uint8_t* dst = cache->buf_ptr + cache->wr; 
    uint8_t* p = instance->apdu_cache; //临时缓冲区 
    uint8_t* p_tmp = instance->apdu_cache; //临时缓冲区 
    uint32_t oad = 0, mask = 0;
    uint16_t offset = 0, len = 0;
    uint8_t num = 0, flag = 0, rptmode = 0;


    if((ctx == NULL) || (mbase_cb == NULL))
    {
        return 0;
    }

    if (false == report_channel_check(instance, ctx->channel))
    {
        return 0;
    }

    if(0 == follow_report_enable_get(instance))
    {
        return 0;
    }

    
    oad = follow_report_3320List_Latest_oad_get(instance, ctx->channel);
    if (oad != 0)
    {
        num++;
        flag |= 0x01;
    }

    mask = mbase_cb->report_2015state[ctx->channel];
    rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);
    if(rptmode != 1 )
    {
        mask = 0;
    }

    if (mask != 0)
    {
        num++;
        flag |= 0x02;    
    }
    
    if (num == 0)
    {
        return 0;
    }

    *p++ = 1; 
    *p++ = 1; 
    *p++ = num;

    if (flag & 0x01)
    {
        *p++ = 0x33;
        *p++ = 0x20;
        *p++ = 0x02;
        *p++ = 0x01;
        *p++ = 1;
        *p++ = TYPE_OAD;
        p += uint32_to_buf_msb(oad, p);
    }

    if (flag & 0x02)
    {
        *p++ = 0x20;
        *p++ = 0x15;
        *p++ = 0x02;
        *p++ = 0x00;
        *p++ = 1;
        *p++ = TYPE_bit_string;
        *p++ = 0x20;
         p += uint32_to_buf_msb(mask, p);
    }

    len = (p - p_tmp);
    
    if (apdu_cache_getFree(cache) < len)
    {
        return 0;
    }

    ndv_memcopy(dst, p_tmp, len);

    cache->wr += len;

    return (uint16_t)len;

}
uint32_t get_3320_save_addr(struct instance_t* instance, uint16_t chn)
{
    uint32_t addr; 
    
    switch (chn)
    {
    case chl_ir_698: 
        addr = Report3320_OADList1;
        break;
    case chl_RS485_1_698: 
        addr = Report3320_OADList2;
        break;

    case chl_PLC_698: 
        addr = Report3320_OADList3;
        break;
    case chl_RS485_2_698: 
        addr = Report3320_OADList4;
        break;

    default:
        return 0;
        break;
    }

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + addr); 
    return addr;
}
// 上报通道使能检查
uint8_t report_channel_check(struct instance_t* instance, uint16_t channel)
{
    int ret;
    uint32_t addr, oad;
    uint8_t i, * p, buf[13] = {0};

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report_channel); 
    ret = nd_get_index(instance, addr, buf, 13);

    if (ret < 0)
    {
        return false;
    }

    if (buf[0] == 0 || buf[0] > 4)
    {
        return false;
    }

    p = buf + 1;

    for (i = 0; i < buf[0]; i++)
    {
        oad = buf_to_uint32_lsb(p);

        if (channel == chl_ir_698 && oad == 0xF2020201)
        {
            return true;
        }

        if (channel == chl_RS485_1_698 && oad == 0xF2010201)
        {
            return true;
        }

        if (channel == chl_PLC_698 && oad == 0xF2090201)
        {
            return true;
        }

        if (channel == chl_RS485_2_698 && oad == 0xF2010202)
        {
            return true;
        }

        p += 4;
    }

    return false;
}
//获取3320中最新的 跟随上报 oad
uint32_t follow_report_3320List_Latest_oad_get(struct instance_t* instance, uint8_t chn)
{
    uint8_t buf[241],num,i,*p, rptmode;
    uint32_t oad = 0;
    uint32_t addr;
    int ret;
    addr = get_3320_save_addr(instance, chn);
    ret = nd_get_index(instance, addr, buf, 241);
    if(ret < 0)
    {
        return 0;
    }
    
    num = buf[0];
    if(num > 60)
        num = 60;
    p = buf+1;
    for (i = 0; i < num; i++)
    {
        oad = buf_to_uint32_lsb(p);
        p += 4;
        rptmode = read_event_report_flag_mode(instance, oad, 1);
        if(rptmode == 1)//跟随上报
        {
            return oad;
        }
    }
    return 0;
}



//模块上报引脚 EventOut处理    
void EventOut_698_check(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t rptmode, count, buf[5];
    uint32_t mask;
    uint32_t addr, ret;
 
                  
    //eventout受43000700允许跟随上报开关影响             //后续做成可配
    if (1)
    {
        if (0 == follow_report_enable_get(instance))//是否打开跟随上报
        {
            PLC_EVENOUT_DIS();
            return;
        }
    }

    //是否是上报通道
    if (false == report_channel_check(instance, chl_PLC_698))
    {
        PLC_EVENOUT_DIS();
        return;
    }

    count = 0;

    if(0x00000000 != follow_report_3320List_Latest_oad_get(instance, chl_PLC_698))
    {
        count = 1;  
    }
    
    mask = mbase_cb->report_2015state[chl_PLC_698];

    rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);
    if(rptmode != 1 )
    {
        mask = 0;
    }

    if ((count == 0) && (mask == 0))
    {
        PLC_EVENOUT_DIS(); //关闭
    }
    else
    {
        PLC_EVENOUT_EN();   //打开 置高    
    }
}

//清除3320和2015事件清零时调用 
void report_3320list_2015state_clear(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i, buf[5];
    uint32_t addr, ret;
    
    for (i = 0; i< 4; i++)
    {
        mbase_cb->report_2015state[i] = 0;      

        buf[0] = 0;
        addr = get_3320_save_addr(instance, i);
        nd_set_index(instance, addr, buf, 1);
    }
    //跟随上报状态字 存e2p
    addr = ((uint32_t)(RES_DEV_EEPROM << 24) + ((uint32_t)mbase_block_persistent_eprom << 16) + \
    offsetof(struct mbase_control_block_t, report_2015state[0])) -  offsetof(struct mbase_control_block_t, persistent_section_start);
    nd_set_offset(instance, addr, (uint8_t*)&mbase_cb->report_2015state[0], 4*4);


    mbase_cb->report_2015state_cache = 0;


    EventOut_698_check(instance);
}


//上报应答
int fr_handle_report_response(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{  
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i,need_report_piid = false;
    uint8_t num,flag;
    uint32_t oad;
    uint8_t *p;
    uint8_t *buf = instance->apdu_cache;
    uint8_t *apdu_rx = (ctx->apdu_d_rx + ctx->apdu_rx_offset);

    

    if (1 == get_addr_type(ctx->SA))
    {
        return 0;
    }

    if(ctx->channel != chl_PLC_698)//RS485是1061，模块是1062
    {
         return 0;
    }
    
    p = apdu_rx ;
    num = *p++;
    ctx->apdu_rx_offset += 7;

    for(flag = 0; flag < num; flag++)
    {
        oad = buf_to_uint32_msb(p);
        p += 4;     
        ctx->apdu_rx_offset += 4;
        if((oad == 0x33200201) || (oad == 0x33200200))
        {            
            if(active_report_3320list_confirmed(instance))
            {
                mbase_cb->active_report_CMDtype &=~ ACTIVE_REPORT_3320;
                mbase_cb->active_report_retry_cnt[R_3320] = 0;
                need_report_piid = true;
            }               
        }
        else if(oad == 0x30110201)
        {           
            
            if(report_3320list_confirmed(instance, chl_PLC_698, oad&0xFFFFFF00, 0))
            {
                //mbase_cb->active_report_CMDtype &=~ ACTIVE_REPORT_3011_END; //挪report_3320list_confirmed里面
                //mbase_cb->active_report_retry_cnt[R_3011_END] = 0;
                need_report_piid = true;
            }                
        }
        else if(oad == 0x20150200)
        {            
            if(mbase_cb->report_2015state[chl_PLC_698] == 0)//确认2015时没有要确认的数据，则不做后续处理
            {                       
                return 0;
            }
            mbase_cb->report_2015state[chl_PLC_698] = 0;           
            mbase_cb->active_report_CMDtype &=~ ACTIVE_REPORT_2015;
            mbase_cb->active_report_retry_cnt[R_2015] = 0;
            need_report_piid = true;        
        }
    }
    if(need_report_piid)
        mbase_cb->report_piid++;

    return 0;
}





//主动上报相关

//清主动上报 重发次数 和 间隔时间
void clear_active_report_time_count(struct instance_t* instance, enum active_report_type_t type) 
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    
    mbase_cb->active_report_retry_dly[type] = 0;  
    mbase_cb->active_report_retry_cnt[type] = 0;

    if ((type == R_3011_START)||(type == R_3011_END))
    {
        mbase_cb->active_report_retry_dly[R_3011_START] = 0;  
        mbase_cb->active_report_retry_cnt[R_3011_START] = 0;
        mbase_cb->active_report_retry_dly[R_3011_END] = 0;  
        mbase_cb->active_report_retry_cnt[R_3011_END] = 0;
    }

}


//置位3320或2015的主动上报 
//setType: 0 开启2015主动上报   1   开启3320主动上报
void active_report_normal_CMD_set(struct instance_t* instance, uint8_t chn, uint8_t setType)
{   
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t buf[10];
    uint32_t addr, ret;  

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ActReport_Info);
    
    if(chn == chl_PLC_698) 
    {
        if(setType)//3320
        {
            if (instance->ac_power_statue == AC_POWER_OFF) 
            {
                ret = nd_get_index(instance, addr, buf, 4);
                if(ret > 0)//恢复主动上报状态字和主动上报次数
                {
                    mbase_cb->active_report_CMDtype = buf[0] | ((uint16_t)buf[1]<<8);
                    mbase_cb->active_report_retry_cnt[R_3320] = buf[2];
                    mbase_cb->active_report_retry_cnt[R_2015] = buf[3];
                }
            }    
            mbase_cb->active_report_CMDtype |= ACTIVE_REPORT_3320;
        }
        else//2015
        {
            mbase_cb->active_report_CMDtype |= ACTIVE_REPORT_2015;
        }
        
        //3320和2015 可合并上报，可分开确认
        clear_active_report_time_count(instance, R_3320);
        clear_active_report_time_count(instance, R_2015);

        if (instance->ac_power_statue == AC_POWER_OFF)
        {
            ndv_memcopy(buf, (uint8_t *)&mbase_cb->active_report_CMDtype, 2);
            ndv_memcopy(buf+2, (uint8_t *)&mbase_cb->active_report_retry_cnt[R_3320], 1);
            ndv_memcopy(buf+3, (uint8_t *)&mbase_cb->active_report_retry_cnt[R_2015], 1);
            ret = nd_set_index(instance, addr, buf, 4);
        }
    }
}


// 上报标识检查，传入上报标识和oad特征
int report_flag_schem_check(uint8_t rptflag, uint8_t Schem)
{

    switch (rptflag)
    {
    case START_RPT: //事件发生上报
        if ((Schem == 1) || (Schem == 2))
        {
            return 1;
        }
        break;

    case END_RPT: //事件恢复上报
        if ((Schem == 3) || (Schem == 4))
        {
            return 1;
        }
        break;

    case ALL_RPT: //事件发生恢复均上报
        if ((Schem >= 1) && (Schem <= 4))
        {
            return 1;
        }
        break;

    default:
        break;
    }
    return -1;
}




//更新 3320列表，   
//add_or_del: 0 删除  1新增,          0xFF: 传入属性6，自动删除属性6/7/8/9 [针对分项事件清零]
//chn通道
uint8_t report_3320list_update(struct instance_t* instance, uint8_t chn, uint8_t add_or_del, uint32_t oad)
{
    uint8_t  rptmode = 0, i, wflag = 0,  count, maxnum, *p, buf[300];
    uint16_t id, len, len2;
    uint32_t data;
    uint8_t cnt = 1, k = 0;
    uint32_t addr;
    int ret;

    addr = get_3320_save_addr(instance, chn);
    ret = nd_get_index(instance, addr, buf, 241);


    cnt = 1;
    if(add_or_del == 0xFF) //针对分项事件清零           删除
    {
        cnt = 4;
    }
   
    for(k = 0; k < cnt; k++)
    {
        p = buf;

        count = *p++;
        len = 1 + count * 4;

        maxnum = 60;  //固定60个
        if (count > maxnum)
        {
            return false;
        }

        oad += (0x00000100 * k);
        oad &= 0xffff1fff;

        for (i = 0; i < count; i++)
        {
            //delete the same oad
            data = buf_to_uint32_lsb(p);

            if (oad == data)
            {
                len2 = len - ((p - buf) + 4);
                ndv_memcopy(p, p + 4, len2);
                len -= 4;
                count--;
                buf[0] = count;
                wflag = 1;
                break;
            }

            p += 4;
        }
    }


    if (add_or_del == 1)
    {
        //add new oad
        if (count > 0)
        {
            ndv_memcopy(buf + 5, buf + 1, len - 1);
        }

        if (count < maxnum)
        {
            count++;
        }

        uint32_to_buf_lsb(oad, buf + 1);
        wflag = 1;

        rptmode = read_event_report_flag_mode(instance, oad, 1);

        if(rptmode == 0)//上报方式为主动上报
        {
            if(oad == 0x30110200)//
            {

            }
            else
            {
                active_report_normal_CMD_set(instance, chn, 1);
            }
        }
    }

    if (wflag != 0)
    {
        buf[0] = count;
        ret = nd_set_index(instance, addr, buf, 241);
        if (ret < 0)
        {
            return false;
        }
    }

    return true;
}


//3320上报列表新增         oad 带属性特征   
//事件记录处调用
void report_3320list_add(struct instance_t* instance,uint32_t oad)
{
    struct mbase_control_block_t* mbase_cb  = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i;
    uint8_t comm_chn[4] = { chl_ir_698, chl_RS485_1_698,  chl_PLC_698,  chl_RS485_2_698 };

    uint8_t Schem = 0,rptflag = 0;
    int ret;

    //获取事件上报标识
    rptflag = read_event_report_flag_mode(instance, oad, 0);

    Schem = get_OAD_Schem(oad);

    ret = report_flag_schem_check(rptflag, Schem);
    if (ret < 0)
    {
        return;
    }

    //本地表在公钥状态下, 电表清零事情不允许上报*/
    if (get_OAD_OI(oad) == 0x3013)
    {
        //测试秘钥状态0-私钥 1-公钥
        if ( mbase_cb->key_state == true ) 
        {
            return;
        }
    }

    for (i = 0; i< (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        report_3320list_update(instance, comm_chn[i], 1, oad);
    }

    EventOut_698_check(instance);

}

//上报列表中只确认主动上报oad 
uint8_t active_report_3320list_confirmed(struct instance_t* instance)//确认掉3320里面处于主动上报的OAD
{
    uint8_t count,i,*p,num, rptmode = 0;
    uint16_t len,len2;
    uint32_t oad = 0;
    uint32_t addr;
    uint8_t *tmpbuf = instance->apdu_cache;
    int ret;

    addr = get_3320_save_addr(instance, chl_PLC_698);
    ret = nd_get_index(instance, addr, tmpbuf, 241);

    p = tmpbuf;
    count = *p++;
    num = count;    
    len = count * 4;
    for (i = 0; i < num; i++)
    {
        oad = buf_to_uint32_lsb(p);
        rptmode = read_event_report_flag_mode(instance, oad, 1);
        if((rptmode == 0) && (oad != 0x30110200))//剔除主动上报的OAD
        {
            len2 = (num - 1-i) * 4;
            ndv_memcopy(p, p + 4, len2);
            count--;
        }
        else
        {
            p += 4;
        }
    }
    if(num == count)//没有OAD被确认，无需处理后续响应
    {        
        return false;
    }
    tmpbuf[0] = count;
    len = 1 + count * 4;
    
    ret = nd_set_index(instance, addr, tmpbuf, 241);
    if (ret < 0)
    {
        return false;
    }

    return true ;    
}


//上报列表确认   
//confirm_SUB_event:0,  只确认单个oad               0xFF:确认分项事件所有6/7/8/9属性
uint8_t report_3320list_confirmed(struct instance_t* instance, uint8_t chn, uint32_t oad, uint8_t confirm_SUB_event)
{
    struct mbase_control_block_t*  mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t add_or_del = 0;

    //新增事件列表
    if(confirm_SUB_event != 0xFF) // 非 ff 即0 
    {
        confirm_SUB_event = 0;
    }
    
    if (false == report_3320list_update(instance, chn, confirm_SUB_event, oad))
    {
        return false;
    }

    if (chn == chl_PLC_698)
    {
        if(0x3011 == get_OAD_OI(oad)) // 3320中确认3011，同步也要确认停电上报事件
        {
            mbase_cb->active_report_CMDtype &=~ ACTIVE_REPORT_3011_END;
            mbase_cb->active_report_retry_cnt[R_3011_END] = 0;
        }
    
        EventOut_698_check(instance);
    }

    return true;
}

uint8_t report_3320list_confirmed_All_channel(struct instance_t* instance, uint32_t oad)
{
    uint8_t i, j, oadClass;
    uint8_t comm_chn[4] = { chl_ir_698, chl_RS485_1_698,  chl_PLC_698,  chl_RS485_2_698 };


    for (i = 0; i< (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        oad = oad & 0xffff0000;
        oadClass = get_event_oad_class(oad);

        if (oadClass == 24)
        {
            oad += 0x00000600;  
            report_3320list_confirmed(instance, comm_chn[i], oad, 0xFF); 
        }
        else
        {
            oad += 0x00000200;
            report_3320list_confirmed(instance, comm_chn[i], oad, 0); 
        }
    }

    return true;
}



//2015上报状态字更新                调用地方  
uint8_t report_2015state_update(struct instance_t* instance, uint32_t mask)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t comm_chn[4] = { chl_ir_698, chl_RS485_1_698,  chl_PLC_698,  chl_RS485_2_698 };
    uint8_t i, rptmode;

    if(mbase_cb->report_2015mode & mask)
    {
        for (i = 0; i< (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
        {
            mbase_cb->report_2015state[i] |= mask;
            if(comm_chn[i] == chl_PLC_698)
            {
                rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);
                if(rptmode == 0)// 主动上报
                {
                    active_report_normal_CMD_set(instance, chl_PLC_698, 0);
                }
            }
        }
    }

    EventOut_698_check(instance);

    return true;
}

//主动上报 使能 检查
uint8_t active_report_enable_check(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int ret;
    uint32_t addr;
#if 1
    uint8_t flag = 0;

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ActiveReport_en);
    ret = nd_get_index(instance, addr, &flag, 1);


    if ((ret < 0) || (flag == 0))
    {
        return false;
    }

#endif
    return true;
}

//3011事件的主动上报方式和标识  检查    
//eventtype: 0掉电发生，1掉电恢复
//返回true 上报， false 不上报
uint8_t active_report_check_3011(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t *buf, uint8_t eventtype)
{
    uint8_t count, i, * p;
    uint32_t oad = 0;
    int ret;
    uint32_t addr;

    uint8_t rptflag = 0, rptmode = 0;

    if (eventtype == 1) //恢复
    {
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report3320_OADList3); // 读取模块通道的 上报列表
        ret = nd_get_index(instance, addr, buf, 241);

        if (ret < 0)
        {
            return false;
        }

        p = buf;
        count = *p++;
        if (count > 60)
            count = 60;

        for (i = 0; i < count; i++)
        {
            oad = buf_to_uint32_lsb(p);
            if (oad == 0x30110200)//
            {
            
                rptflag = read_event_report_flag_mode(instance, 0x30110200, 0);
                rptmode = read_event_report_flag_mode(instance, 0x30110200, 1);
                if (((rptflag == END_RPT) || (rptflag == ALL_RPT)))//掉电上报标识为事件恢复上报
                {
                    if (rptmode == ACTIVE_RPT)
                    {
                        return true;
                    }
                }
            }
            p += 4;
        }
    }
    else // 发生
    {
        rptflag = read_event_report_flag_mode(instance, 0x30110200, 0);
        rptmode = read_event_report_flag_mode(instance, 0x30110200, 1);
        if (((rptflag == START_RPT) || (rptflag == ALL_RPT)))//掉电上报标识为事件发生上报
        {
            if (rptmode == ACTIVE_RPT)
            {
                return true;
            }
        }

    }
    return true;
}

// 掉电事件 发生和恢复 主动上报组帧(apdu)
int active_report_3011_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t eventtype)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t data_buf[500]={0};
    uint8_t *ptr_cache, *ptr = data_buf;//instance->apdu_cache;    // 500字节
    uint8_t* tmp_buff = instance->apdu_cache + 500; //保证300字节空间
    uint32_t rcsd_list[(REQ_RCSD_MAXNUM + 1)];
    uint32_t oad;
    uint8_t i;
    uint16_t  data_len = 0;
    uint8_t* esam_apdu_p;
    
    struct rsd_info_t rsd_info;
    int ret;

    if (false == active_report_enable_check(instance, pipeline, mesg))
    {
        return 0;
    }
    
    if (false == report_channel_check(instance, chl_PLC_698)) 
    {
        return 0;
    }

    active_report_check_3011(instance, pipeline, mesg, tmp_buff, eventtype);

    if (eventtype)
    {
        ptr_cache = ptr + 3;
    }
    else
    {
        ptr_cache = ptr;
    }

    esam_apdu_p = ptr_cache;
    *ptr_cache++ = 0x88;
    *ptr_cache++ = 0x02;
    *ptr_cache++ = mbase_cb->report_piid & 0x3F;
    *ptr_cache++ = 0x01;

    oad = 0x30110201;
    ptr_cache += uint32_to_buf_msb(oad, ptr_cache); //oad

    oad = 0x30110200;
    get_capture_list(instance, oad, rcsd_list, 3);

    *ptr_cache++ = rcsd_list[0]; // rcsd个数
    if (rcsd_list[0] > REQ_RCSD_MAXNUM)
    {
        return -1;
    }

    for (i = 0; i < rcsd_list[0]; i++)
    {
        *ptr_cache++ = 0; // rcsd:oad
        ptr_cache += uint32_to_buf_msb(rcsd_list[1 + i], ptr_cache);
    }

    *ptr_cache++ = 1; // 数据


    memset((uint8_t *)&rsd_info, 0, sizeof(rsd_info));
    rsd_info.select = 9;
    rsd_info.last_no = 1;
    rsd_info.start_mark = 0;
    rsd_info.end_mark = 0xffffffffU;
    rsd_info.mode = 0;

    ret = record_data_package(instance, oad, tmp_buff, &rsd_info, rcsd_list,  300,  NULL);
    if (ret < 0)
    {
        return 0;
    }
    data_len = ret;
    ndv_memcopy(ptr_cache, tmp_buff, data_len);
    ptr_cache += data_len;

    *ptr_cache++ = 0; //跟随上报
    *ptr_cache++ = 0; //timeTag
    
    if (eventtype)
    {
        data_len = ptr_cache - ptr - 3;
        ptr[0] = 0x10;
        ptr[1] = 0x00;
        ptr[2] = data_len;   

        *ptr_cache++ = 0x02;//RN+MAC
        *ptr_cache++ = 0x0c;//RN长度

        if (mbase_cb->esam_chip_abnormal == EVENT_SET)
        {
            data_len = 12 + 5;
            memset(ptr_cache, 0xFF, data_len);
            *(ptr_cache + 12) = 0x04;
        }
        else
        {
            get_upreport_esam_rn_mac(instance, esam_apdu_p, data_len , ptr_cache);
            
            data_len = 12 + 5;
            ndv_move_buffer(ptr_cache+12, ptr_cache+13, 4);
            *(ptr_cache+12) = 0x04;       
        }
        ndv_memcopy(mbase_cb->report_random_data, ptr_cache, 12); 
        ptr_cache += data_len;
    }

    
    ndv_memcopy(instance->apdu_cache, data_buf, (ptr_cache - ptr));
    return (ptr_cache - ptr);

}

//获取模块通道 3320上报列表中主动上报 oad
//函数返回值为主动上报oad个数，    传入buf空间需保证300字节 
uint8_t active_report_3320list_get(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* buf)
{
    uint8_t count, i, * p, num, tmp_buf[241];
    uint16_t len, len2;
    uint32_t oad = 0;


    int ret;
    uint32_t addr;
    uint8_t rptmode = 0;

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + Report3320_OADList3); //读取模块通道的 上报列表
    ret = nd_get_index(instance, addr, tmp_buf, 241);

    if (ret < 0)
    {
        return false;
    }

    p = tmp_buf;
    count = *p++;
    if (count > 60)
        count = 60;
    num = count;
    len = 1 + count * 4ul;
    for (i = 0; i < num; i++)
    {
        oad = buf_to_uint32_lsb(p);
        rptmode = read_event_report_flag_mode(instance, oad, 1);
        if ((rptmode == FOLLOW_RPT) || (oad == 0x30110200))//非主动上报 不放在列表中,3011也不放主动上报OAD中
        {
            len2 = len - ((p - tmp_buf) + 4);
            ndv_memcopy(p, p + 4, len2);
            //len -= 4;
            count--;
        }
        else
            p += 4;
    }
    p = buf;
    for (i = 0; i < count; i++)
    {
        *p++ = TYPE_OAD;
        mem_swap_cpy(p, &tmp_buf[1 + i * 4], 4);
        p += 4;
    }
    return count;
}
//2015跟随上报状态字  检查是否需要上报，及获取状态字
//mode: 0 主动上报， 1跟随上报
uint32_t report_2015_get(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint16_t chn, enum report_mode_t mode)
{
    uint32_t value;

    uint8_t rptmode = 0;
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);

    if (rptmode != mode)//上报方式为跟随上报，才能跟随上报
    {
        return 0;
    }

    value = mbase_cb->report_2015state[chn%4];  //chl_PLC_698

    return value;
}
// 常规的3320和2015 主动上报组帧(apdu)
// get_data:0 只判断是否需要上报。  其他:获取一次上报内容
int active_report_normal_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t get_data)
{
    uint8_t* p, * apdu, num, flag, count, need_report_piid = false;
    uint32_t value;
    uint16_t encry_len = 0, len_apdu = 0;


    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t data_buf[500]={0};
    uint8_t *ptr = data_buf; //instance->apdu_cache;    // 500字节
    uint8_t* tmp_buff = instance->apdu_cache + 500; //保证300字节空间

    if (false == active_report_enable_check(instance, pipeline, mesg))
    {
        return 0;
    }

    if (false == report_channel_check(instance, chl_PLC_698))
    {
        return 0;
    }


    p = ptr;
    num = 0;
    flag = 0;

    if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3320)
    {
        if ((mbase_cb->active_report_retry_dly[R_3320] == 0) && (mbase_cb->active_report_retry_cnt[R_3320] < 3))//已发送，主站未确认，等待超时
        {
            count = active_report_3320list_get(instance, pipeline, mesg, &tmp_buff[0]);

            if (count)
            {
                flag |= 0x01;
                num++;
                if(get_data)
                {
                    mbase_cb->active_report_retry_cnt[R_3320]++;
                    if (3 == mbase_cb->active_report_retry_cnt[R_3320])
                        need_report_piid = true;
                }

            }
            else
            {
                if(get_data)
                {
                    mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3320;
                }
            }
        }
    }

    if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_2015)
    {
        if ((mbase_cb->active_report_retry_dly[R_3320] == 0) && (mbase_cb->active_report_retry_cnt[R_2015] < 3))//已发送，主站未确认，等待超时
        {
            value = report_2015_get(instance, pipeline, mesg, chl_PLC_698, ACTIVE_RPT);// 获取模块通道 需上报 2015
            
            if (value != 0)
            {
                flag |= 0x02;
                num++;
                if(get_data)
                {
                    mbase_cb->active_report_retry_cnt[R_2015]++;
                    if (3 == mbase_cb->active_report_retry_cnt[R_2015])
                        need_report_piid = true;
                }        
            }
            else
            {
                if(get_data)
                {
                    mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_2015;
                }
            }
        }
    }
    if (num == 0)
    {
        return 0;
    }
    *p++ = 0x10;
    *p++ = 0x00;
    *p++ = len_apdu;
    apdu = p;
    *p++ = 0x88;
    *p++ = 0x01;
    *p++ = mbase_cb->report_piid & 0x3F;
    *p++ = num;

    if (flag & 0x01)
    {
        *p++ = 0x33;
        *p++ = 0x20;
        *p++ = 0x02;
        *p++ = 0x00;
        *p++ = 0x01;
        *p++ = TYPE_array;
        *p++ = count;
        ndv_memcopy(p, &tmp_buff[0], 5ul * count);
        p += 5ul * count;
    }

    if (flag & 0x02)
    {
        *p++ = 0x20;
        *p++ = 0x15;
        *p++ = 0x02;
        *p++ = 0x00;
        *p++ = 1;
        *p++ = TYPE_bit_string;
        *p++ = 0x20;
        p += uint32_to_buf_msb(value, p);
    }
    *p++ = 0;
    *p++ = 0;
    len_apdu = p - apdu;
    *(apdu - 1) = len_apdu;
    *p++ = 0x02;//RN+MAC
    *p++ = 0x0c;//RN长度

    if (mbase_cb->esam_chip_abnormal == EVENT_SET)
    {
        encry_len = 12 + 5;
        memset(p, 0xFF, encry_len);
        *(p + 12) = 0x04;
    }
    else
    {
        get_upreport_esam_rn_mac(instance, apdu, len_apdu , p);
        encry_len = 12 + 5;
        ndv_move_buffer(p+12, p+13, 4);
        *(p+12) = 0x04;       
    }
    ndv_memcopy(mbase_cb->report_random_data, p, 12); //拷贝随机数
    p += encry_len;

    if (need_report_piid)
        mbase_cb->report_piid++;

    ndv_memcopy(instance->apdu_cache, data_buf, (len_apdu + encry_len + 5));
    
    return (len_apdu + encry_len + 5);
}

uint16_t report_flag = 0;
//掉电后 上报处理 每秒调用
int powerOff_report_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t get_data)
{
    struct mbase_control_block_t*  mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int len = 0;
    uint8_t start_time = 1;
    CLK_InitTypeDef CLK_Struct;
    uint32_t Periphral = 0;

    start_time = 1;
    if ((instance->ac_power_off_sec_cnt >= start_time) && (instance->ac_power_off_sec_cnt <= (start_time+2)))
    {
        if ((mbase_cb->power_on_sec_accum >= mbase_cb->active_report_sec_limit) && (mbase_cb->active_report_retry_cnt[R_3011_START] < 3)) // 上电超过1小时 进行掉电期间上报
        {
            len = active_report_3011_pro(instance, pipeline, mesg, 0);
            if(len > 0)
            {
                //打开vcc 时间？
                //配置io口 初始化模块uart 
                if(get_data)
                {
                    report_port_open(instance);
                    mbase_cb->active_report_retry_cnt[R_3011_START]++;
                    report_flag = 0xa55a;
                }
                //协议发送
            }
        }
    }
    return len;
    
}

// 主动上报处理  每秒调用   
// 主动上报apdu数据存instance->apdu_cache， 返回值len为上报apdu数据长度。 获取一次上报内容，相应消耗一次上报次数。
// get_data:0 只判断是否需要上报。  其他:获取一次上报内容
int fr_active_report_pro(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t get_data)
{
    struct mbase_control_block_t*  mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    int len;


    //停电
    if( instance->ac_power_statue != AC_POWER_ON ) 
    {

         return powerOff_report_pro(instance, pipeline, mesg, get_data);
    }
    else //上电
    {    
        if (mbase_cb->poweron_active_report_dly < 5)
        {
            return -1;
        }
        if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3011_END) 
        {
            if ((mbase_cb->active_report_retry_dly[R_3011_END] == 0) && (mbase_cb->active_report_retry_cnt[R_3011_END] < 3))//已发送，主站未确认，等待超时
            {
                len = active_report_3011_pro(instance, pipeline, mesg, 1);
                
                if (len > 0)
                {
                    if (get_data)
                    {
                        mbase_cb->active_report_retry_dly[R_3011_END] = 10;//  若无响应 10秒后重发       
                        mbase_cb->active_report_retry_cnt[R_3011_END]++;

                        if (3 == mbase_cb->active_report_retry_cnt[R_3011_END])
                            mbase_cb->report_piid++;
                    }
                    return len;

                }
                else
                {
                    if (get_data)
                    {
                        mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3011_END;
                    }
                }

            }
        }
        //不完全掉电时 掉电发生上报
        else if (mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3011_START)
        {
            len = active_report_3011_pro(instance, pipeline, mesg, 0);

            if (len > 0)
            {
                if (get_data)
                {
                    mbase_cb->active_report_retry_cnt[R_3011_START]++;
                    if (3 == mbase_cb->active_report_retry_cnt[R_3011_START])
                    {
                        mbase_cb->report_piid++;
                        mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3011_START;
                    }
                }
                return len;
            }
            else
            {
                if (get_data)
                {
                    mbase_cb->active_report_CMDtype &= ~ACTIVE_REPORT_3011_START;
                }
            }
        }
        
        if ((mbase_cb->active_report_CMDtype & ACTIVE_REPORT_3011_END) && (mbase_cb->active_report_retry_cnt[R_3011_END] < 3))
        {
            return 0;
        }
        if (mbase_cb->active_report_retry_dly[R_3011_END])
            return 0;
        if (mbase_cb->active_report_CMDtype & (ACTIVE_REPORT_3320 | ACTIVE_REPORT_2015))//有上报请求
        {
            len = active_report_normal_pro(instance, pipeline, mesg, get_data);
            
            if (len > 0)
            {
                if (get_data)
                {
                    mbase_cb->active_report_retry_dly[R_3320] = 10;//  10秒无响应重发     
                    mbase_cb->active_report_retry_dly[R_2015] = 10;//  10秒无响应重发   
                }
                return len;
            }
        }
        return 0;
    }
}

int active_report_handle(struct instance_t* instance, struct pipeline_t* p_pipeline, struct message_t* mesg)
{
    struct pipeline_t pipeline = {0};
    uint8_t buf[4];
    int len = _fr_active_report_pro(instance, p_pipeline, mesg, 0);//只判断是否有主动上报
    if(len > 0 ){
        buf[0] = 0x88;
        buf[1] = 0x01;
        buf[2] = len>>8;
        buf[3] = len;        
        mesg->len = 4;
        mesg->payload = buf;
        mesg->to = pid_plc_tx_698;
        mesg->from = pid_plc_tx_698;
        pipeline.id = pid_plc_rx_698;//pid_plc_rx_698;

        send_mesg(instance, &pipeline, mesg);
    }
    return 0;
}


int get_report_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return _fr_active_report_pro(instance, pipeline, mesg, 1);
}


