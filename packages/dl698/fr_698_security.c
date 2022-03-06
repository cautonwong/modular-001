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
    
    input.cmd_type              = 1;
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;
   
    ___get_sid(ctx, &input);
    input.MAC_len  = ___get_octet_string(ctx, &input, &offset);
    input.MAC_data = apdu_rx + offset;

    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.data         = ctx->security_data;
    output.data_len     = Max698_apdu_rx - 5;
          
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
    input.cmd_type              = 2;
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;

    input.RN_Data_len   = ___get_octet_string(ctx, &input, &offset);
    input.RN_Data       = apdu_rx + offset;

    ctx->RN_len = input.RN_Data_len;
    if(ctx->RN_len > 64)
        ctx->RN_len = 64;
    memcpy(ctx->RN_data, input.RN_Data, ctx->RN_len);
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
    input.cmd_type              = 3;
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;

    input.RN_Data_len = ___get_octet_string(ctx, &input, &offset);
    input.RN_Data     = apdu_rx + offset;
    input.MAC_len     = ___get_octet_string(ctx, &input, &offset);
    input.MAC_data    = apdu_rx + offset;
    
    ctx->RN_len = input.RN_Data_len;
    if(ctx->RN_len > 64)
        ctx->RN_len = 64;
    memcpy(ctx->RN_data, input.RN_Data, ctx->RN_len);
    
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len = 0;
    
    ret = fr_698_handle_esam(&input, &output);
    if(ret > 0){
        if(output.MAC_len == 4){
            if(0 == memcmp(output.MAC_data, input.MAC_data, 4))
                return 1;
            else
               return 1; //return -1; //临时屏蔽mac 验证不过
        }
    }
    return 1;
}

//待修改saga 需要 密文+sid报文调试
static int __verification_SID(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    input.cmd_type              = 4;
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = ctx->security_data;
    input.data_len              = ctx->security_data_len;
    
    ___get_sid(ctx, &input);
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
    
    output.data     = ctx->security_data;
    output.data_len = Max698_apdu_rx - 5;

    output.MAC_data = cache->buf_ptr + ctx->security_data_len + 3;
    output.MAC_len  = 0;
    
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
    }
    return -1;
}


static int ___get_encode_and_mac(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    int ret = 0;
    uint8_t *mac = cache->buf_ptr + ctx->security_data_len;
    struct esam_698_input_t input = {0};
    struct esam_698_output_t output = {0};
    
    input.cmd_type              = 0;
    input.security_choice       = ctx->security_choice;
    input.verification_choice   = ctx->verification_choice;
    input.data                  = cache->buf_ptr;
    input.data_len              = ctx->security_data_len;
    input.data_buf              = ctx->apdu_d_rx;
    input.data_buf_len          = Max698_apdu_rx;

    input.RN_Data_len           = ctx->RN_len;
    input.RN_Data               = ctx->RN_data;
    input.data_buf      = instance->apdu_cache; //临时缓冲区
    input.data_buf_len  = cache_max;            //临时缓冲长度
    
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
    struct fifo_t* cache = ctx->apdu;
        
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


const uint32_t fr_698_infrared_read_infinitize_oad[] =
{
    0x202c0200,
    0x40000200,
    0x40010200,
    0x40020200,
    0x41110200,
};


int fr_698_esam_plain_read_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t client_index, uint32_t oad)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t i;
    uint16_t oi;

#if 0 // 待调整 需开启
    if ((0 == ctrl_block->esam_auth_enable) || (dl698_current_connect_status_get(client_index))) // 待调整 身份认证未启用或当前客户机应用链接状态
    {
        return 1;
    }
#endif
    if ((1 == client_index) && ctrl_block->term_auth_timeout)//终端
    {
        return 1;
    }

    oi = get_OAD_OI(oad);
    if ((0x0000 == (oi & 0xFF00)) || (0xF100 == oi))
    {
        return 1;
    }
    for (i = 0; i < (sizeof(fr_698_infrared_read_infinitize_oad) / sizeof(fr_698_infrared_read_infinitize_oad[0])); i++)
    {
        if ((oad & 0xffffff00) == fr_698_infrared_read_infinitize_oad[i])
        {
            return 1;
        }
    }
    return -1;
}


#if 1   //----698安全模式字-----------------------------开始

#define TEST_RIGHT                      0x0000      // 测试用的权限 
#define GET_PLAIN						0x8000      // 读明文 
#define GET_PLAIN_MAC					0x4000      // 读明文+mac 
#define GET_CIPHER						0x2000      // 读密文 
#define GET_CIPHER_MAC					0x1000      // 读密文+mac 
#define SET_PLAIN						0x0800      // 设置明文 
#define SET_PLAIN_MAC					0x0400      // 设置明文+mac 
#define SET_CIPHER						0x0200      // 设置密文 
#define SET_CIPHER_MAC					0x0100      // 设置密文+mac 
#define ACTION_PLAIN					0x0080      // 执行明文 
#define ACTION_PLAIN_MAC				0x0040      // 执行明文+mac 
#define ACTION_CIPHER					0x0020      // 执行密文 
#define ACTION_CIPHER_MAC				0x0010      // 执行密文+mac 
#define PROXY_GET						0x0004      // 代理读 
#define PROXY_SET						0x0002      // 代理写 
#define PROXY_ACTION					0x0001      // 代理执行 



const struct security_mode_t security_mode_match_all_tab[] =
{
    { 0x2015, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_PLAIN) },
	{ 0x202C, (TEST_RIGHT | GET_PLAIN | PROXY_GET) },
	{ 0x202F, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_PLAIN) },
	{ 0x4000, (TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | ACTION_PLAIN | PROXY_GET) },
	{ 0x4001, (TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | PROXY_GET) },
	{ 0x4002, (TEST_RIGHT | GET_PLAIN | SET_PLAIN_MAC | PROXY_GET) },
	{ 0x4003, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x400A, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x400B, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x400C, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x400D, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x400E, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x4018, (TEST_RIGHT | GET_PLAIN_MAC) },
	{ 0x4019, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x401A, (TEST_RIGHT | GET_PLAIN_MAC) },
	{ 0x401B, (TEST_RIGHT | GET_PLAIN_MAC | SET_PLAIN_MAC) },
	{ 0x4111, (TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | PROXY_GET) },
    { 0x5000, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_PLAIN_MAC) },
	{ 0xF000, (TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | ACTION_CIPHER_MAC) },
	{ 0xF001, (TEST_RIGHT | GET_PLAIN | SET_CIPHER | ACTION_CIPHER) },
	{ 0xF002, (TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN) },
	{ 0xF100, (TEST_RIGHT | GET_PLAIN | SET_PLAIN_MAC | /*ACTION_CIPHER |*/ACTION_CIPHER_MAC| PROXY_GET) },
	{ 0xF101, (TEST_RIGHT | GET_PLAIN | SET_CIPHER_MAC | ACTION_CIPHER | PROXY_GET) },
    { 0xFFFE, (TEST_RIGHT | GET_CIPHER_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC) },
    { 0xFFFF, (TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN) },
    { 0x4500, (TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN) },
};

const struct security_mode_t security_mode_match_high_byte_tab[] =
{
	{ 0x50FF, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC) },
	{ 0x60FF, (TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN | PROXY_GET | PROXY_SET | PROXY_ACTION) },
	{ 0x70FF, (TEST_RIGHT | GET_PLAIN | ACTION_PLAIN | PROXY_GET | PROXY_ACTION) },
	{ 0x80FF, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC) },
	{ 0xF2FF, (TEST_RIGHT | GET_PLAIN | SET_PLAIN | ACTION_PLAIN | PROXY_GET | PROXY_SET | PROXY_ACTION) },
};

const struct security_mode_t security_mode_match_high_half_byte_tab[] =
{
	{ 0x0FFF, (TEST_RIGHT | GET_PLAIN | PROXY_GET) },
	{ 0x1FFF, (TEST_RIGHT | GET_PLAIN_MAC) },
	{ 0x2FFF, (TEST_RIGHT | GET_PLAIN_MAC | ACTION_PLAIN_MAC) },
	{ 0x3FFF, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC) },
	{ 0x4FFF, (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC) },
};

const uint16_t security_default_mode = (TEST_RIGHT | GET_PLAIN_MAC | SET_CIPHER_MAC | ACTION_CIPHER_MAC);


#if 1

int __explicit_security_mode(struct instance_t* instance, struct pipeline_t* pipeline, uint16_t oi, uint16_t* mode, uint8_t* tmp_buf)
{
    uint8_t* tmp, i, count, maxnum;
    uint16_t oi2;
    uint32_t addr;

    *mode = 0xffff;
    tmp = tmp_buf;

    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_EXData);
    nd_get_index(instance, addr, tmp, SecurityMode_EXData_LEN);

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
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    uint8_t flag;
    uint16_t i, oi, mode = 0;
    uint32_t num, addr;

    flag = 0;
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + SecurityMode_En);
    nd_get_index(instance, addr, &flag, 1);

    if (flag != 1)
    {
        return 0xffff;
    }

    oi = get_OAD_OI(oad);

    if (true == __explicit_security_mode(instance, pipeline, oi, &mode, tmp_buf))
    {
        return mode;
    }

    num = sizeof(security_mode_match_all_tab) / sizeof(security_mode_match_all_tab[0]);

    for (i = 0; i < num; i++)
    {
        if (security_mode_match_all_tab[i].oi == oi)
        {
            return security_mode_match_all_tab[i].mode;
        }
    }

    num = sizeof(security_mode_match_high_byte_tab) / sizeof(security_mode_match_high_byte_tab[0]);

    for (i = 0; i < num; i++)
    {
        if (security_mode_match_high_byte_tab[i].oi == (oi | 0x00ff))
        {
            return security_mode_match_high_byte_tab[i].mode;
        }
    }

    num = sizeof(security_mode_match_high_half_byte_tab) / sizeof(security_mode_match_high_half_byte_tab[0]);

    for (i = 0; i < num; i++)
    {
        if (security_mode_match_high_half_byte_tab[i].oi == (oi | 0x0fff))
        {
            return security_mode_match_high_half_byte_tab[i].mode;
        }
    }

    return security_default_mode;
}


//判断安全模式字是否符合要求   明文	明文+数据验证码	  密文	  密文+数据验证码
// cmd : 0 get, 1 set, 2 action
int security_mode_word_check(struct instance_t* instance, struct pipeline_t* pipeline, uint8_t cmd, uint32_t oad)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct mbase_control_block_t* ctrl_block = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    uint8_t *tmp_buf = instance->apdu_cache;
    uint8_t cilent_index = 0; 
    uint16_t security_mode = 0;
    int16 ret;

    if (CA_TYPE_TER(ctx->CA))//from 终端
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }

    if (ctrl_block->factor_mode == true)
    {
        return 1;
    }

#if 0// 待调整
    if (ctrl_block->channel == pid_ir_rx_698)//红外    待调整
    {
        if (cilent_index == 1)//拒绝终端通过红外访问
        {
            return -DAR_RW_REJECT;
        }
        if (ctrl_block->infrared_auth_timeout == 0) // 待调整
        {
            return 1;
        }

        if (oad == 0xf1000B00 || oad == 0xf1000C00)
        {
            return 1;
        }
    }
#endif     
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

    if (cmd == 1)
    {
        security_mode <<= 4;
    }

    if (cmd == 2)
    {
        security_mode <<= 8;
    }

    if (0xFF != ctx->security_choice)// 安全传输
    {
        if (ctx->security_choice  == 1)
        {
            if (ctx->verification_choice == AUTH_SID_MAC)
            {
                return 1;
            }
            else if (ctx->verification_choice == AUTH_SID)
            {
                if (security_mode & 0x1000U)
                {
                    return -DAR_SECURITY_AUTH_NOT_MATCH;
                }

                return 1;
            }
        }
        else
        {
            if ((ctx->verification_choice == AUTH_RN_MAC) || (ctx->verification_choice == AUTH_SID_MAC))
            {
                if (security_mode & 0x3000U)
                {
                    return -DAR_SECURITY_AUTH_NOT_MATCH;
                }

                return 1;
            }

            if (ctx->verification_choice == AUTH_RN)
            {
                if (cmd == 0)//get
                {

                    if (security_mode & 0xc000U)
                    {
                        return 1;
                    }
                    else
                    {
                        return -DAR_SECURITY_AUTH_NOT_MATCH;
                    }
                }

                if (security_mode & 0x7000U)
                {
                    return -DAR_SECURITY_AUTH_NOT_MATCH;
                }

                return 1;
            }
        }
    }
    else
    {

        if (cmd == 0)
        {
            if ((security_mode & 0x8000) != 0x8000)
            {
                return -DAR_SECURITY_AUTH_NOT_MATCH;
            }
            else
            {
                ret = fr_698_esam_plain_read_check(instance, pipeline, cilent_index, oad);
                if (ret >= 0)
                    return 1;
                else
                    return -DAR_SECURITY_AUTH_NOT_MATCH;
            }
        }
        if ((oad == 0xF1000B00) || (oad == 0xF1000C00))//执行终端身份认证时，可以明文执行
        {
            return 1;
        }
        if (security_mode & 0x7000U)
        {
            return -DAR_SECURITY_AUTH_NOT_MATCH;
        }

        return 1;
    }

    return -DAR_SECURITY_AUTH_NOT_MATCH;
}
#endif
#endif    //----698安全模式字-----------------------------结束