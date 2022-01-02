#include "nidavellir.h"
#include "fr_698.h"


int fr_security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t len)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_apdu = cache->buf_ptr;
    uint32_t addr;
    uint8_t Temp08,rxlen;
    uint16_t txlen=0,temp=0,offset=0;
    
    offset = fifo->rd;
    
    fifoBuf_getData(fifo, ctx->keybuf, len+1);   //明文/密文数据
    Temp08 = ctx->keybuf[len];
    if( (Temp08 == 1) || (Temp08 == 2) ) //RN(1)、RN_MAC(2)
    {
        
    }
    else    //SID(3)、SID_MAC(0)
    {
        fifoBuf_getData(fifo, &addr, 4);//SID(4)
        fifoBuf_getData(fifo, &temp, 1);//跳过tag
        fifoBuf_getData(fifo, &temp, 2);//附加信息长度（2）
    }

	//MAC
	if( (Temp08 != 1) && (Temp08 != 3) )  //SID_MAC、RN_MAC
	{
        fifoBuf_getData(fifo, &ctx->keybuf[len], 1);//跳过tag
		fifoBuf_getData(fifo, &ctx->keybuf[len], 4);
	}
    
    mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&temp, 2);
    
    rxlen = write_esam(addr , ctx->keybuf , txlen);
    
    fifo->wr = offset;
    fifoBuf_putData(fifo, ctx->keybuf, rxlen);
    
	return 1;
}


int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	return 1;
}