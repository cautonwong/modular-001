#include "nidavellir.h"
#include "fr_698.h"

uint8_t Security_Mode;

int _fr_mw_sid_mac(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t len);

int _fr_security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint16_t len)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_apdu = cache->buf_ptr;
    uint32_t addr;
    uint16_t rxlen;
    uint16_t txlen=0,temp=0,offset=0;
    
    if(len == 0x6B0)
    {
        Security_Mode = (0x80 | ctx->apdu_rx->buf_ptr[len+5]);  //80/83
        
        memcpy((uint8_t*)&addr, (uint8_t*)&ctx->apdu_rx->buf_ptr[len+6], 4);//SID
        
        mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&ctx->apdu_rx->buf_ptr[len+6+5], 2);//LEN
        
        memcpy((uint8_t*)&ctx->apdu_rx->buf_ptr[len+5], (uint8_t*)&ctx->apdu_rx->buf_ptr[len+6+5+3], 4);//MAC
        
        rxlen = write_esam(addr , (uint8_t*)&ctx->apdu_rx->buf_ptr[5] , txlen);
        ctx->apdu_rx->buf_ptr[5 + rxlen] = 0x55;
        ctx->apdu_rx->buf_ptr[5 + rxlen+1] = 0x55;
        ctx->apdu_rx->buf_ptr[5 + rxlen+2] = 0x16;//补FF
        
        fifo->wr = fifo->rd;
        fifoBuf_putData(fifo, (uint8_t*)&ctx->apdu_rx->buf_ptr[5+3], rxlen);//+3的含义是让外层的fifo不用调整5字节
    }
    else
    {
        fifoBuf_getData(fifo, ctx->keybuf, len+1);   //明文/密文数据
        Security_Mode = (0x80 | ctx->keybuf[len]);  //80/83

        //SID(3)、SID_MAC(0)
        fifoBuf_getData(fifo, &addr, 4);//SID(4)
        fifoBuf_getData(fifo, &temp, 1);//跳过tag
        fifoBuf_getData(fifo, &temp, 2);//附加信息长度（2）

        //MAC
        if( (Security_Mode != 0x83) )  //SID_MAC、RN_MAC
        {
            fifoBuf_getData(fifo, &ctx->keybuf[len], 1);//跳过tag
            fifoBuf_getData(fifo, &ctx->keybuf[len], 4);
        }
        
        mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&temp, 2);
        
        rxlen = write_esam(addr , ctx->keybuf , txlen);
        
        fifo->wr = fifo->rd;
        fifoBuf_putData(fifo, ctx->keybuf, rxlen);
    }
    
	return 1;
}

int fr_security_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* fifo_apdu_rx = ctx->apdu_rx;
    uint16_t rd = 0, wr = 0;
    uint8_t data[4] = { 0 };
    uint16_t len = 0;

    if(ctx->segment_rx)
    {
        fifo = fifo_apdu_rx;
    }
    ctx->choice = fifoBuf_getByte(fifo);    
    fifoBuf_peekByteBuf(fifo, 0, data, 4);
    uint8_t ret = axdr_decode(TYPE_octet_string, data, &len);
    for(uint8_t i =0 ; i < ret ; i++)
    {
        fifoBuf_getByte(fifo);
    }
    
    if (SECURITY_CHOICE_1_Ciphertext == ctx->choice)//密文
    {
        _fr_security_decode(instance, pipeline, mesg, len);
    }
    else if (SECURITY_CHOICE_0_Plaintext == ctx->choice)//明文
    {
        rd = fifo->rd;
        wr = fifo->wr;
        fifo->rd = fifo->rd + len;
        if (fifoBuf_getByte(fifo) == 0)
        {
            fifo->rd = rd;
            fifo->wr = wr;
            if (_fr_mw_sid_mac(instance, pipeline, mesg, len) == 0)//MAC效验错误
            {
                return -1;
            }
        }
        fifo->rd = rd;
        fifo->wr = wr;
    }
    else
    {
        return -1;
    }
    return 1;
}

int _fr_mw_sid_mac(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t len)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_apdu = cache->buf_ptr;
    uint32_t addr;
    uint8_t rxlen;
    uint16_t txlen=0,temp=0,offset=0;
    
    fifoBuf_getData(fifo, ctx->keybuf, len+1);   //明文/密文数据

    //SID(3)、SID_MAC(0)
    fifoBuf_getData(fifo, &addr, 4);//SID(4)
    fifoBuf_getData(fifo, &temp, 1);//跳过tag
    fifoBuf_getData(fifo, &temp, 2);//附加信息长度（2）

	//MAC
    fifoBuf_getData(fifo, &ctx->keybuf[len], 1);//跳过tag
    fifoBuf_getData(fifo, &ctx->keybuf[len], 4);
    
    mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&temp, 2);
    
    rxlen = write_esam(addr , ctx->keybuf , txlen);
    
//    fifo->wr = fifo->rd;
//    fifoBuf_putData(fifo, ctx->keybuf, rxlen);
    
	return rxlen;
    
}

int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_apdu = cache->buf_ptr;
    uint32_t addr;
    uint8_t rxlen;
    uint16_t txlen=0,temp=0,offset=0;
    
    if((Security_Mode & 0x80) == 0x00)  //明文+RN
    {
        Security_Mode = fifoBuf_getByte(fifo);
    }
    
    if(Security_Mode == 0x01)
    {
        txlen = fifoBuf_getByte(fifo);//RN
        fifoBuf_getData(fifo, ctx->keybuf, txlen);
        
        memcpy((uint8_t*)&ctx->keybuf[txlen], p_apdu, cache->wr);//实际数据
        *p_apdu++ = SECURITY_Response;//回应字节
        *p_apdu++ = 0x00;//明文
        *p_apdu++ = cache->wr;//长度
        memcpy(p_apdu, (uint8_t*)&ctx->keybuf[txlen], cache->wr);
        txlen = txlen + cache->wr;//长度
        addr = 0x02400E80;
        rxlen = write_esam(addr , ctx->keybuf , txlen);
        p_apdu = p_apdu + cache->wr;
        *p_apdu++ = 0x01;
        *p_apdu++ = 0x00;
        *p_apdu++ = 0x04;
        memcpy(p_apdu, ctx->keybuf, 4);//MAC
        p_apdu = p_apdu + 4;
        cache->wr = p_apdu - cache->buf_ptr;
    }
    else
    {
        txlen = cache->wr;
        memcpy(ctx->keybuf, p_apdu, txlen);
        if((Security_Mode == 0x00) || (Security_Mode == 0x80) || (Security_Mode == 0x83))//00:明文SID+MAC 80:SID+MAC  83:SID
        {
            switch(Security_Mode)
            {
                case 0x00:
                  addr = 0x11001C81;
                  break;
                case 0x80:
                  addr = 0xA7001C81;
                  break;
                case 0x83:
                  addr = 0xA6001C81;
                  break;
            }
            rxlen = write_esam(addr , ctx->keybuf , txlen);
            *p_apdu++ = SECURITY_Response;
            if(Security_Mode == 0)
                *p_apdu++ = 0x00;
            else
                *p_apdu++ = 0x01;
            *p_apdu++ = rxlen - 4;
            memcpy(p_apdu, ctx->keybuf, rxlen-4);
            p_apdu = p_apdu +  rxlen - 4;
            *p_apdu++ = 0x01;
            *p_apdu++ = 0x00;
            *p_apdu++ = 0x04;
            memcpy(p_apdu, (uint8_t*)&ctx->keybuf[rxlen-4], 4);
            p_apdu = p_apdu + 4;
            cache->wr = p_apdu - cache->buf_ptr;
        }
    }
    
	return 1;
}