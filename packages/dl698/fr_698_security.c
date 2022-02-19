#include "nidavellir.h"
#include "fr_698.h"
#include "fr_698_esam.h"

static int __verification_SID_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __verification_RN(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __verification_RN_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __verification_SID(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);


#define choice_FUNC                                        \
    DEF_X(verification_choice_SID_MAC,  __verification_SID_MAC)\
    DEF_X(verification_choice_RN,       __verification_RN)\
    DEF_X(verification_choice_RN_MAC,   __verification_RN_MAC)\
    DEF_X(verification_choice_SID,      __verification_SID)\
              

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


static uint16_t ___get_octet_string(struct context_698_t* ctx, struct esam_698_input_t *input, uint16_t *offset)
{
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t len = 0;
    ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, apdu_rx+ctx->apdu_rx_offset, &len);
    *offset = ctx->apdu_rx_offset;
    ctx->apdu_rx_offset += len;
    return len;
}


static int ___get_sid(struct context_698_t* ctx, struct esam_698_input_t *input)
{
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t offset = 0;
    
    input->sid_id                = buf_to_uint32_msb(apdu_rx+ctx->apdu_rx_offset);
    ctx->apdu_rx_offset += 4;
    input->AttachData_len = ___get_octet_string(ctx, input, &offset);
    input->AttachData = apdu_rx + offset;
    return 1;
}
    
static int __verification_SID_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t offset = 0; 
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
          
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;
   
    ___get_sid(ctx, &input);
    input.MAC_len  = ___get_octet_string(ctx, &input, &offset);
    input.MAC_data = apdu_rx + offset;

    input.data_buf              = cache->buf_ptr;
    input.data_buf_len          = Max698_apdu;
    
    output.data        = ctx->security_data;
    output.data_len    = Max698_apdu_rx - 5;
          
    ret = fr_698_handle_esam(&input, &output);
    if(ret > 0){
      if(output.MAC_len == 4){

        }
    }
    return 1;
}

static int __verification_RN(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t offset = 0;    
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
          
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;

    input.RN_Data_len   = ___get_octet_string(ctx, &input, &offset);
    input.RN_Data       = apdu_rx + offset;
    
    input.data_buf              = ctx->apdu_d_rx;
    input.data_buf_len          = Max698_apdu_rx;
            
    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len = 0;
    
    ret = fr_698_handle_esam(&input, &output);
    if(ret > 0){
      if(output.MAC_len == 4){

        }
    }
    return 1;
}

static int __verification_RN_MAC(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t offset = 0; 
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
          
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;

    input.RN_Data_len = ___get_octet_string(ctx, &input, &offset);
    input.RN_Data     = apdu_rx + offset;
    input.MAC_len     = ___get_octet_string(ctx, &input, &offset);
    input.MAC_data    = apdu_rx + offset;
    
    input.data_buf              = ctx->apdu_d_rx;
    input.data_buf_len          = Max698_apdu_rx;
    
    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len = 0;
    
    ret = fr_698_handle_esam(&input, &output);
    if(ret > 0){
      if(output.MAC_len == 4){

        }
    }
    return 1;
}

//待修改saga 需要 密文+sid报文调试
static int __verification_SID(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t len = 0;
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
          
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;
    
    ___get_sid(ctx, &input);
    
    output.data        = ctx->security_data;
    output.data_len    = Max698_apdu_rx - 5;
            
    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len = 0;
    
    ret = fr_698_handle_esam(&input, &output);
    if(ret > 0){
      if(output.MAC_len == 4){

        }
    }
    return 1;
}


int fr_security_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t len  = 0;
    uint16_t offset  = 0;
    int ret = 0;
    
    ctx->security_choice = apdu_rx[ctx->apdu_rx_offset++];
    ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, apdu_rx+ctx->apdu_rx_offset, &len);
    ctx->security_data = apdu_rx + ctx->apdu_rx_offset;
    ctx->security_data_len = len;
    ctx->apdu_rx_offset += len;
    ctx->verification_choice = apdu_rx[ctx->apdu_rx_offset++];
    
    if(ctx->verification_choice > verification_choice_SID){
        return -1;
    }
    
    if ((SECURITY_CHOICE_1_Ciphertext == ctx->security_choice)
      ||(SECURITY_CHOICE_0_Plaintext  == ctx->security_choice)){
        if(Func_verification[ctx->verification_choice]){
            ret = Func_verification[ctx->verification_choice](instance, pipeline, mesg);
            ctx->apdu_rx_offset = ctx->security_data - apdu_rx;
            return ret;
        }
    }else{
        return -1;
    }
    
#if 0
    if (SECURITY_CHOICE_1_Ciphertext == ctx->security_choice){
        return _security_decode(instance, pipeline, mesg);
    }
    else if (SECURITY_CHOICE_0_Plaintext == ctx->security_choice){
        if(Func_verification[ctx->verification_choice]){
            ret = Func_verification[ctx->verification_choice](instance, pipeline, mesg);
            ctx->apdu_rx_offset = ctx->security_data - apdu_rx;
            return ret;
        }
    }else{
        return -1;
    }
#endif
}



static int ___get_encode_and_mac(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
          
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = cache->buf_ptr;
    input.data_len              = ctx->security_data_len;
    input.data_buf              = ctx->apdu_d_rx;
    input.data_buf_len          = Max698_apdu_rx;
            
    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len = 0;
    
    ret = fr_698_handle_esam(&input, &output);
    if(ret > 0){
      if(output.MAC_len == 4){
            *mac++ = 0x01;
            *mac++ = 0x00;
            *mac++ = 0x04;
            //memcpy(mac, ctx->apdu_d_rx, 4);//MAC
            cache->wr += 3+4;
        }
    }
    return 1;
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
        
    if(ctx->verification_choice > verification_choice_SID){
        return -1;
    }
    ctx->security_data_len = cache->wr;
    if ((SECURITY_CHOICE_1_Ciphertext == ctx->security_choice)
      ||(SECURITY_CHOICE_0_Plaintext  == ctx->security_choice)){
        return ___get_encode_and_mac(instance, pipeline, mesg);
    }else{
        return -1;
    }
}
