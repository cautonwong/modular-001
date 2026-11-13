#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "fr_698_esam.h"
#include "parameter_port.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "meter_base_view.h"
#include "domain/common/status.h"
#include "domain/security/security_mode.h"
#include "domain/security/security_request.h"
#include "fr_698_esam_flow.h"

static int fr_698_security_request_check(struct instance_t* instance, struct pipeline_t* pipeline);

int fr_security_request(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint16_t len  = 0;
    int ret = 0;
    
    ctx->security_choice = apdu_rx[ctx->apdu_rx_offset++];
    ctx->apdu_rx_offset += axdr_decode(TYPE_octet_string, apdu_rx+ctx->apdu_rx_offset, &len);
    ctx->security_data = apdu_rx + ctx->apdu_rx_offset;
    ctx->security_data_len = len;
    ctx->apdu_rx_offset += len;
    ctx->verification_choice = apdu_rx[ctx->apdu_rx_offset++];
    
    if(ctx->verification_choice > ND_SECURITY_AUTH_SID){
        return -1;
    }
    ctx->apdu_len_valid = ctx->apdu_rx_offset;
    ret = fr_698_security_request_check(instance, pipeline); 
    if(ret > 0){
        if ((SECURITY_CHOICE_1_Ciphertext == ctx->security_choice)
            ||(SECURITY_CHOICE_0_Plaintext  == ctx->security_choice)){
            ret = fr_698_esam_verify(instance, pipeline, mesg);
            ctx->apdu_rx_offset = ctx->security_data - apdu_rx;
            if(ret < 0)
            {
                ret = -DAR_UNAUTHORIZED;
            }
            return ret;
        }
        return -DAR_UNAUTHORIZED;
    }else{
      
    }
    return ret;
}


int fr_698_esam_plain_read_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t client_index, uint32_t oad)
{
    if ((0 == nd_mb_auth_on(instance)) || (true == fr_698_current_connect_status_get(instance, pipeline, client_index))) //身份认证未启用或当前客户机应用链接状态
    {
        return 1;
    }

    if ((1 == client_index) && nd_mb_term_auth_timeout(instance))//终端
    {
        return 1;
    }

    return nd_security_plain_read_allowed(oad) ? 1 : -1;
}

//红外通道 认证检查
int fr_infrared_auth_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    if (ctx->channel != FR_698_CHL_IR || (nd_mb_infrared_auth_timeout(instance) != 0) || (nd_mb_factor_mode(instance) == true))
    {
        return 1;
    }
    else
    {
        if ((oad == 0x80008300))
        {
            return 1;
        }
    }

    if (cmd == 1)
    {
        return -DAR_SECURITY_AUTH_FAILED;
    }

    if (cmd == 2)
    {

        if( *nd_fr698_power_state(instance) != ND_AC_POWER_ON ) //停电下不支持红外认证
        {
            return -DAR_SECURITY_AUTH_FAILED;
        }

        if ((oad == 0xf1000b00) || (oad == 0xf1000c00))
        {
            return 1;
        }

        return -DAR_SECURITY_AUTH_FAILED;
    }

    if ((oad & 0xF0000000) == 0x00000000) //类1
    {
        return 1;
    }

    if (nd_security_infrared_oad_allowed(oad))
    {
        return 1;
    }

    return -DAR_RW_REJECT;
}

#if 1   //----698安全模式字-----------------------------开始

#if 1

int __explicit_security_mode(struct instance_t*, struct pipeline_t*, uint16_t oi, uint16_t* mode, uint8_t* tmp_buf)
{
    uint8_t* tmp, i, count, maxnum;
    uint16_t oi2;

    *mode = 0xffff;
    tmp = tmp_buf;

    if (dl698_parameter_read(DL698_PARAM_SECURITY_MODE_EXDATA, tmp,
                             SecurityMode_EXData_LEN) != 0)
    {
        return -1;
    }

    count = *tmp++;
    maxnum = (SecurityMode_EXData_LEN - 1) / 4;

    if ((count == 0) || (count > maxnum))
    {
        return -1;
    }


    for (i = 0; i < count; i++)
    {
        oi2 = buf_to_uint16_lsb(tmp);

        if (oi2 == oi)
        {
            *mode = buf_to_uint16_lsb(tmp + 2);
            return 1;
        }

        tmp += 4;
    }

    return -1;
}



uint16_t __security_mode_word_get(struct instance_t* instance, struct pipeline_t* pipeline, uint32_t oad, uint8_t* tmp_buf)
{
    uint8_t flag = 0;
    uint16_t oi;
    uint16_t mode = 0;

    (void)pipeline;

    if (dl698_parameter_read(DL698_PARAM_SECURITY_MODE_EN, &flag, 1) != 0)
    {
        return 0xffff;
    }

    if (flag != 1)
    {
        return 0xffff;
    }

    oi = get_OAD_OI(oad);

    if (__explicit_security_mode(instance, pipeline, oi, &mode, tmp_buf))
    {
        return mode;
    }

    return nd_security_mode_lookup(oi);
}


//判断安全模式字是否符合要求   明文	明文+数据验证码	  密文	  密文+数据验证码
// cmd : 0 get, 1 set, 2 action
static int __security_mode_word_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);

    uint8_t *tmp_buf = nd_fr698_apdu_cache(instance);
    uint8_t client_index = 0; 
    uint16_t security_mode = 0;
    int ret;

    if (CA_TYPE_TER(ctx->CA))//from 终端
    {
        client_index = 1;
    }
    else
    {
        client_index = 0;
    }

    if (nd_mb_factor_mode(instance) == true)
    {
        return 1;
    }


    if (ctx->channel == FR_698_CHL_IR)//红外    
    {
        if (client_index == 1)//拒绝终端通过红外访问
        {
            return -DAR_RW_REJECT;
        }
        if (nd_mb_infrared_auth_timeout(instance) == 0)
        {
            return 1;
        }

        if (oad == 0xf1000B00 || oad == 0xf1000C00)
        {
            return 1;
        }
    }
  
    // 明文合闸特殊处理 
    if (oad == 0x80008300)
    {
        return 1;
    }

    // 明文确认跟随上报状态字
    if (oad == 0x20157f00)
    {
        return 1;
    }

    // ESAM相关参数不判断权限
    if ((cmd == 0) && (get_OAD_OI(oad) == 0xf100))
    {
        return 1;
    }

    security_mode = __security_mode_word_get(instance, pipeline, oad, tmp_buf);

    if (security_mode == 0xffff)
    {
        return 1;
    }

    if ((ctx->security_choice == 0xFF)
        && (cmd != ND_SECURITY_CMD_GET)
        && ((oad == 0xF1000B00) || (oad == 0xF1000C00)))
    {
        return 1;
    }

    ret = nd_security_mode_check(security_mode, cmd, ctx->security_choice,
                                 ctx->verification_choice);
    if (ret == ND_SECURITY_NEED_PLAIN_CHECK)
    {
        ret = fr_698_esam_plain_read_check(instance, pipeline, client_index, oad);
        return (ret >= 0) ? 1 : -DAR_SECURITY_AUTH_NOT_MATCH;
    }
    if (ret == ND_SECURITY_DENY_AUTH)
    {
        return -DAR_SECURITY_AUTH_NOT_MATCH;
    }
    return 1;
}
//安全模式
int security_mode_word_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad)
{
    int r;

    r = __security_mode_word_check(instance, pipeline, cmd, oad);
    if(r < 0)
    {
        return r;  //安全模式不匹配
    }

    r = fr_infrared_auth_check(instance, pipeline, cmd, oad); 

    return r;  //安全模式不匹配

}
#endif
#endif    //----698安全模式字-----------------------------结束




uint8_t fr_698_current_connect_status_get(struct instance_t* instance, struct pipeline_t*, uint8_t client_index)
{
    //struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);

    uint8_t rlt = 0, i;
    uint8_t comm_chn[4] = { FR_698_CHL_IR, FR_698_CHL_RS485_1,  FR_698_CHL_PLC,  FR_698_CHL_RS485_2 };

    if(!nd_mb_valid(instance))
    {
        return 0;
    }

    for (i = 0; i < (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        if (nd_mb_current_connect_channel(instance)[client_index] == comm_chn[i])
        {
            break;
        }
    }
    if (i >= 4)
    {
        return 0;
    }

    rlt |= nd_mb_connect_status(instance)[i][client_index];

    return rlt;
}

#if 1 //--698 安全传输权限判断
//返回 : 1 成功；     0 不需要应答；     ＜ 0 错误dar ；
static int fr_698_security_request_check(struct instance_t* instance, struct pipeline_t* pipeline)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    nd_security_request_t request = {0};
    uint8_t* data = ctx->security_data;
    int result;

    request.security_choice = ctx->security_choice;
    request.verification_choice = ctx->verification_choice;
    request.client_index = CA_TYPE_TER(ctx->CA) ? 1 : 0;
    request.sa_type = (uint8_t)SA_TYPE(ctx->SA);
    request.channel_is_ir = (ctx->channel == FR_698_CHL_IR);
    request.connected = fr_698_current_connect_status_get(
        instance, pipeline, request.client_index);
    request.auth_on = nd_mb_auth_on(instance);
    request.term_auth_timeout = nd_mb_term_auth_timeout(instance);
    request.infrared_auth_timeout = nd_mb_infrared_auth_timeout(instance);
    request.report_response = (ctx->service == SECURITY_Response)
                              && (data[0] == REPORT_Response);
    request.get_request = (data[0] == GET_Request);
    request.oad = ((uint32_t)data[3] << 24) | ((uint32_t)data[4] << 16)
                  | ((uint32_t)data[5] << 8) | data[6];

    if ((request.security_choice == ND_SECURITY_CHOICE_PLAIN)
        && (request.verification_choice == ND_SECURITY_AUTH_RN)
        && request.get_request) {
        request.plain_read_allowed =
            fr_698_esam_plain_read_check(instance, pipeline,
                                         request.client_index,
                                         request.oad) > 0;
        request.any_connected =
            fr_698_current_connect_status_get(instance, pipeline, 0)
            || fr_698_current_connect_status_get(instance, pipeline, 1);
    }

    result = nd_security_request_check(&request);
    if (result == ND_SECURITY_REQUEST_ALLOW) {
        return 1;
    }
    if (result == ND_SECURITY_REQUEST_SILENT) {
        return 0;
    }
    if (result == ND_SECURITY_REQUEST_REJECTED) {
        return -DAR_RW_REJECT;
    }
    return -DAR_UNAUTHORIZED;
}
#endif



#if 1 // 挪地儿  
//上电初始化

#endif


uint32_t fr_698_current_esam_timeout_get(struct instance_t* instance, struct pipeline_t*, uint8_t client_index)
{
    uint8_t i = 0;

    uint8_t comm_chn[4] = { FR_698_CHL_IR, FR_698_CHL_RS485_1,  FR_698_CHL_PLC,  FR_698_CHL_RS485_2 };
    
    for (i = 0; i < (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        if (nd_mb_current_connect_channel(instance)[client_index] == comm_chn[i])
        {
            break;
        }
    }

    if(i >= 4)      
    {
        return 0;
    }
    
    return nd_mb_esam_connect_timeout(instance)[i][client_index];
}

uint8_t fr_698_current_channel_num_get(struct instance_t* instance, struct pipeline_t* pipeline)
{
    uint8_t i = 0;
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);

    uint8_t client_index = 0;
    uint8_t comm_chn[4] = { FR_698_CHL_IR, FR_698_CHL_RS485_1,  FR_698_CHL_PLC,  FR_698_CHL_RS485_2 };
   
    
    if (CA_TYPE_TER(ctx->CA))//终端
    {
        client_index = 1;
    }
    else
    {
        client_index = 0;
    }
    
    for (i = 0; i < (sizeof(comm_chn) / sizeof(comm_chn[0])); i++)
    {
        if (nd_mb_current_connect_channel(instance)[client_index] == comm_chn[i]) // 应用连接时
        {
            break;
        }
    }

    return i;
}
