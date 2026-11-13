#include "application/ports/fifo.h"
#include "fr_698.h"
#include "fr_698_esam.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "fr_698_env.h"
#include "domain/security/security_mode.h"
#include "security_encoder.h"

static int ___get_encode_and_mac(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    uint8_t data_mac[16] = {0};
    
    input.cmd_type              = 0;
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = cache->buf_ptr;
    input.data_len              = ctx->security_data_len;
    input.data_buf              = ctx->apdu_d_rx;
    input.data_buf_len          = Max698_apdu_rx;

    input.RN_Data_len           = ctx->RN_len;
    input.RN_Data               = ctx->RN_data;
    input.data_buf              = nd_fr698_apdu_cache(instance); //临时缓冲区
    input.data_buf_len          = cache_max;            //临时缓冲长度
    if (CA_TYPE_TER(ctx->CA))
        input.from_type = 1;
    
    output.data     = cache->buf_ptr;
    output.data_len = Max698_apdu - 7 - 5; 
    output.MAC_data = data_mac;
    output.MAC_len  = 16;
    
    fr_698_handle_esam(instance, &input, &output);
    if(output.esam_err_no == 0x9000){
        if(output.MAC_len == 4){
            size_t new_wr;

            if(ctx->security_data_len < output.data_len)
                ctx->security_data_len = output.data_len;
            cache->wr = ctx->security_data_len;
            if (nd_security_encode_tail(cache->buf_ptr, cache->buf_size,
                                        cache->wr, output.MAC_data, 4,
                                        &new_wr) != 0) {
                return -1;
            }
            cache->wr = (uint16_t)new_wr;
            return 1;
        }
        if(output.MAC_len == 0){
            size_t new_wr;

            ctx->security_data_len = output.data_len;
            cache->wr = output.data_len;
            if (nd_security_encode_tail(cache->buf_ptr, cache->buf_size,
                                        cache->wr, NULL, 0, &new_wr) != 0) {
                return -1;
            }
            cache->wr = (uint16_t)new_wr;
            return 1;
        }
    }
    return -1;
}


int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
        
    if(ctx->verification_choice > ND_SECURITY_AUTH_SID){
        return -1;
    }
    ctx->security_data_len = cache->wr;
    if ((SECURITY_CHOICE_1_Ciphertext == ctx->security_choice)
      ||(SECURITY_CHOICE_0_Plaintext  == ctx->security_choice)){
        return ___get_encode_and_mac(instance, pipeline, mesg);
    }else{
        return -DAR_OTHERS_ERROR;
    }
}





