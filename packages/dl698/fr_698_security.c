#include "nidavellir.h"
#include "fr_698.h"


int fr_security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t len)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_apdu = cache->buf_ptr;
    uint32_t addr;
    uint8_t buf[100],Temp08,rxlen;
    uint16_t txlen=0,temp=0;
    
    fifoBuf_getData(fifo, buf, len+1);   //明文/密文数据
    Temp08 = buf[len];
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
        fifoBuf_getData(fifo, &buf[len], 1);//跳过tag
		fifoBuf_getData(fifo, &buf[len], 4);
	}
    
    mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&temp, 2);
    
    rxlen = write_esam(addr , buf , txlen);
    
    fifoBuf_putData(fifo, buf, rxlen);
    fifo->wr = fifo->wr - rxlen;
  
	return 1;
}


int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	return 1;
}