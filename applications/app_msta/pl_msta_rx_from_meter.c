#include "nidavellir.h"

extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_91(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_86(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_06(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_93_from_meter(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_645_decode_91_from_meter(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_rx_645_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

int fr_msta_rx_from_meter_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    int r = 0;

    //todo ºÏ·¨ÐÔÅÐ¶Ï
    //todo ÅÐ¶ÏID ¿´¿´ÊÇ²»ÊÇÒª×ª·¢proxy
    fifoBuf_removeData(fifo, 10);//½«Êý¾ÝÓòÇ°µÄ×Ö½ÚÇåµô
	
		//先处理转发的数据
		if(ctx_msta_rx_from_bt.mesg.qos == 0xA3)  //蓝牙的转发处理
		{
				mesg->to = msta_tx_to_bt;
				mesg->ctrl = ctx->ctrl;
				mesg->len = 0x00;
				uint8_t len = ctx->min_len - 12;//收到的数据域的长度
				uint8_t* p = instance->cache;//数据域的起始
				fifoBuf_getData(fifo, p + mesg->len, len);
				mesg->id = buf_to_uint32_lsb(p + mesg->len);
				mesg->len += len;
				send_mesg(instance, pipeline, mesg, p);
				memset(&ctx_msta_rx_from_bt.mesg,0x00,sizeof(struct message_t));//清除转发标志		
				return 0;
		}
		if(ctx_msta_proxy_to_bt.mesg.qos == 0xA3)  //蓝牙的转发处理
		{
				mesg->to = msta_tx_to_bt;
				mesg->ctrl = ctx->ctrl;
				mesg->len = 0x00;
				uint8_t len = ctx->min_len - 12;//收到的数据域的长度
				uint8_t* p = instance->cache;//数据域的起始
				fifoBuf_getData(fifo, p + mesg->len, len);
				mesg->id = buf_to_uint32_lsb(p + mesg->len);
				mesg->len += len;
				send_mesg(instance, pipeline, mesg, p+4);
				memset(&ctx_msta_proxy_to_bt.mesg,0x00,sizeof(struct message_t));//清除转发标志		
				return 0;
		}		
		
		if(ctx_msta_rx_from_sta.mesg.qos == 0xA3)  //STA的转发处理
		{
				mesg->to = msta_tx_to_sta;
				mesg->ctrl = ctx->ctrl;
				mesg->len = 0x00;
				uint8_t len = ctx->min_len - 12;//收到的数据域的长度
				uint8_t* p = instance->cache;//数据域的起始
				fifoBuf_getData(fifo, p + mesg->len, len);
				mesg->id = buf_to_uint32_lsb(p + mesg->len);
				mesg->len += len;
				send_mesg(instance, pipeline, mesg, p);
				memset(&ctx_msta_rx_from_sta.mesg,0x00,sizeof(struct message_t));//清除转发标志	
				return 0;
		}
		
    switch (ctx->ctrl)
    {
    case 0x91:
        r = fr_645_decode_91_from_meter(instance, pipeline, mesg);
        break;
    case 0x93:
        r = fr_645_decode_93_from_meter(instance, pipeline, mesg);
        break;
    case 0x86:
        r = fr_decode_645_86(instance, pipeline, mesg);
        break;
    default:
        //todo  ²»ÈÏÊ¶µÄ±¨ÎÄ  ×ª·¢¸ø»ù±í
        break;
    }

    if (r == 1)
    {
        fr_rx_645_end(instance, pipeline, mesg);
    }

    return 0;
}

//msta½ÓÊÕµ½À´×ÔmeterµÄ±¨ÎÄ´¦ÀíÁ÷Ë®Ïß
const struct filter_t fr_msta_rx_from_meter[] =
{
    {0x00,fr_rx_645_entry,1,13},
    {0x01,fr_msta_rx_from_meter_gatekeeper,0x00,13},
};

struct pipeline_t pl_msta_rx_from_meter =
{
    1,
    "msta_rx_from_meter",
    &ctx_msta_rx_from_meter,
    &pipe_msta_rx_from_meter,
    &pipe_msta_tx_to_meter,
    sizeof(fr_msta_rx_from_meter) / sizeof(fr_msta_rx_from_meter[0]),
    fr_msta_rx_from_meter,
};
