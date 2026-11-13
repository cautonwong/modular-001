#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "fr_698_esam.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "meter_base_view.h"
#include "domain/common/status.h"
#include "domain/security/esam_result.h"
#include "fr_698_esam_input.h"

static int __verification_SID_MAC(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
static int __verification_RN(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
static int __verification_RN_MAC(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
static int __verification_SID(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);

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

typedef int(*pFunc_verification_choice)(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
     
const pFunc_verification_choice Func_verification[] = 
{
    #define DEF_X(a,b)  b,
    choice_FUNC
    #undef DEF_X
};





static int ___verification_handle(struct instance_t* instance, struct esam_698_input_t *p_input, struct esam_698_output_t *p_output, uint8_t type)
{
    int mac_matches = 1;

    fr_698_handle_esam(instance, p_input, p_output);
    if (type == 1) {
        mac_matches = nd_esam_mac_optional_matches(
            p_output->MAC_data, p_output->MAC_len,
            p_input->MAC_data, p_input->MAC_len);
    }

    return nd_esam_result(p_output->esam_err_no, type, mac_matches, 1);
}


static int __verification_SID_MAC(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint16_t offset = 0;
    uint16_t len = 0;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    int ret = 0;

    fr_698_esam_input_init(ctx, &input, 1);
    fr_698_esam_input_sid(ctx, &input);
    input.MAC_len = fr_698_esam_input_octet(ctx, &offset, &input.MAC_data,
                                            &len);
    fr_698_esam_input_buffers(instance, &input);
    
    output.data         = ctx->security_data;
    output.data_len     = Max698_apdu_rx - 5;
    
    ctx->apdu_len_valid += len + input.MAC_len;

    //return ___verification_handle(instance, &input, &output, 0);
    ret = ___verification_handle(instance, &input, &output, 0);
    if ((ret > 0)&& (output.data_len > 0))
    {
        ret =  output.data_len;
    }

    return ret;


}
extern int check_upreport_esam_rn_mac(struct instance_t* instance, struct esam_698_input_t *p_input, struct esam_698_output_t *p_output);
static int __verification_RN(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t offset = 0;
    uint16_t len = 0;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    uint8_t rn_buf[12+4] = {0};  // ndv_memcopy(nd_mb_report_random_data(instance), ptr_cache, 12);
    uint8_t mac_buf[5] = {0};
    int ret = 0;

    
    fr_698_esam_input_init(ctx, &input, 2);
    input.RN_Data_len = fr_698_esam_input_octet(ctx, &offset,
                                                &input.RN_Data, &len);
    fr_698_esam_input_buffers(instance, &input);
    
    
    ndv_memcopy(rn_buf, nd_mb_report_random_data(instance), 12);
    ndv_memcopy(mac_buf, apdu_rx + offset + 1, 4);

    fr_698_esam_input_store_rn(ctx, input.RN_Data, input.RN_Data_len);
    if(ctx->service == SECURITY_Response){
        if ((*ctx->security_data) == REPORT_Response) // 上报确认
        {
            
            input.RN_Data_len = 12+4; // rn + 00000000
            input.RN_Data = rn_buf;
            input.MAC_len = 4;
            input.MAC_data = mac_buf;
            ret = check_upreport_esam_rn_mac(instance, &input, &output);
            if (nd_mb_esam_chip_abnormal(instance) == ND_EVENT_SET)   //ESAM错误的情况下  不验证MAC 直接返回成功
            {
                ret = 1; 
            }
            if (ret < 0)//MAC验证不过，不能确认，也不需要响应
            {
                ret = 0;
            }
        }
        else
        {
            ret = ___verification_handle(instance, &input, &output, 1);//ESAM错误的情况下  不验证MAC 直接返回成功   
        }
    }
    else
    {
        ret = 1;
    }
    ctx->apdu_len_valid += len + input.RN_Data_len;

    if ((ret > 0) && (output.data_len > 0))
    {
        ret =  output.data_len;
    }
    
    return ret;
}

static int __verification_RN_MAC(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
    uint16_t offset = 0;
    uint16_t len = 0;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    int ret = 0;

    fr_698_esam_input_init(ctx, &input, 3);

    input.RN_Data_len = fr_698_esam_input_octet(ctx, &offset,
                                                &input.RN_Data, &len);
    ctx->apdu_len_valid += len + input.RN_Data_len;

    input.MAC_len = fr_698_esam_input_octet(ctx, &offset, &input.MAC_data,
                                            &len);
    ctx->apdu_len_valid += len + input.MAC_len;

    fr_698_esam_input_store_rn(ctx, input.RN_Data, input.RN_Data_len);
    fr_698_esam_input_buffers(instance, &input);
    
    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len = 0;

    ret = ___verification_handle(instance, &input, &output, 1);

    if ((ret > 0) && (output.data_len > 0))
    {
        ret =  output.data_len;
    }

    return ret;
}

static int __verification_SID(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    int ret = 0;
    
    fr_698_esam_input_init(ctx, &input, 4);
    fr_698_esam_input_sid(ctx, &input);
    fr_698_esam_input_buffers(instance, &input);
    
    output.data     = ctx->security_data;
    output.data_len = Max698_apdu_rx - 5;

    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len  = 0;

    ret =  ___verification_handle(instance, &input, &output, 0);


    if ((ret > 0) && (output.data_len > 0))
    {
        ret =  output.data_len;
    }
    
    return ret;

    
}



int fr_698_esam_verify(struct instance_t* instance, struct pipeline_t* pipeline,
                       nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx =
        (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);

    if (ctx->verification_choice > verification_choice_SID) {
        return -1;
    }
    return Func_verification[ctx->verification_choice](instance, pipeline, mesg);
}
