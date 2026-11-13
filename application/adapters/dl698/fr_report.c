#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "parameter_port.h"
#include "report_list.h"
#include "meter_base_view.h"
#include "application/ports/platform.h"
#include "domain/common/status.h"
#include "domain/report/report_policy.h"
#include "domain/report/report_retry.h"
#include "fr_645_report.h"
#include "fr_report_frames.h"

extern uint8_t DEVICE_ERR;
extern void report_3320list_add(struct instance_t* instance, uint32_t oad);
extern uint8_t active_report_3320list_confirmed(struct instance_t* instance);
extern uint8_t report_3320list_confirmed(struct instance_t* instance, uint8_t cnl, uint32_t oad, uint8_t confirm_SUB_event);
extern uint8_t report_3320list_confirmed_All_channel(struct instance_t* instance, uint32_t oad);
extern int fr_handle_report_response(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
extern void EventOut_698_check(struct instance_t* instance);
extern uint8_t report_channel_check(struct instance_t* instance, uint16_t channel);
extern uint32_t follow_report_3320List_Latest_oad_get(struct instance_t* instance, uint8_t chn);

extern void report_port_open(struct instance_t* instance);


extern uint8_t read_event_report_flag_mode(struct instance_t* instance, uint32_t oad, uint8_t mode);



//每秒调用
void report_2015state_judge(struct instance_t* instance)
{

    static uint8_t   rtc_cnt = 0;


    if( nd_mb_voltage_battery(instance) < 300 ) //电池电压低
    {
        
        if(rtc_cnt > 2)
        {
            if (!(nd_mb_report_2015_cache(instance) & RPT_2015_RTC_BATTERY_LOW))
            {
                nd_mb_report_2015_cache(instance) |= RPT_2015_RTC_BATTERY_LOW;
                report_645state_judge(instance, RPT_2015_RTC_BATTERY_LOW);
                report_2015state_update(instance, RPT_2015_RTC_BATTERY_LOW);
            }
        }
        else
        {
            rtc_cnt++;
        }
    }
    else if( nd_mb_voltage_battery(instance) > 305 ) //电池电压低
    {
        rtc_cnt = 0;
        nd_mb_report_2015_cache(instance) &=~RPT_2015_RTC_BATTERY_LOW;
    }

    //ESAM故障
    if ( nd_mb_esam_chip_abnormal(instance) == ND_EVENT_SET )
    {
        if (!(nd_mb_report_2015_cache(instance) & RPT_2015_ESAM_ERR))
        {
            nd_mb_report_2015_cache(instance) |= RPT_2015_ESAM_ERR;
            report_645state_judge(instance, RPT_2015_ESAM_ERR);
            report_2015state_update(instance, RPT_2015_ESAM_ERR);
        }
    }
    else
    {
        nd_mb_report_2015_cache(instance) &=~RPT_2015_ESAM_ERR;
    }


    
    //存储器故障
    if ( DEVICE_ERR & (1u << 1) )
    {
        if (!(nd_mb_report_2015_cache(instance) & RPT_2015_MEMORIZER_ERR))
        {
            nd_mb_report_2015_cache(instance) |= RPT_2015_MEMORIZER_ERR;
            report_645state_judge(instance, RPT_2015_MEMORIZER_ERR);
            report_2015state_update(instance, RPT_2015_MEMORIZER_ERR);
        }
    }
    else
    {
        nd_mb_report_2015_cache(instance) &=~RPT_2015_MEMORIZER_ERR;
    }

      
#if 0  ////拉合闸成功
    if ( nd_mb_switch_on_success(instance) == 0x5A   )
    {
        nd_mb_switch_on_success(instance) = 0;
        report_645state_judge(instance, RPT_2015_SWITCH_ON_SUCCEES);
        report_2015state_update(instance, RPT_2015_SWITCH_ON_SUCCEES);
    }

    if ( nd_mb_switch_off_success(instance) == 0x5A   )
    {
        nd_mb_switch_off_success(instance) = 0;
        report_645state_judge(instance, RPT_2015_SWITCH_OFF_SUCCEES);
        report_2015state_update(instance, RPT_2015_SWITCH_OFF_SUCCEES);
    }
#endif


}





//获取跟随上报使能开关
uint8_t follow_report_enable_get(struct instance_t*)
{
    int ret;
    uint8_t buf[5] = {0};

    ret = dl698_parameter_read(DL698_PARAM_REPORT_FOLLOW_EN, buf, 1);
    if ((ret == 0) && (1 == buf[0]))//是否打开跟随上报
    {
        return 1;
    }
    return 0;
}



//跟随上报相关
int fr_follow_report_optional(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
    uint8_t* dst = cache->buf_ptr + cache->wr; 
    uint8_t* p = nd_fr698_apdu_cache(instance); //临时缓冲区 
    uint8_t* p_tmp = nd_fr698_apdu_cache(instance); //临时缓冲区 
    uint32_t oad = 0, mask = 0;
uint16_t len = 0;
    uint8_t num = 0, flag = 0, rptmode = 0;


    if((ctx == NULL) || !nd_mb_valid(instance))
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

    mask = nd_mb_report_2015_state(instance)[ctx->channel];
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
static dl698_parameter_t report_3320_key(uint16_t channel)
{
    switch (channel)
    {
    case FR_698_CHL_IR:
        return DL698_PARAM_REPORT_3320_LIST_IR;
    case FR_698_CHL_RS485_1:
        return DL698_PARAM_REPORT_3320_LIST_RS485_1;
    case FR_698_CHL_PLC:
        return DL698_PARAM_REPORT_3320_LIST_PLC;
    case FR_698_CHL_RS485_2:
        return DL698_PARAM_REPORT_3320_LIST_RS485_2;
    default:
        return DL698_PARAM_COUNT;
    }
}

int dl698_report_list_read(uint16_t channel, uint8_t* buffer, uint16_t length)
{
    dl698_parameter_t key = report_3320_key(channel);

    if (key == DL698_PARAM_COUNT)
    {
        return -1;
    }
    return dl698_parameter_read(key, buffer, length);
}

int dl698_report_list_write(uint16_t channel, const uint8_t* buffer, uint16_t length)
{
    dl698_parameter_t key = report_3320_key(channel);

    if (key == DL698_PARAM_COUNT)
    {
        return -1;
    }
    return dl698_parameter_write(key, buffer, length);
}

// 上报通道使能检查
uint8_t report_channel_check(struct instance_t*, uint16_t channel)
{
    uint8_t buf[13] = {0};

    if (dl698_parameter_read(DL698_PARAM_REPORT_CHANNEL, buf, sizeof(buf)) < 0)
    {
        return false;
    }

    return nd_report_channel_enabled(channel, buf, sizeof(buf));
}
//获取3320中最新的 跟随上报 oad
uint32_t follow_report_3320List_Latest_oad_get(struct instance_t* instance, uint8_t chn)
{
    nd_report_list_t list;
    uint8_t buf[241], i, rptmode;

    if (dl698_report_list_read((uint16_t)chn, buf, sizeof(buf)) != 0)
    {
        return 0;
    }
    if (nd_report_list_parse(&list, buf, sizeof(buf)) != 0)
    {
        return 0;
    }

    for (i = 0; i < list.count; i++)
    {
        rptmode = read_event_report_flag_mode(instance, list.oad[i], 1);
        if (rptmode == ND_REPORT_MODE_FOLLOW)
        {
            return list.oad[i];
        }
    }
    return 0;
}



//模块上报引脚 EventOut处理    
void EventOut_698_check(struct instance_t* instance)
{
uint8_t rptmode, count;
    uint32_t mask;
 
                  
    //eventout受43000700允许跟随上报开关影响             //后续做成可配
    if (1)
    {
        if (0 == follow_report_enable_get(instance))//是否打开跟随上报
        {
            nd_platform_event_output_set(false);
            return;
        }
    }

    //是否是上报通道
    if (false == report_channel_check(instance, FR_698_CHL_PLC))
    {
        nd_platform_event_output_set(false);
        return;
    }

    count = 0;

    if(0x00000000 != follow_report_3320List_Latest_oad_get(instance, FR_698_CHL_PLC))
    {
        count = 1;  
    }
    
    mask = nd_mb_report_2015_state(instance)[FR_698_CHL_PLC];

    rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);
    if(rptmode != 1 )
    {
        mask = 0;
    }

    if ((count == 0) && (mask == 0))
    {
        nd_platform_event_output_set(false); //关闭
    }
    else
    {
        nd_platform_event_output_set(true);   //打开 置高    
    }
}

//清除3320和2015事件清零时调用 
void report_3320list_2015state_clear(struct instance_t* instance)
{
    uint8_t i, buf[5];
    
    for (i = 0; i< 4; i++)
    {
        nd_mb_report_2015_state(instance)[i] = 0;

        buf[0] = 0;
        dl698_report_list_write((uint16_t)i, buf, 1);
    }
    //跟随上报状态字 存e2p
    nd_mb_save_report_2015_state(instance);


    nd_mb_report_2015_cache(instance) = 0;


    EventOut_698_check(instance);
}


//上报应答
int fr_handle_report_response(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{  
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
uint8_t need_report_piid = false;
    uint8_t num,flag;
    uint32_t oad, State2015_reset = 0xFFFFFFFF;
    uint8_t *p;
    uint8_t *apdu_rx = (ctx->apdu_d_rx + ctx->apdu_rx_offset);

    

    if (1 == get_addr_type(ctx->SA))
    {
        return 0;
    }

    if(ctx->channel != FR_698_CHL_PLC)//RS485是1061，模块是1062
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
                nd_mb_active_report_cmdtype(instance) &=~ ACTIVE_REPORT_3320;
                nd_mb_active_report_retry_cnt(instance)[R_3320] = 0;
                need_report_piid = true;
            }               
        }
        else if(oad == 0x30110201)
        {           
            
            if(report_3320list_confirmed(instance, FR_698_CHL_PLC, oad&0xFFFFFF00, 0))
            {
                //nd_mb_active_report_cmdtype(instance) &=~ ACTIVE_REPORT_3011_END; //挪report_3320list_confirmed里面
                //nd_mb_active_report_retry_cnt(instance)[R_3011_END] = 0;
                need_report_piid = true;
            }                
        }
        else if(oad == 0x20150200)
        {            
            if(nd_mb_report_2015_state(instance)[FR_698_CHL_PLC] == 0)//确认2015时没有要确认的数据，则不做后续处理
            {                       
                return 0;
            }
            nd_mb_report_2015_state(instance)[FR_698_CHL_PLC] = 0;
            nd_mb_active_report_cmdtype(instance) &=~ ACTIVE_REPORT_2015;
            nd_mb_active_report_retry_cnt(instance)[R_2015] = 0;
            need_report_piid = true;   

            State2015_reset = 0xFFFFFFFF;
            report_stat_reset_from_698(instance,  State2015_reset);
        }
    }
    if(need_report_piid)
        nd_mb_report_piid(instance)++;

    EventOut_698_check(instance);    
    return 0;
}





//主动上报相关

//清主动上报 重发次数 和 间隔时间
void clear_active_report_time_count(struct instance_t* instance, enum active_report_type_t type) 
{
    
    nd_mb_active_report_retry_dly(instance)[type] = 0;
    nd_mb_active_report_retry_cnt(instance)[type] = 0;

    if ((type == R_3011_START)||(type == R_3011_END))
    {
        nd_mb_active_report_retry_dly(instance)[R_3011_START] = 0;
        nd_mb_active_report_retry_cnt(instance)[R_3011_START] = 0;
        nd_mb_active_report_retry_dly(instance)[R_3011_END] = 0;
        nd_mb_active_report_retry_cnt(instance)[R_3011_END] = 0;
    }

}


//置位3320或2015的主动上报 
//setType: 0 开启2015主动上报   1   开启3320主动上报
void active_report_normal_CMD_set(struct instance_t* instance, uint8_t chn, uint8_t setType)
{   


    
    if(chn == FR_698_CHL_PLC) 
    {
        if(setType)//3320
        {  
            nd_mb_active_report_cmdtype(instance) |= ACTIVE_REPORT_3320;
        }
        else//2015
        {
            nd_mb_active_report_cmdtype(instance) |= ACTIVE_REPORT_2015;
        }
        
        //3320和2015 可合并上报，可分开确认
        clear_active_report_time_count(instance, R_3320);
        clear_active_report_time_count(instance, R_2015);

        if (*nd_fr698_power_state(instance) == ND_AC_POWER_OFF)
        {
            nd_mb_save_active_report_state(instance);
        }
    }
}







//更新 3320列表，   
//add_or_del: 0 删除  1新增,          0xFF: 传入属性6，自动删除属性6/7/8/9 [针对分项事件清零]
//chn通道
uint8_t report_3320list_update(struct instance_t* instance, uint8_t chn, uint8_t add_or_del, uint32_t oad)
{
    nd_report_list_t list;
    uint8_t buf[241];
    uint8_t variants = 1, k, wflag = 0;
    uint32_t masked_oad = oad & 0xffff1fff;
    uint8_t rptmode;

    if (dl698_report_list_read((uint16_t)chn, buf, sizeof(buf)) != 0)
    {
        return false;
    }
    if (nd_report_list_parse(&list, buf, sizeof(buf)) != 0)
    {
        return false;
    }

    if (add_or_del == 0xFF) //针对分项事件清零 删除属性6/7/8/9
    {
        variants = 4;
    }

    for (k = 0; k < variants; k++)
    {
        if (nd_report_list_remove(&list, nd_report_3320_variant(masked_oad, k)))
        {
            wflag = 1;
        }
    }

    if (add_or_del == 1)
    {
        nd_report_list_insert_front(&list, masked_oad);
        wflag = 1;

        rptmode = read_event_report_flag_mode(instance, masked_oad, 1);
        if ((rptmode == 0) && (masked_oad != 0x30110200))
        {
            active_report_normal_CMD_set(instance, chn, 1);
        }
    }

    if (!wflag)
    {
        return true;
    }
    if (nd_report_list_encode(&list, buf, sizeof(buf)) == 0)
    {
        return false;
    }
    return dl698_report_list_write((uint16_t)chn, buf, sizeof(buf)) == 0;
}


//3320上报列表新增         oad 带属性特征   
//事件记录处调用
void report_3320list_add(struct instance_t* instance,uint32_t oad)
{
    uint8_t i;
    uint8_t comm_chn[4] = { FR_698_CHL_IR, FR_698_CHL_RS485_1,  FR_698_CHL_PLC,  FR_698_CHL_RS485_2 };

    uint8_t Schem = 0, rptflag = 0;

    //获取事件上报标识
    rptflag = read_event_report_flag_mode(instance, oad, 0);

    Schem = get_OAD_Schem(oad);

    if (!nd_report_flag_matches(rptflag, Schem))
    {
        return;
    }

    //本地表在公钥状态下, 电表清零事情不允许上报*/
    if (get_OAD_OI(oad) == 0x3013)
    {
        //测试秘钥状态0-私钥 1-公钥
        if ( nd_mb_key_state(instance) == true )
        {
            return;
        }
    }


    //停电事件、主动上报 且不足1小时  不上报    也不应加入列表     //低功耗下主动上报 不加入列表


    for (i = 0; i< (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        report_3320list_update(instance, comm_chn[i], 1, oad);
    }

    EventOut_698_check(instance);

}

//上报列表中只确认主动上报oad 
uint8_t active_report_3320list_confirmed(struct instance_t* instance)//确认掉3320里面处于主动上报的OAD
{
    nd_report_list_t list;
    uint8_t *tmpbuf = nd_fr698_apdu_cache(instance);
    uint8_t i = 0, removed = 0;
    uint8_t rptmode;

    if (dl698_report_list_read(FR_698_CHL_PLC, tmpbuf, 241) != 0)
    {
        return false;
    }
    if (nd_report_list_parse(&list, tmpbuf, 241) != 0)
    {
        return false;
    }

    while (i < list.count)
    {
        rptmode = read_event_report_flag_mode(instance, list.oad[i], 1);
        if ((rptmode == 0) && (list.oad[i] != 0x30110200))
        {
            report_stat_reset_from_3320_698(instance, list.oad[i]);
            nd_report_list_remove_at(&list, i);
            removed = 1;
        }
        else
        {
            i++;
        }
    }

    if (!removed)
    {
        return false;
    }
    if (nd_report_list_encode(&list, tmpbuf, 241) == 0)
    {
        return false;
    }
    return dl698_report_list_write(FR_698_CHL_PLC, tmpbuf, 241) == 0;
}


//上报列表确认   
//confirm_SUB_event:0,  只确认单个oad               0xFF:确认分项事件所有6/7/8/9属性
uint8_t report_3320list_confirmed(struct instance_t* instance, uint8_t chn, uint32_t oad, uint8_t confirm_SUB_event)
{

    //新增事件列表
    if(confirm_SUB_event != 0xFF) // 非 ff 即0 
    {
        confirm_SUB_event = 0;
    }
    
    if (false == report_3320list_update(instance, chn, confirm_SUB_event, oad))
    {
        return false;
    }

    report_stat_reset_from_3320_698(instance, oad);

    if (chn == FR_698_CHL_PLC)
    {
        if(0x3011 == get_OAD_OI(oad)) // 3320中确认3011，同步也要确认停电上报事件
        {
            nd_mb_active_report_cmdtype(instance) &=~ ACTIVE_REPORT_3011_END;
            nd_mb_active_report_retry_cnt(instance)[R_3011_END] = 0;
        }
     
    }
    EventOut_698_check(instance);

    return true;
}

uint8_t report_3320list_confirmed_All_channel(struct instance_t* instance, uint32_t oad)
{
uint8_t i, oadClass;
    uint8_t comm_chn[4] = { FR_698_CHL_IR, FR_698_CHL_RS485_1,  FR_698_CHL_PLC,  FR_698_CHL_RS485_2 };


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
    uint8_t comm_chn[4] = { FR_698_CHL_IR, FR_698_CHL_RS485_1,  FR_698_CHL_PLC,  FR_698_CHL_RS485_2 };
    uint8_t i, rptmode;

    if (nd_report_2015_enabled(nd_mb_report_2015_mode(instance), mask))
    {
        for (i = 0; i< (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
        {
            nd_mb_report_2015_state(instance)[i] |= mask;
            if(comm_chn[i] == FR_698_CHL_PLC)
            {
                rptmode = read_event_report_flag_mode(instance, 0x20150200, 1);
                if(rptmode == 0)// 主动上报
                {
                    active_report_normal_CMD_set(instance, FR_698_CHL_PLC, 0);
                }
            }
        }
    }

    EventOut_698_check(instance);

    return true;
}

//主动上报 使能 检查
uint8_t active_report_enable_check(struct instance_t*, struct pipeline_t*, nd_dl698_msg_t*)
{
    int ret;
#if 1
    uint8_t flag = 0;

    ret = dl698_parameter_read(DL698_PARAM_REPORT_ACTIVE_EN, &flag, 1);


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
uint8_t active_report_check_3011(struct instance_t* instance, struct pipeline_t*, nd_dl698_msg_t*, uint8_t *buf, uint8_t eventtype)
{
    uint8_t rptflag, rptmode;
    int has_start_record = 1;

    if (eventtype == 1) //恢复
    {
        nd_report_list_t list;

        if (dl698_parameter_read(DL698_PARAM_REPORT_3320_LIST_PLC, buf, 241) != 0)
        {
            return false;
        }
        if (nd_report_list_parse(&list, buf, 241) != 0)
        {
            return false;
        }
        has_start_record = nd_report_list_find(&list, 0x30110200) >= 0;
        if (!has_start_record)
        {
            return false;
        }
    }

    rptflag = read_event_report_flag_mode(instance, 0x30110200, 0);
    rptmode = read_event_report_flag_mode(instance, 0x30110200, 1);

    return nd_report_3011_active_allowed(eventtype, rptflag, rptmode,
                                         has_start_record);
}

// 掉电事件 发生和恢复 主动上报组帧(apdu)
uint16_t report_flag = 0;

static void report_retry_load(struct instance_t* instance,
                              nd_report_retry_t* retry)
{
    uint8_t i;

    retry->cmdtype = nd_mb_active_report_cmdtype(instance);
    retry->piid = nd_mb_report_piid(instance);
    for (i = 0; i < ND_REPORT_RETRY_COUNT; i++)
    {
        retry->retry_dly[i] = nd_mb_active_report_retry_dly(instance)[i];
        retry->retry_cnt[i] = nd_mb_active_report_retry_cnt(instance)[i];
    }
}

static void report_retry_store(struct instance_t* instance,
                               const nd_report_retry_t* retry)
{
    uint8_t i;

    nd_mb_active_report_cmdtype(instance) = retry->cmdtype;
    nd_mb_report_piid(instance) = retry->piid;
    for (i = 0; i < ND_REPORT_RETRY_COUNT; i++)
    {
        nd_mb_active_report_retry_dly(instance)[i] = retry->retry_dly[i];
        nd_mb_active_report_retry_cnt(instance)[i] = retry->retry_cnt[i];
    }
}

int fr_active_report_pro(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg, uint8_t get_data)
{
    nd_report_retry_t retry;
    nd_report_retry_input_t input = { 0 };
    nd_report_action_t action;
    int len = 0;

    report_retry_load(instance, &retry);

    input.power_on = (*nd_fr698_power_state(instance) == ND_AC_POWER_ON);
    input.reset_flag = *nd_fr698_reset_flag(instance);
    input.ac_power_off_sec = *nd_fr698_ac_power_off_sec_cnt(instance);
    input.power_on_sec = nd_mb_power_on_sec_accum(instance);
    input.power_on_sec_limit = nd_mb_active_report_sec_limit(instance);
    input.power_on_report_delay = nd_mb_poweron_active_report_dly(instance);

    action = nd_report_retry_next(&retry, &input);

    if (action == ND_REPORT_ACTION_WAIT)
    {
        return -1;
    }

    if (action == ND_REPORT_ACTION_3011_START)
    {
        len = active_report_3011_pro(instance, pipeline, mesg, 0);
        if (input.power_on)
        {
            nd_report_retry_3011_sent(&retry, 0, get_data, len > 0);
        }
        else
        {
            nd_report_retry_3011_start_off_sent(&retry, get_data, len > 0);
            if ((len > 0) && get_data)
            {
                report_port_open(instance);
                report_flag = 0xa55a;
            }
        }
    }
    else if (action == ND_REPORT_ACTION_3011_END)
    {
        len = active_report_3011_pro(instance, pipeline, mesg, 1);
        nd_report_retry_3011_sent(&retry, 1, get_data, len > 0);
    }

    if ((len <= 0) && input.power_on && (action != ND_REPORT_ACTION_NORMAL))
    {
        nd_report_retry_t probe = retry;

        probe.cmdtype &= (uint16_t)~ND_REPORT_CMD_3011_START;
        action = nd_report_retry_next(&probe, &input);
        if (action == ND_REPORT_ACTION_NORMAL)
        {
            len = active_report_normal_pro(instance, pipeline, mesg, get_data,
                                           &retry);
            nd_report_retry_normal_sent(&retry, get_data, len > 0);
        }
        else
        {
            len = 0;
        }
    }
    else if (action == ND_REPORT_ACTION_NORMAL)
    {
        len = active_report_normal_pro(instance, pipeline, mesg, get_data,
                                       &retry);
        nd_report_retry_normal_sent(&retry, get_data, len > 0);
    }

    report_retry_store(instance, &retry);
    return len;
}

int active_report_handle(struct instance_t* instance)
{
    nd_dl698_msg_t mesg = { 0 };
    struct context_698_t *ctx;
    uint8_t buf[4];
    int len = fr_active_report_pro(instance, NULL, &mesg, 0);//只判断是否有主动上报

    if (len > 0) {
        buf[0] = 0x88;
        buf[1] = 0x01;
        buf[2] = len >> 8;
        buf[3] = len;
        mesg.len = 4;
        mesg.payload = buf;
        mesg.to = FR_698_PID_PLC_TX;
        mesg.from = FR_698_PID_PLC_TX;

        ctx = (struct context_698_t*)get_pipeline_ctx(instance, FR_698_PID_PLC_RX);
        if ((ctx != NULL) && (ctx->send != NULL)) {
            ctx->send(instance, NULL, &mesg);
        }
    }
    return 0;
}


int get_report_apdu(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    return fr_active_report_pro(instance, pipeline, mesg, 1);
}


