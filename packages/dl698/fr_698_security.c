#include "nidavellir.h"
#include "fr_698.h"

uint8_t Security_Mode;


static int __verification_choice_SID_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __verification_choice_RN(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __verification_choice_RN_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __verification_choice_SID(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
  

#define choice_FUNC                                        \
            DEF_X(verification_choice_SID_MAC,  __verification_choice_SID_MAC)  \
            DEF_X(verification_choice_RN,       __verification_choice_RN)       \
            DEF_X(verification_choice_RN_MAC,   __verification_choice_RN_MAC)   \
            DEF_X(verification_choice_SID,      __verification_choice_SID)      \
              

enum verification_choice_e
{
    #define DEF_X(a,b)  a,
    choice_FUNC
    #undef DEF_X
    verification_choice_MAX
};

typedef int(*pFunc_verification_choice)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
     
const pFunc_verification_choice Func_verification[] = 
{
    #define DEF_X(a,b)  b,
    choice_FUNC
    #undef DEF_X
};
             

static int __verification_choice_SID_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t len = 0;
    uint8_t *ver_data = ctx->apdu_d_rx;
    //ctx->security_data     数据指针
    //ctx->security_data_len 数据长度
    //组织esam数据
    return 1;
}

static int __verification_choice_RN(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t len = 0;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t *ver_data;
    
    ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, apdu_rx+ctx->apdu_rx_offset, &len);
    
    //ctx->security_data     数据指针
    //ctx->security_data_len 数据长度    
    //len 随机数长度
    //随机数指针
    ver_data = apdu_rx +  ctx->apdu_rx_offset;

    //组织esam数据
    
    return 1;
}

static int __verification_choice_RN_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t len = 0;
    uint8_t *ver_data = ctx->apdu_d_rx;
    //ctx->security_data     数据指针
    //ctx->security_data_len 数据长度
    //组织esam数据
    return 1;
}

static int __verification_choice_SID(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t len = 0;
    uint8_t *ver_data = ctx->apdu_d_rx;
    //ctx->security_data     数据指针
    //ctx->security_data_len 数据长度
    //组织esam数据
}




//待删除函数saga
int _security_verification(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
#if 0
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_apdu = cache->buf_ptr;
    uint32_t addr;
    uint8_t rxlen;
    uint16_t txlen=0,temp=0,offset=0;
        
    fifoBuf_getData(fifo, ctx->keybuf, len+1);   //����/��������

    //SID(3)��SID_MAC(0)
    fifoBuf_getData(fifo, &addr, 4);//SID(4)
    fifoBuf_getData(fifo, &temp, 1);//����tag
    fifoBuf_getData(fifo, &temp, 2);//������Ϣ���ȣ�2��

	//MAC
    fifoBuf_getData(fifo, &ctx->keybuf[len], 1);//����tag
    fifoBuf_getData(fifo, &ctx->keybuf[len], 4);
    
    mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&temp, 2);
    
    rxlen = write_esam(addr , ctx->keybuf , txlen);
    
//    fifo->wr = fifo->rd;
//    fifoBuf_putData(fifo, ctx->keybuf, rxlen);
    
	return rxlen;
#endif
}



int _security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint16_t len = 0;
    uint8_t *ver_data = ctx->apdu_d_rx;
    //ctx->security_data     数据指针
    //ctx->security_data_len 数据长度
    //组织esam数据
    
    
#if 0
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
        ctx->apdu_rx->buf_ptr[5 + rxlen+2] = 0x16;//��FF
        
        fifo->wr = fifo->rd;
        fifoBuf_putData(fifo, (uint8_t*)&ctx->apdu_rx->buf_ptr[5+3], rxlen);//+3�ĺ�����������fifo���õ���5�ֽ�
    }
    else
    {
        fifoBuf_getData(fifo, ctx->keybuf, len+1);   //����/��������
        Security_Mode = (0x80 | ctx->keybuf[len]);  //80/83

        //SID(3)��SID_MAC(0)
        fifoBuf_getData(fifo, &addr, 4);//SID(4)
        fifoBuf_getData(fifo, &temp, 1);//����tag
        fifoBuf_getData(fifo, &temp, 2);//������Ϣ���ȣ�2��

        //MAC
        if( (Security_Mode != 0x83) )  //SID_MAC��RN_MAC
        {
            fifoBuf_getData(fifo, &ctx->keybuf[len], 1);//����tag
            fifoBuf_getData(fifo, &ctx->keybuf[len], 4);
        }
        
        mem_swap_cpy((uint8_t*)&txlen, (uint8_t*)&temp, 2);
        
        rxlen = write_esam(addr , ctx->keybuf , txlen);
        
        fifo->wr = fifo->rd;
        fifoBuf_putData(fifo, ctx->keybuf, rxlen);
    }
    
	return 1;
#endif
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////
int fr_security_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t len  = 0;
    
    ctx->security_choice = apdu_rx[ctx->apdu_rx_offset++];
    ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, apdu_rx+ctx->apdu_rx_offset, &len);
    ctx->security_data = apdu_rx + ctx->apdu_rx_offset;
    ctx->security_data_len = len;
    ctx->apdu_rx_offset += len;
    ctx->verification_choice = apdu_rx[ctx->apdu_rx_offset++];
    
    if(ctx->verification_choice > verification_choice_SID){
        return -1;
    }
    
    if (SECURITY_CHOICE_1_Ciphertext == ctx->choice){
        return _security_decode(instance, pipeline, mesg);
    }
    else if (SECURITY_CHOICE_0_Plaintext == ctx->choice){
        if(ctx->verification_choice < verification_choice_MAX){
            return Func_verification[ctx->verification_choice](instance, pipeline, mesg);
        }
    }else{
        return -1;
    }
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
    
    if((Security_Mode & 0x80) == 0x00)  //����+RN
    {
        Security_Mode = fifoBuf_getByte(fifo);
    }
    
    if(Security_Mode == 0x01)
    {
        txlen = fifoBuf_getByte(fifo);//RN
        fifoBuf_getData(fifo, ctx->keybuf, txlen);
        
        memcpy((uint8_t*)&ctx->keybuf[txlen], p_apdu, cache->wr);//ʵ������
        *p_apdu++ = SECURITY_Response;//��Ӧ�ֽ�
        *p_apdu++ = 0x00;//����
        *p_apdu++ = cache->wr;//����
        memcpy(p_apdu, (uint8_t*)&ctx->keybuf[txlen], cache->wr);
        txlen = txlen + cache->wr;//����
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
        if((Security_Mode == 0x00) || (Security_Mode == 0x80) || (Security_Mode == 0x83))//00:����SID+MAC 80:SID+MAC  83:SID
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