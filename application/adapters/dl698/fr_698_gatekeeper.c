#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "meter_base_view.h"
#include "fr_698_internal.h"

int fr_check_service_and_choice(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    uint16_t len = 0;
    if (((GET_Request == ctx->service) &&((ctx->choice ==2)|| (ctx->choice == 4)))||
        ((SET_Request == ctx->service) && ((ctx->choice == 2) || (ctx->choice == 3)))||
        ((ACTION_Request == ctx->service) && ((ctx->choice == 2) || (ctx->choice == 3))))
    {
        //mesg->oad_n = apdu_rx[ctx->apdu_rx_offset++]; //
        ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, &apdu_rx[ctx->apdu_rx_offset], &len);
        mesg->oad_n = len;
        fifoBuf_putByte(cache, mesg->oad_n);
    }else if (((GET_Request == ctx->service) &&((ctx->choice ==1) || (ctx->choice ==3)))||
             ((SET_Request == ctx->service) && (ctx->choice == 1))||
             ((ACTION_Request == ctx->service) && (ctx->choice == 1)))
    {
        mesg->oad_n = 1;
    }else if(((GET_Request == ctx->service) && (ctx->choice ==5))||
             (CONNECT_Request == ctx->service) || (RELEASE_Request == ctx->service) ||
             (REPORT_Response == ctx->service) || (SECURITY_Request == ctx->service)) {
        mesg->oad_n = 0;
    }else{
        return 1;
    }
    
    return 0;
}



//struct fifo_t* s_fifo;
int fr_gatekeeper_698_server(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t data[8]={0};
    struct fifo_t* cache = ctx->apdu;
    int err_no = 0, ret = 0;
    uint8_t service = 0;
    uint32_t tmp_u32 = 0;
    uint32_t apdu_offset_len_cache = 0;
    //uint8_t err_s_response_flag = 0;

    log_info(" OK.\n");
    //s_fifo = (struct fifo_t*)(struct fifo_t*)nd_fr698_pipeline_in_queue(pipeline);
    
    //协议一致性判断和apdu合法判断
    if(0 == fr_pack_698_check(instance, pipeline, mesg)){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    
    fifoBuf_removeData(fifo, APDU_OFFSET(ctx->SA));
    ctx->segment_rx = 0xffff;
    if(__fr_segment_tx_handle(instance, pipeline, mesg)){
        return -1;
    }

    fr_ctx_clear(instance, pipeline,  mesg);    
    if(is_segment_set(ctx->ctrl)){
        err_no = fr_segments_handle(instance, pipeline,  mesg);
        if (err_no <= 0) {
            //err_s_response_flag = 1;
            //ctx->service = ERROR_S_Response;
            //ctx->err_n = 1;
            log_info("fr_segments_handle: err_no=%02x.\n", err_no);
            return err_no;
        }
    }else{
        ctx->apdu_rx_len = __get_apdu_data_len(ctx);
        if(ctx->apdu_rx_len > Max698_apdu_rx){
            return -1;
        }
        int len_tail = fifoBuf_getUsed(fifo);
        fifoBuf_getData(fifo, apdu_rx,  len_tail);
        //fifoBuf_getData(fifo, apdu_rx,  ctx->apdu_rx_len);
        
        handle_scrambler(instance, pipeline, mesg);
    }
    
    update_plc_baut_tick(instance, pipeline, mesg);
    update_connect_timeout_cnt(instance, pipeline, mesg);

    
    ctx->service = apdu_rx[ctx->apdu_rx_offset++];
    ctx->security_choice = 0xff;
    ctx->verification_choice = 0xff;
    ctx->apdu_len_after_decode = ctx->apdu_rx_len;
    if ((SECURITY_Request == ctx->service) || (SECURITY_Response == ctx->service)){
        apdu_offset_len_cache = ctx->apdu_rx_offset;
        service = ctx->service;
        err_no = fr_security_request(instance, pipeline, mesg);
        if(err_no == 0){
            return -1;
        }

        ctx->service = apdu_rx[ctx->apdu_rx_offset++];

        if (SECURITY_CHOICE_0_Plaintext == ctx->security_choice)
        {
            if ( err_no > 0) //
            {
                err_no = ctx->security_data_len;//明文 apdu 长度  
            }
        }
        if ( err_no > 1)
        {
            apdu_offset_len_cache = ctx->apdu_rx_offset - apdu_offset_len_cache ;  //记录 安全传输 apdu 额外偏移的长度。
            ctx->apdu_len_after_decode = err_no + apdu_offset_len_cache;
        }
        /*
        if (SECURITY_CHOICE_1_Ciphertext == ctx->security_choice) //获取解密后apdu长度 用于帧格式解析判断  
        {
            if ( err_no > 1)
            {
                apdu_offset_len_cache = ctx->apdu_rx_offset - apdu_offset_len_cache ;  //记录 安全传输 apdu 额外偏移的长度。
                ctx->apdu_len_after_decode = err_no + apdu_offset_len_cache;
            }
        }
        */
    }
    

    if (!((CONNECT_Request == ctx->service) || (RELEASE_Request == ctx->service))){
        ctx->choice = apdu_rx[ctx->apdu_rx_offset++];
    }

    ctx->PIID = apdu_rx[ctx->apdu_rx_offset++];
    //ctx->PIID |= (1 << 6);//PIID-ACD


    ret = fr_timetag_check(instance, pipeline, mesg);
    
    if(0 == ret)
    {
        ctx->fr_err = FRM_TIME_TAG_ERR;
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    else if (ret < 0)
    {
        ctx->service = ERROR_S_Response;
        ctx->err_n = 1; // apdu 无法解析
    }

    mesg->from  = nd_fr698_pipeline_id(pipeline);//从哪里来
    mesg->to    = FR_698_PID_TX;//要到哪里去
    mesg->ctrl  = ctx->service |(1<<7);
    mesg->qos   = ctx->choice;
    mesg->id    = 0x00;//暂时没用

    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);

    if(fr_check_service_and_choice(instance, pipeline, mesg)){
        if (ctx->service != ERROR_S_Response)
        {
            ctx->service = ERROR_S_Response;
            ctx->err_n = 2; //服务不支持
        }
    }

    //if (err_s_response_flag != 0)
    //{
    //   ctx->service = ERROR_S_Response;
    //    ctx->err_n = err_s_response_flag;  
    //}

    
    //mac挂起判断
    if((nd_mb_mac_suspend_state(instance) == true) && ((service == SECURITY_Request) || (ctx->service == SET_Request) || (ctx->service == ACTION_Request)))
    {
        //ctx->service = SECURITY_Response;
        //service = SECURITY_Response;
        if (GET_Request != ctx->service) //挂起后 允许读
        {
            tmp_u32 = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
            app_handle_mac_suspend(instance, tmp_u32);
            err_no = -DAR_METER_SUSPEND;
        }

    }
    
    if((service == SECURITY_Request) && (err_no < 0)){
        ctx->service = SECURITY_Response;
        service = SECURITY_Response;
    }
    
    if(0 == fr_apdu_effective_check(instance, pipeline, mesg)){
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return -1;
    }
    log_info("security_request: service=%02x.\n", ctx->service);
    log_hex(ctx->apdu_d_rx, ctx->apdu_rx_len);
        
    if ((0 == DIR(ctx->ctrl)) && (0 == PRM(ctx->ctrl))) {
        if(REPORT_Response != ctx->service){
            fifoBuf_removeData(fifo, ctx->min_len + 2);
            return -1;
        }
    }
    
    switch (ctx->service)
    {
        case CONNECT_Request:
            err_no = fr_handle_connect(instance, pipeline, mesg);
            break;   
        case RELEASE_Request:
            err_no = fr_handle_disconnect(instance, pipeline, mesg);
            break;
        case GET_Request:  
            err_no = fr_handle_get_request(instance, pipeline, mesg);
            break;
        case SET_Request:
        case ACTION_Request:
            err_no = fr_handle_set_action_request(instance, pipeline, mesg);
            break;
        case REPORT_Response://8上报应答 client
            err_no = fr_handle_report_response(instance, pipeline, mesg);
            return -1;  //上报确认不响应
            break;
        case SECURITY_Response:
            cache->buf_ptr[0] = SECURITY_Response;
            cache->buf_ptr[1] = 2;
            cache->buf_ptr[2] = -err_no;
            cache->buf_ptr[3] = 0;
            cache->wr = 4;
            break;
        default:
            cache->buf_ptr[0] = ERROR_S_Response;
            cache->buf_ptr[2] = ctx->err_n;
            break;
    }
    
    log_info("security_encode:\n");
    log_hex(ctx->apdu->buf_ptr, fifoBuf_getUsed(ctx->apdu));
    
    err_no = fr_timeTag_optional(instance, pipeline, mesg);
    if(err_no < 0){
        return err_no;
    }
    
    if (service == SECURITY_Request){
        err_no = fr_security_encode(instance, pipeline, mesg);
        if(err_no < 0){
            cache->buf_ptr[0] = SECURITY_Response;
            cache->buf_ptr[1] = 2;
            cache->buf_ptr[2] = DAR_OTHERS_ERROR;
            cache->buf_ptr[3] = 0;
            cache->wr = 4;
        }else{
            uint8_t len = 2;
            data[0] = SECURITY_Response;
            data[1] = ctx->security_choice;
            len += axdr_len_encode(data+len, ctx->security_data_len);
            cache->rd = Max698_apdu - len;
            memcpy(cache->buf_ptr + cache->rd, data, len);
        }
    }

    if( (0 == get_addr_type(ctx->SA)) || 
        ((1 == get_addr_type(ctx->SA)) && (0 == ctx->resp_choice)) ){
        __fr_send_handle(instance, pipeline, mesg);
        return 0;
    }
    return 0;
}
