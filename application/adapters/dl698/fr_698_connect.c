#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
struct instance_t;
#include "fr_698.h"
#include "domain/events/legacy/gs_record.h"
#include "application/adapters/dl698/fr_698.h"
#include "application/adapters/dl698/fr_698_esam.h"
#include "meter_base_view.h"



enum connect_result_e
{
    CONNECT_RESULT_SUCCESS               = 0,    /**< 允许建立应用连接 */
    CONNECT_RESULT_PASSWORD_ERR          = 1,    /**< 密码错误 */
    CONNECT_RESULT_SYMMETRIC_DECODE_ERR  = 2,    /**< 对称解密错误 */
    CONNECT_RESULT_ASYMMETRIC_DECODE_ERR = 3,    /**< 非对称解密错误 */
    CONNECT_RESULT_SIGNATURE_ERR         = 4,    /**< 签名错误 */
    CONNECT_RESULT_VERSION_NOT_MATCH     = 5,    /**< 协议版本不匹配 */
    CONNECT_RESULT_ESM_ERR               = 6,    /**< ESAM */
    CONNECT_RESULT_OTHERS_ERR            = 255,  /**< 其他错误 */
};

struct connect_cfg_info_t
{
    uint32_t version;
    uint8_t  protocal_conformance[8]; //=bit-string(SIZE(64))
    uint8_t  function_conformance[16]; //=bit-string(SIZE(128))   
    uint16_t link_send_buf_size;
    uint16_t link_recv_buf_size;   
    uint16_t apply_req_buf_size;
    uint16_t apply_res_buf_size;   
    uint8_t  client_window_num;
    uint16_t client_process_apdu_size;
    uint16_t connect_timeout;
};




struct factory_version_t
{
    char vendor_code[4];        //厂商代码 visible-string(SIZE(4))，
    char software_version[4];   //软件版本号 visible-string(SIZE(4))，
    char software_date[6];      //软件版本日期 visible-string(SIZE(6))，
    char hardware_version[4];   //硬件版本号 visible-string(SIZE(4))，
    char hardware_date[6];      //硬件版本日期 visible-string(SIZE(6))，
    char vendor_infomation[8];  //厂商扩展信息 visible-string(SIZE(8))
};

//通道顺序 :红外、485-1，载波，485-2；
const struct connect_request_t default_connect = 
{
    0x0016,
    {0xFF,0xFF,0xFF,0xff,0xc0,0x00,0x00,0x00},
    {0xFF,0xFE,0xC4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    0x0200,
    0x0200,
    0x01,
    0x07d0,
    0x00001C20,
    0x02,
};

struct connect_request_t get_default_connect_para(uint8_t channel)
{
    struct connect_request_t default_connect_para;
    ndv_memcopy((uint8_t *)&default_connect_para,  (uint8_t *)&default_connect, sizeof(struct connect_request_t));
    if(channel == FR_698_CHL_IR)
    {
        default_connect_para.send_frm_size = 200;
        default_connect_para.recv_frm_size = 200;
    }
    else if(channel == FR_698_CHL_PLC)
    {
        default_connect_para.send_frm_size = 255; //
        default_connect_para.recv_frm_size = 255;
    }


    return default_connect_para;
}



struct connect_request_t cfg_connect = {
    0x0016,
    {0xFF,0xFF,0xFF,0xff,0xc0,0x00,0x00,0x00},
    {0xFF,0xFE,0xC4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    0x0200,
    0x0200,
    0x01,
    0x07d0,
    0x00001C20,
    0x00
};
    
    


static int __common_process(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
static int __password_process(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
static int __symmetry_process(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
static int __signature_process(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);


#define Security_FUNC   \
    DEF_X(NullSecurity,         __common_process)\
    DEF_X(PasswordSecurity,     __password_process)\
    DEF_X(SymmetrySecurity,     __symmetry_process)\
    DEF_X(SignatureSecurity,    __signature_process)\
              

enum connect_mode_e
{
    #define DEF_X(a,b)  a,
    Security_FUNC
    #undef DEF_X
    Security_MAX
};

typedef int(*pFunc_security_process)(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
     
const pFunc_security_process Func_security[] = 
{
    #define DEF_X(a,b)  b,
    Security_FUNC
    #undef DEF_X
};


static int __common_process(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;

    uint8_t *p_apdu = cache->buf_ptr;
    uint8_t buf[70];
    uint8_t *p_connect_timeout = cache->buf_ptr;

    memcpy(buf, apdu_rx + ctx->apdu_rx_offset, 38);
    ctx->apdu_rx_offset += 38;
    
    cache->wr = cache->wr - 3;
    *(p_apdu+cache->wr) |= 0x82;  //应答
    cache->wr++;
    *(p_apdu+cache->wr) = ctx->PIID;  //PIID
    cache->wr++;
    memset((p_apdu+cache->wr), 0x30, 32);//厂家信息
    cache->wr = cache->wr + 32;
    
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.pro_ver, 2);//版本号
    cache->wr = cache->wr + 2;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.protocal_conformance, 8);//期望协议一致性块
    cache->wr = cache->wr + 8;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.function_conformance, 16);  //期望的功能一致性块 
    cache->wr = cache->wr + 16;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.send_frm_size, 2);    //发送最大尺寸
    cache->wr = cache->wr + 2;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.recv_frm_size, 2);    //接收最大尺寸
    cache->wr = cache->wr + 2;
    *(p_apdu+cache->wr) = cfg_connect.client_win_size;//客户机接收帧最大窗口尺寸
    cache->wr = cache->wr + 1;

    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.client_apdu_size, 2);    //客户机最大可处理帧尺寸
    cache->wr = cache->wr + 2;

    p_connect_timeout = (p_apdu+cache->wr);
    mem_swap_cpy(p_connect_timeout, (uint8_t*)&cfg_connect.connect_time, 4);     //期望的应用连接超时时间    
    cache->wr = cache->wr + 4;


    //不支持数字签名
    *(p_apdu+cache->wr) = DL698_SEC_OTHERS_ERR;
    cache->wr++;
    *(p_apdu+cache->wr) = 0;//认证附加信息
    cache->wr++;

    return DL698_SEC_OTHERS_ERR;

}

static int __password_process(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{

    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;
    
    uint8_t *p_apdu = cache->buf_ptr;
    uint8_t buf[70];
    uint8_t cilent_index = 0;
    uint32_t esam_session_timeout = 0xffffffffU;
    uint8_t *p_connect_timeout = cache->buf_ptr;

    uint8_t *p_password = cache->buf_ptr;
    uint8_t password[33] = {0};
    uint32_t oad = 0;
    int ret = DL698_SEC_SUCCESS;
    uint8_t channel = 0;
    
    if((ctx)&&(ctx->channel < FR_698_CHL_NUM))
    {
        channel = ctx->channel;
    }



    if (fr_698_get(instance, oad, password, sizeof(password), NULL) < 0)
    {
        ret = DL698_SEC_PASSWORD_ERR;
    }

    if(0 != mem_compare(password, p_password, p_password[0])){
        ret =  DL698_SEC_PASSWORD_ERR;
    }


   
    if (CA_TYPE_TER(ctx->CA))//from 终端
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }
    
    memcpy(buf, apdu_rx + ctx->apdu_rx_offset, 38);
    ctx->apdu_rx_offset += 38;
    
    cache->wr = cache->wr - 3;
    *(p_apdu+cache->wr) |= 0x82;  //应答
    cache->wr++;
    *(p_apdu+cache->wr) = ctx->PIID;  //PIID
    cache->wr++;
    memset((p_apdu+cache->wr), 0x30, 32);//厂家信息
    cache->wr = cache->wr + 32;
    
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.pro_ver, 2);//版本号
    cache->wr = cache->wr + 2;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.protocal_conformance, 8);//期望协议一致性块
    cache->wr = cache->wr + 8;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.function_conformance, 16);  //期望的功能一致性块 
    cache->wr = cache->wr + 16;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.send_frm_size, 2);    //发送最大尺寸
    cache->wr = cache->wr + 2;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.recv_frm_size, 2);    //接收最大尺寸
    cache->wr = cache->wr + 2;
    *(p_apdu+cache->wr) = cfg_connect.client_win_size;//客户机接收帧最大窗口尺寸
    cache->wr = cache->wr + 1;

    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.client_apdu_size, 2);    //客户机最大可处理帧尺寸
    cache->wr = cache->wr + 2;

    p_connect_timeout = (p_apdu+cache->wr);
    mem_swap_cpy(p_connect_timeout, (uint8_t*)&cfg_connect.connect_time, 4);     //期望的应用连接超时时间    
    cache->wr = cache->wr + 4;

    if (ret == DL698_SEC_SUCCESS)
    {
        *(p_apdu+cache->wr) = 0;//允许建立应用连接
        cache->wr++;
        *(p_apdu+cache->wr) = 0;//认证附加信息
        cache->wr++;
        
        if(cilent_index)
        {
            read_esam_info(instance, buf, 20);
        }
        else
        {
            read_esam_info(instance, buf, 5);
        }

        esam_session_timeout = buf_to_uint32_msb(&buf[0]);
        esam_session_timeout = 60 * esam_session_timeout;
        

        if (esam_session_timeout == 0)
        {
            esam_session_timeout = nd_mb_connect_timeout(instance)[channel][cilent_index];
        }

        if (nd_mb_connect_timeout(instance)[channel][cilent_index] > esam_session_timeout)
        {
            nd_mb_connect_timeout(instance)[channel][cilent_index] = esam_session_timeout;
            mem_swap_cpy(p_connect_timeout, (uint8_t*)&nd_mb_connect_timeout(instance)[channel][cilent_index], 4);     //期望的应用连接超时时间
        }
        
        nd_mb_esam_connect_timeout(instance)[channel][cilent_index] = esam_session_timeout;
        
        nd_mb_current_connect_channel(instance)[cilent_index] = channel;
        nd_mb_connect_status(instance)[channel][cilent_index] = true;
        nd_mb_connect_timeout_cnt(instance)[channel][cilent_index] = 0;

        
        MeterWr_obj_list_clear(instance, pipeline, NULL);
        return DL698_SEC_SUCCESS;        
    }
    else
    {
        //不支持数字签名
        *(p_apdu+cache->wr) = DL698_SEC_PASSWORD_ERR;
        cache->wr++;
        *(p_apdu+cache->wr) = 0;//认证附加信息
        cache->wr++;
        return DL698_SEC_OTHERS_ERR;
    }


    
   
}

static int __symmetry_process(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;

    uint8_t *p_apdu = cache->buf_ptr;
	uint8_t buf[70];
    uint8_t  rn_len = 0;
    uint8_t cilent_index = 0;
    uint32_t esam_session_timeout = 0xffffffffU;
    int len_len = 0;
    uint8_t *p_connect_timeout = cache->buf_ptr;
    uint8_t channel = 0;
    
    if((ctx)&&(ctx->channel < FR_698_CHL_NUM))
    {
        channel = ctx->channel;
    }

    
    if (CA_TYPE_TER(ctx->CA))//from 终端
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }
    
    memcpy(buf, apdu_rx + ctx->apdu_rx_offset, 38);
    ctx->apdu_rx_offset += 38;
    
    cache->wr = cache->wr - 3;
    *(p_apdu+cache->wr) |= 0x82;  //应答
    cache->wr++;
    *(p_apdu+cache->wr) = ctx->PIID;  //PIID
    cache->wr++;
    memset((p_apdu+cache->wr), 0x30, 32);//厂家信息
    cache->wr = cache->wr + 32;
    
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.pro_ver, 2);//版本号
    cache->wr = cache->wr + 2;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.protocal_conformance, 8);//期望协议一致性块
    cache->wr = cache->wr + 8;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.function_conformance, 16);  //期望的功能一致性块 
    cache->wr = cache->wr + 16;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.send_frm_size, 2);    //发送最大尺寸
    cache->wr = cache->wr + 2;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.recv_frm_size, 2);    //接收最大尺寸
    cache->wr = cache->wr + 2;
    *(p_apdu+cache->wr) = cfg_connect.client_win_size;//客户机接收帧最大窗口尺寸
    cache->wr = cache->wr + 1;

    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.client_apdu_size, 2);    //客户机最大可处理帧尺寸
    cache->wr = cache->wr + 2;

    p_connect_timeout = (p_apdu+cache->wr);
    mem_swap_cpy(p_connect_timeout, (uint8_t*)&cfg_connect.connect_time, 4);     //期望的应用连接超时时间    
    cache->wr = cache->wr + 4;

    nd_mb_connect_timeout(instance)[channel][cilent_index] = cfg_connect.connect_time;


    len_len = esam_consult_key(instance, CA_TYPE_TER(ctx->CA) , buf+1 );

    if(len_len > 0)
    {
        *(p_apdu+cache->wr) = 0;//允许建立应用连接
        cache->wr++;
        *(p_apdu+cache->wr) = 1;//认证附加信息
        cache->wr++;
        
        if(cilent_index)
        {
            rn_len = 64;
        }
        else
        {
            rn_len = 48;
        }
        
        *(p_apdu+cache->wr) = rn_len;//长度
        cache->wr++;
        memcpy((p_apdu+cache->wr), buf+1,rn_len);//随机数
        cache->wr = cache->wr + rn_len;
        *(p_apdu+cache->wr) = 4;
        cache->wr++;
        memcpy((p_apdu+cache->wr), buf+1+rn_len,4);//MAC
        cache->wr = cache->wr + 4;

        if(cilent_index)
        {
            read_esam_info(instance, buf, 20);
        }
        else
        {
            read_esam_info(instance, buf, 5);
        }

        esam_session_timeout = buf_to_uint32_msb(&buf[0]);
        esam_session_timeout = 60 * esam_session_timeout;
        

        if (esam_session_timeout == 0)
        {
            esam_session_timeout = nd_mb_connect_timeout(instance)[channel][cilent_index];
        }

        if (nd_mb_connect_timeout(instance)[channel][cilent_index] > esam_session_timeout)
        {
            nd_mb_connect_timeout(instance)[channel][cilent_index] = esam_session_timeout;
            mem_swap_cpy(p_connect_timeout, (uint8_t*)&nd_mb_connect_timeout(instance)[channel][cilent_index], 4);     //期望的应用连接超时时间
        }
        
        nd_mb_esam_connect_timeout(instance)[channel][cilent_index] = esam_session_timeout;
        
        nd_mb_current_connect_channel(instance)[cilent_index] = channel;
        nd_mb_connect_status(instance)[channel][cilent_index] = true;
        nd_mb_connect_timeout_cnt(instance)[channel][cilent_index] = 0;

        
        if(1 == cilent_index)//建立终端应用链接时，要失效终端身份认证
        {
            nd_mb_term_auth_timeout(instance) = 0;
        }

//        nd_mb_clientaddr(instance) = ctx->CA;

        
        MeterWr_obj_list_clear(instance, pipeline, NULL);
        return DAR_SUCCESS;
    }
    else
    {
        *(p_apdu+cache->wr) = DL698_SEC_SYMMETRIC_DECODE_ERR;//对称解密错误
        cache->wr++;
        *(p_apdu+cache->wr) = 0;//认证附加信息
        cache->wr++;

        nd_mb_connect_status(instance)[channel][cilent_index] = false;
        nd_mb_connect_timeout(instance)[channel][cilent_index]   = 0;
        return DAR_ESAM_CHECK_FAILED;
    }
}

static int __signature_process(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;

    uint8_t *p_apdu = cache->buf_ptr;
    uint8_t buf[70];
    uint8_t *p_connect_timeout = cache->buf_ptr;

    memcpy(buf, apdu_rx + ctx->apdu_rx_offset, 38);
    ctx->apdu_rx_offset += 38;
    
    cache->wr = cache->wr - 3;
    *(p_apdu+cache->wr) |= 0x82;  //应答
    cache->wr++;
    *(p_apdu+cache->wr) = ctx->PIID;  //PIID
    cache->wr++;
    memset((p_apdu+cache->wr), 0x30, 32);//厂家信息
    cache->wr = cache->wr + 32;
    
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.pro_ver, 2);//版本号
    cache->wr = cache->wr + 2;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.protocal_conformance, 8);//期望协议一致性块
    cache->wr = cache->wr + 8;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.function_conformance, 16);  //期望的功能一致性块 
    cache->wr = cache->wr + 16;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.send_frm_size, 2);    //发送最大尺寸
    cache->wr = cache->wr + 2;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.recv_frm_size, 2);    //接收最大尺寸
    cache->wr = cache->wr + 2;
    *(p_apdu+cache->wr) = cfg_connect.client_win_size;//客户机接收帧最大窗口尺寸
    cache->wr = cache->wr + 1;

    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.client_apdu_size, 2);    //客户机最大可处理帧尺寸
    cache->wr = cache->wr + 2;

    p_connect_timeout = (p_apdu+cache->wr);
    mem_swap_cpy(p_connect_timeout, (uint8_t*)&cfg_connect.connect_time, 4);     //期望的应用连接超时时间    
    cache->wr = cache->wr + 4;


    //不支持数字签名
    *(p_apdu+cache->wr) = DL698_SEC_SIGNATURE_ERR;
    cache->wr++;
    *(p_apdu+cache->wr) = 0;//认证附加信息
    cache->wr++;

    return DL698_SEC_SIGNATURE_ERR;
}

static uint32_t __get_connect_info(uint32_t val, uint32_t def_val)
{
    if ((val == 0) || (val > def_val)){
        return def_val;
    }
    return val;
}

static void __consistency_block(uint8_t *data,  uint8_t* cfg_data, const uint8_t* default_data,uint8_t len)
{
    for (uint8_t i = 0; i < len; i++){
        data[i] = cfg_data[i] & default_data[i];
    }
}

static void ___get_consult_info(struct connect_request_t *connect, struct connect_request_t *connect_cache, const struct connect_request_t *default_connect)
{
    __consistency_block(connect->protocal_conformance, connect_cache->protocal_conformance, default_connect->protocal_conformance, 8);
    __consistency_block(connect->function_conformance, connect_cache->function_conformance, default_connect->function_conformance, 16);

    connect->send_frm_size      = __get_connect_info(connect_cache->recv_frm_size, default_connect->send_frm_size);
    connect->recv_frm_size      = __get_connect_info(connect_cache->send_frm_size, default_connect->recv_frm_size);
    connect->client_win_size    = __get_connect_info(connect_cache->client_win_size, default_connect->client_win_size);
    connect->client_apdu_size   = __get_connect_info(connect_cache->client_apdu_size, default_connect->client_apdu_size);
    connect->connect_time       = __get_connect_info(connect_cache->connect_time, default_connect->connect_time);
}


int fr_handle_disconnect(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    uint8_t cilent_index = 0;
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t offset= 0;

    if (CA_TYPE_TER(ctx->CA))//终端
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }
    
        
    nd_mb_connect_timeout(instance)[ctx->channel][cilent_index] = 0;
    nd_mb_esam_connect_timeout(instance)[ctx->channel][cilent_index] = 0;
    nd_mb_connect_status(instance)[ctx->channel][cilent_index] = false;
    
    if ((nd_mb_connect_status(instance)[ctx->channel][0] == false)&& (nd_mb_connect_status(instance)[ctx->channel][1] == false))
    {
        MeterWr_obj_list_clear(instance, pipeline, NULL);
    }
    
    //复位ESAM
    //memcpy(p + cache->wr, apdu_rx+ctx->apdu_rx_offset, 2);
    //cache->wr += 2;
    //ctx->apdu_rx_offset += 1; 
    //cache->wr += 1;
    offset = cache->wr;
    p[offset-2] = p[offset-1];
    p[offset-1] = 0;
    return 1;
}




int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    //struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct connect_request_t connect = {0};
    struct connect_request_t default_connect_para = {0};
    struct connect_request_t cfg_connect_cache = {0};
    int result = DAR_OTHERS_ERROR;
    uint16_t offset = 0;
    uint8_t channel = 0;
    
    if((ctx)&&(ctx->channel < FR_698_CHL_NUM))
    {
        channel = ctx->channel;
    }


    
    // 通配地址 拒绝服务   //此代码是否有问题？？？  待调整
    if(SA_TYPE_GENERAL == SA_TYPE(ctx->SA)) {
        result = DAR_OTHERS_ERROR;
    }
    
    //memcpy(&connect, apdu_rx+ctx->apdu_rx_offset, sizeof(struct connect_request_t));
    offset = 0;
    mem_swap_cpy((uint8_t*)&connect.pro_ver, (apdu_rx +  ctx->apdu_rx_offset + offset), 2);//版本号
    offset += 2;
    
    ndv_memcopy((uint8_t*)&connect.protocal_conformance,(apdu_rx +  ctx->apdu_rx_offset + offset),  8);//期望协议一致性块
    offset += 8;

    ndv_memcopy((uint8_t*)&connect.function_conformance,(apdu_rx +  ctx->apdu_rx_offset + offset),  16);  //期望的功能一致性块 
    offset += 16;

    mem_swap_cpy( (uint8_t*)&connect.send_frm_size,(apdu_rx +  ctx->apdu_rx_offset + offset),  2);    //发送最大尺寸
    offset += 2;

    mem_swap_cpy( (uint8_t*)&connect.recv_frm_size, (apdu_rx +  ctx->apdu_rx_offset + offset), 2);    //接收最大尺寸
    offset += 2;

    connect.client_win_size = *(apdu_rx +  ctx->apdu_rx_offset + offset);//客户机接收帧最大窗口尺寸
    offset += 1;

    mem_swap_cpy((uint8_t*)&connect.client_apdu_size,(apdu_rx +  ctx->apdu_rx_offset + offset),  2);    //客户机最大可处理帧尺寸
    offset += 2;

    mem_swap_cpy((uint8_t*)&connect.connect_time,(apdu_rx +  ctx->apdu_rx_offset + offset),  4);        //期望的应用链接超时时间
    offset += 4;

    connect.connect_mode = *(apdu_rx +  ctx->apdu_rx_offset + offset);// 认证模式
    offset += 1;

    cfg_connect.connect_mode = connect.connect_mode;
    
    ctx->apdu_rx_offset += sizeof(struct connect_request_t);
    
    if (connect.connect_mode > 4){
        result = DAR_OTHERS_ERROR;
    }

    //先暂存
    ndv_memcopy((uint8_t *)&cfg_connect_cache, (uint8_t *)&cfg_connect, sizeof(struct connect_request_t));
    default_connect_para = get_default_connect_para(channel);
    ___get_consult_info(&cfg_connect, &connect, &default_connect_para);
    
    if (Security_MAX > connect.connect_mode){
        if(Func_security[connect.connect_mode]){
            result = Func_security[connect.connect_mode](instance, pipeline, mesg);
            if (result == DL698_SEC_SUCCESS)
            {
                nd_mb_clientaddr(instance)[channel] = ctx->CA;
                //mbase_cb->connect_mode[ctx->channel] = cfg_connect.connect_mode;
            }
            else
            {
                //失败恢复应用链接信息
                ndv_memcopy((uint8_t *)&cfg_connect, (uint8_t *)&cfg_connect_cache, sizeof(struct connect_request_t));
            }
        }
    }
	return 1;

}


int get_CONNECT_Request_len(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    uint16_t len = 0;
    int offset = 0;
    uint8_t *apdu = ctx->apdu_d_rx + ctx->apdu_rx_offset;

    offset = 37;
    apdu += offset;
    switch (apdu[0])
    {
    case NullSecurity:
        offset += 2;
        break;

    case PasswordSecurity:
        offset += 1;
        apdu++;
        len += axdr_decode(TYPE_octet_string, apdu, &len);
        offset += len;
        break;

    case SymmetrySecurity:
    case SignatureSecurity:
        offset += 1;
        apdu++;
        len += axdr_decode(TYPE_octet_string, apdu, &len);
        offset += len;
        apdu += len;
        len += axdr_decode(TYPE_octet_string, apdu, &len);
        offset += len;
        break;
    default:
        return -1;
        break;
    }
    return offset;
}










//给定通道号的 断开应用链接
int fr_handle_disconnect2(struct instance_t* instance,  struct pipeline_t* pipeline,  uint8_t cilent_index, uint8_t channel)
{
    //uint8_t result = DAR_SUCCESS;
   
    if (cilent_index > 1)//终端
    {
        return -1;
    }

    if (channel > 3)//终端
    {
        return -1;
    }

     
    nd_mb_connect_timeout(instance)[channel][cilent_index] = 0;
    nd_mb_esam_connect_timeout(instance)[channel][cilent_index] = 0;
    nd_mb_connect_status(instance)[channel][cilent_index] = false;
    
    if ((nd_mb_connect_status(instance)[channel][0] == false)&& (nd_mb_connect_status(instance)[channel][1] == false))
    {
        MeterWr_obj_list_clear(instance, pipeline, NULL);
    }
    
    //复位ESAM
    return 1;
}

// 断开所有通道 应用链接
int fr_handle_disconnect_all_channel(struct instance_t* instance,  struct pipeline_t* pipeline,  uint8_t cilent_index)
{
    uint8_t i;
   

    for(i = 0; i< 4; i++)
    {
         fr_handle_disconnect2(instance,  pipeline, cilent_index,  i);
    }

    return 1;

}
