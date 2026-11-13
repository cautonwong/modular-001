#include <stdbool.h>
#include "application/ports/fifo.h"
#include "fr_698_pack.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "meter_base_view.h"
#include "fr_698_internal.h"

uint8_t fr_698_get_logic_name(uint8_t *buf, uint16_t OI)
{
    uint8_t *p=buf;

    *p++ = TYPE_octet_string;
    *p++ = 2;
    *p++ = (uint8_t)(OI >> 8);
    *p++ =  (uint8_t)OI;
    return (p-buf);
}

uint16_t get_crc16(uint16_t cs)
{
    cs ^= 0xffff;
    return cs;
}

int fr_rx_698_end(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    fr_pack_698_head(instance, pipeline, mesg);
    if(mesg->len)
        fr_pack_698_apdu(instance, pipeline, mesg);
    
    fr_pack_698_tail(instance, pipeline, mesg);
    return 0;
}


int fr_timetag_check(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint32_t interval_sec = 0;
    uint32_t time_sec = 0;
    uint32_t time_tag_sec = 0;
    int offset = 0;

    ctx->timeTag_valid = 0;	
    if ((ctx->service == LINK_Request) || (ctx->service == SECURITY_Request)){
        return -1;
    }

    switch (ctx->service)
    {
    case CONNECT_Request:
        offset = get_CONNECT_Request_len(instance, pipeline, mesg);
        break;
    case RELEASE_Request:
        offset = 0;
        break;
    case GET_Request:
        offset = get_GET_Request_len(instance, pipeline, mesg);
        break;
    case SET_Request:
    case ACTION_Request:
        offset = get_SET_and_ACTION_Request_len(instance, pipeline, mesg);
        break;
    default:
        return 1;
    }

    if (offset < 0) {
        return offset;
    }
    
    memcpy(ctx->timeTag, apdu_rx + ctx->apdu_rx_offset + offset, 11);
    ctx->timeTag_valid = ctx->timeTag[0];
    if (1 == ctx->timeTag_valid) {
        ctx->timeTag_valid = 2;
        if (false == dl_data_time_check(&ctx->timeTag[1], TIME_WILDCARD)){
          
        }else{
            interval_sec = dl_timeTag_interval(&ctx->timeTag[8], 0);
            time_tag_sec = dl_datetime_to_second(&ctx->timeTag[1], 0);
            time_sec     = rtc_mktime(nd_fr698_time(instance));
            
            //if((time_tag_sec >= time_sec) && 
               if(((time_tag_sec + interval_sec) > time_sec)){
                ctx->timeTag_valid = 1;
            }
        }
        offset += 11;
    }else{
        ctx->timeTag_valid = 0;
        offset += 1;
    }

    if((ctx->apdu_rx_offset + offset) != ctx->apdu_len_after_decode)  
    {
        return -1; //apdu 无法解析
    }
    
    if(ctx->apdu_len_valid == 0){
        ctx->apdu_len_valid = ctx->apdu_rx_offset + offset;
    }
    return 1;
}

int fr_addr_handle(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t i = 0;
    
    if (2 == get_addr_type(ctx->SA)) {
        return 0;
    }

    //地址判断
    if (0 != SA_LOGIC(ctx->SA)) {
        return 0;
    }
    
    if(!nd_mb_valid(instance)){
        return 0;
    }
    //uint8_t tsa[16] = {0};
    //mem_swap_cpy(tsa, nd_mb_tsa(instance), SA_LEN(ctx->SA) + 1);
    uint8_t *tsa = nd_mb_tsa(instance);
#if 1 
    if(0 == get_addr_type(ctx->SA)){
        if (0 != mem_compare(ctx->addr, tsa, SA_LEN(ctx->SA) + 1)){
            return 0;
        }
    }else if(1 == get_addr_type(ctx->SA)){
        for (i = 0; i < (SA_LEN(ctx->SA) + 1); i++)
        {
            if ((ctx->addr[i] & 0xf0) != 0xa0){
                if ((tsa[i] & 0xf0) != (ctx->addr[i]  & 0xf0)){
                    return 0;
                }
            }
            if ((ctx->addr[i] & 0x0f) != 0x0a){
                if ((tsa[i] & 0x0f) != (ctx->addr[i]  & 0x0f)){
                    return 0;
                }
            }
        }
        memcpy(ctx->addr, nd_mb_tsa(instance), SA_LEN(ctx->SA) + 1);
    }else if(3 == get_addr_type(ctx->SA)){
        if(!((1 == (SA_LEN(ctx->SA) + 1)) && (ctx->addr[0] == 0xaa))){
            return 0;
        }
    }else{
        return 0;
    }
#endif

    return 1;
}

int fr_pack_698_check(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);
    //struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)nd_fr698_pipeline_ctx(pipeline);
    
    if(!nd_mb_valid(instance))
    {
        return -1;
    }

    if (DIR(ctx->ctrl)) {   //合法性和一致性判断
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }
    //控制域判断
    if ((1 == DIR(ctx->ctrl)) && (1 == PRM(ctx->ctrl))) {
        return 0;
    }

    if (is_cmd(ctx->ctrl)) {
        return 0;
    }
    
    if(0 == fr_addr_handle(instance, pipeline, mesg)){
        return 0;
    }
    

    switch(nd_fr698_pipeline_id(pipeline))
    {
        case FR_698_PID_IR_RX: 
            ctx->channel = FR_698_CHL_IR;
            break;
        case FR_698_PID_RX: 
            ctx->channel = FR_698_CHL_RS485_1;
            break;
        case FR_698_PID_PLC_RX: 
            ctx->channel = FR_698_CHL_PLC;
            break;
        case FR_698_PID_4852_RX: 
            ctx->channel = FR_698_CHL_RS485_2;
            break;
        default:
            ctx->channel = FR_698_CHL_IR;
    }
    
    if((ctx->channel == FR_698_CHL_IR) || (ctx->channel == FR_698_CHL_RS485_1))
    {
        if(ctx->channel == FR_698_CHL_IR)  nd_mb_backlight_tmr(instance) = 2* (nd_mb_lcd_autodis_list_num(instance)) * (nd_mb_lcd_autodis_keep_sec(instance));
        nd_mb_commdisp_ir485(instance) = 2;
    }
    else if(ctx->channel == FR_698_CHL_PLC)
    {
        nd_mb_commdisp_plc(instance) = 2;
    }
    
    return 1;
}


int fr_apdu_effective_check(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);

    
    if((3 == get_addr_type(ctx->SA)) &&  (ctx->service != ACTION_Request)){
        return 0;
    }
    
    if((1 == get_addr_type(ctx->SA)) &&  ((ctx->service == SET_Request) || (ctx->service == ACTION_Request))){
        return 0;
    }
    if (1 == get_addr_type(ctx->SA)) {
        ctx->SA &= 0x3F;
    }
    
    if(ctx->apdu_len_valid != ctx->apdu_rx_len){
        //ctx->service = ERROR_S_Response;
        //ctx->err_n = 1;
    }
    return 1;
}

int fr_timeTag_optional(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    
    if(ctx->service == SECURITY_Response){
        return 1;
    }
    if(0 == fr_follow_report_optional(instance, pipeline, mesg)){
        p[cache->wr++] = 0x00;
    }
    
    if ((ctx->timeTag_valid >= 1) && (apdu_cache_getFree(cache) > 11)){    
        memcpy(p+cache->wr, ctx->timeTag, 11);
        cache->wr += 11;
    }else{
        p[cache->wr++] = 0x00;
    }
    
    return 1;
}

