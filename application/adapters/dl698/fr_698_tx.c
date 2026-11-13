#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "meter_base_view.h"
#include "fr_698_internal.h"

int __fr_send_handle(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t data[2]={0};
    uint16_t apdu_send_max = 512;
    uint16_t apdu_len = fifoBuf_getUsed(ctx->apdu);
    uint8_t channel = 0, cilent_index = 0;
    struct connect_request_t connect;

    if((ctx)&&(ctx->channel < FR_698_CHL_NUM))
    {
        channel = ctx->channel;
    }

    if (CA_TYPE_TER(ctx->CA)) 
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }


    if(0 != nd_mb_connect_timeout(instance)[channel][cilent_index])
    {
        ndv_memcopy(&connect, &cfg_connect, sizeof(cfg_connect));
    }
    else
    {
        connect = get_default_connect_para(channel);
    }

    apdu_send_max = connect.send_frm_size;

    
    if (apdu_len > 0){
        if (apdu_len > apdu_send_max){
            set_segment(ctx->ctrl);
            if(0 == ctx->segment_tx){
                ctx->segment_tx_no = 0;
                ctx->segment_tx = 1;
                set_first_segment_no(ctx->segment_tx_no);
            }else{
                set_middle_segment_no(ctx->segment_tx_no);
            }
            mesg->len = apdu_send_max;
        } else {
            if(ctx->segment_tx){
                set_segment(ctx->ctrl);
                set_last_segment_no(ctx->segment_tx_no);
            }
            mesg->len = apdu_len;
        }

        if(ctx->segment_tx){
            data[0] = ctx->segment_tx_no;
            data[1] = ctx->segment_tx_no>>8;
            fifoBuf_update_rd(ctx->apdu, 2, 0);          
            mesg->len+=2;
            memcpy(ctx->apdu->buf_ptr + ctx->apdu->rd, data, 2);
        }
        fr_rx_698_end(instance, pipeline, mesg);
        if (apdu_len <= apdu_send_max){
            ringbuffer_clear(ctx->apdu);
            ctx->segment_tx_no = 0;
            ctx->segment_tx = 0;
        }
    }
    return 0;
}
int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    int len = 0;
    //client  或者 主动上报 或者 延时读取发送
    if ((mesg->len >= 4) && (mesg->payload != NULL)){

        uint8_t *payload = mesg->payload;

        ctx->ctrl = 0x83;        
        ctx->CA = 0x00;
        ctx->SA = 0x05;
        ctx->addr[0] = 0x08;
        mem_swap_cpy(ctx->addr, nd_mb_tsa(instance), SA_LEN(ctx->SA) + 1);
        mesg->from = FR_698_PID_PLC_TX;
        mesg->len = (payload[2]<<8) + payload[3];
        
        if(mesg->len){
            if((len = get_report_apdu(instance, pipeline, mesg)) > 0)
            {   
                fr_pack_698_head(instance, pipeline, mesg);
                ringbuffer_clear(ctx->apdu);
                fifoBuf_putData(ctx->apdu, nd_fr698_apdu_cache(instance), mesg->len);
                
                fr_pack_698_apdu(instance, pipeline, mesg);
                /*
                if(0 == fr_follow_report_optional(instance, pipeline, mesg)){
                    fifoBuf_putByte(cache, 0);
                }
                fifoBuf_putByte(cache, 0);
                */
            }
            else
            {
                return 0;
            }
        }
        fr_pack_698_tail(instance, pipeline, mesg);
    }

    return 0;
}
