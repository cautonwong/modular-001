#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_07(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_11(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_rx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_msta_decode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int __update_cs(void* pctx, uint8_t* byte);
extern int __update_scrambler_and_cs(void* pctx, uint8_t* byte);
extern uint8_t bcd_to_hex(uint8_t bcd);
int fr_decode_645_11_from_bt(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = pipeline->in_queue;
    int r = 0;
    struct cache_t* cache = instance->cache2;
	cache->used = 0;
//    uint8_t buf[256];
    ctx->cs = 0;
    append_byte_to_cache(cache, 0x68, __update_cs, (void *)ctx);
    append_data_to_cache(cache, ctx->addr, 6, __update_cs, ctx);
    append_byte_to_cache(cache, 0x68, __update_cs, ctx);
    uint8_t len = ctx->min_len - 12;//收到的数据域的长度
    uint8_t* p = cache->buf +10;
    fifoBuf_getData(fifo, p, 4);
    uint32_t id = buf_to_uint32_lsb(p);
		mesg->id = id;
		mesg->ctrl = ctx->ctrl;
    mesg->len = 4;
//    uint8_t byte;

		switch(id)
		{
			case 0x0E800001:
			{
					uint8_t meter_no[8];
					uint64_t m = 0;
				
					r = __read(instance, 0x04000401, meter_no, 0x06);
					if (r < 0)
					{
							r = 1;
							mesg->ctrl = 0xb1;
							p[mesg->len++] = -r;//todo 错误码映射
					}
					else
					{						
							memset(p + mesg->len, 0x00, 25);	      //地址的特殊转换
							m += (uint64_t)bcd_to_hex(meter_no[5]) * 10000000000;							
							m += (uint64_t)bcd_to_hex(meter_no[4]) * 100000000;
							m += bcd_to_hex(meter_no[3]) * 1000000;
							m += bcd_to_hex(meter_no[2]) * 10000;
							m += bcd_to_hex(meter_no[1]) * 100;
							m += bcd_to_hex(meter_no[0]);
							p[mesg->len + 5] = 0xc0;
							p[mesg->len + 4] = (m >> 32) & 0xff;
							p[mesg->len + 3] = (m >> 24) & 0xff;
							p[mesg->len + 2] = (m >> 16) & 0xff;
							p[mesg->len + 1] = (m >> 8) & 0xff;
							p[mesg->len] = m & 0xff;					  
							mesg->len += 25;
					}
					break;
			}							
			case 0x07FB0303:
			{
					fifoBuf_getData(fifo, p + mesg->len, 5);
					//SA = p[mesg->len];
					uint16_t n = (p[mesg->len + 1] | (p[mesg->len + 2] << 8));
					uint16_t m = (p[mesg->len + 3] | (p[mesg->len + 4] << 8));
					//todo 判断其合法性
					r = (m + 7) / 8 - n / 8;//要读取的总包数
					mesg->len += 5;
					memcpy(&p[mesg->len], instance->upgrade_info.bits + n / 8, r);
					mesg->len += r;		
					break;	
			}
			default:
					mesg->to = msta_tx_to_meter;
					mesg->qos = 0xA3;     //转发的标志；
					send_mesg(instance, pipeline, mesg, p);
					memcpy(&ctx->mesg,mesg,sizeof(struct message_t));//记录转发的mesg	
					return 0x00;
		}
    append_byte_to_cache(cache, ctx->ctrl, __update_cs, ctx);
    append_byte_to_cache(cache, mesg->len, __update_cs, ctx);
    append_data_to_cache(cache, p, mesg->len, __update_scrambler_and_cs, ctx);
    append_byte_to_cache(cache, ctx->cs, __update_cs, ctx);
    append_byte_to_cache(cache, 0x16, __update_cs, ctx);
    return 1;
}

int fr_msta_rx_from_bt_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    pipeline->ctx = &ctx_msta_rx_from_bt;
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct cache_t* cache = instance->cache2;
    int r = 0;
		uint16_t bl_fc = 0x00;

    //todo 第一层合法性判断 识别通道
    if (ctx->addr[2] == 0xff)//内部通道
    {
        fifoBuf_removeData(fifo, 10);//将内层的数据域前的字节清掉
        switch (ctx->ctrl)
        {
        case 0x11:
            r = fr_decode_645_11(instance, pipeline, mesg);
            break;
				case 0x13:
					  r = fr_msta_decode_645_13(instance, pipeline, mesg);
        default:   //todo  不认识的报文  转发给基表	
						mesg->to = msta_tx_to_meter;
						mesg->ctrl = ctx->ctrl;
						mesg->len = 0x00;
						uint8_t len = ctx->min_len - 12;//收到的数据域的长度
						uint8_t* p = instance->cache;//数据域的起始
						fifoBuf_getData(fifo, p + mesg->len, len);
				    uint32_t id = buf_to_uint32_lsb(p + mesg->len);
						mesg->len += len;
						mesg->id = id;
						mesg->qos = 0xA3;     //转发的标志；
						send_mesg(instance, pipeline, mesg, p);
						memcpy(&ctx->mesg,mesg,sizeof(struct message_t));//记录转发的mesg										
						break;							
        }
        if (r == 1)
        {
            fr_rx_645_end(instance, pipeline, mesg);
        }
        return 0;
    }
    else
    {
        fifoBuf_removeData(pipeline->in_queue, 10);//剥掉外层的头
        ctx_msta_proxy_to_bt.step = 0;
        ctx_msta_proxy_to_bt.cs = 0;
        ctx_msta_proxy_to_bt.min_len = 12;
        pipeline->ctx = &ctx_msta_proxy_to_bt;
        ctx = (struct context_645_t*)pipeline->ctx;
        ctx->min_len = 0;
        if (fr_rx_645_entry(instance, pipeline, mesg) == 0)
        {
            //内层报文格式不对
            return 0;
        }

        //todo 合法性判断
        //todo 判断ID 看看是不是要转发proxy
        fifoBuf_removeData(fifo, 10);//将内层的数据域前的字节清掉

        switch (ctx->ctrl)
        {
        case 0x11:
            r = fr_decode_645_11_from_bt(instance, pipeline, mesg);
            break;
        default:
						mesg->to = msta_tx_to_meter;
						mesg->ctrl = ctx->ctrl;
						mesg->len = 0x00;
						uint8_t len = ctx->min_len - 12;//收到的数据域的长度
						uint8_t* p = instance->cache;//数据域的起始
						fifoBuf_getData(fifo, p + mesg->len, len);
				    uint32_t id = buf_to_uint32_lsb(p + mesg->len);
						mesg->len += len;
						mesg->id = id;
						mesg->qos = 0xA3;     //转发的标志；
						send_mesg(instance, pipeline, mesg, p);
						memcpy(&ctx->mesg,mesg,sizeof(struct message_t));//记录转发的mesg						
            return 0;
        }
				
				if(r > 0)
				{
						pipeline->ctx = &ctx_msta_rx_from_bt;
						ctx = pipeline->ctx;
						fifo = (struct fifo_t*)pipeline->out_queue;
						fr_pack_645_head(instance, pipeline, mesg);
						append_byte(fifo, 0x11, __update_cs, ctx);
						append_byte(fifo, cache->used, __update_cs, ctx);
						append_data(fifo, instance->cache2->buf,
								cache->used, __update_scrambler_and_cs, ctx);
						fr_pack_645_tail(instance, pipeline, mesg);
						uart_ctrl_enable(pipeline->id);				
				}
    }
    return 0;
}

struct filter_t fr_msta_rx_from_bt[] =
{
    {0x00,fr_rx_645_entry,0x01,13},
    {0x01,fr_msta_rx_from_bt_gatekeeper,0x00,13},
};

//msta接收到来自meter的报文处理流水线
struct pipeline_t pl_msta_rx_from_bt = 
{
    1,
    "msta_rx_from_bt",
    &ctx_msta_rx_from_bt,
    &pipe_msta_rx_from_bt,
    &pipe_msta_tx_to_bt,
    sizeof(fr_msta_rx_from_bt)/sizeof(fr_msta_rx_from_bt[0]),
    fr_msta_rx_from_bt,
};
