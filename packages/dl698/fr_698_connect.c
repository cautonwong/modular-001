#include "nidavellir.h"
#include "fr_698.h"



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

#pragma pack(1)
struct connect_request_t
{
    uint16_t pro_ver;
    uint8_t  protocal_conformance[8];   //期望的协议一致性块 ∷=bit-string(SIZE(64))
    uint8_t  function_conformance[16];  //期望的功能一致性块 ∷=bit-string(SIZE(128))
    uint16_t send_frm_size;             //客户机发送帧最大尺寸 long-unsigned
    uint16_t recv_frm_size;             //客户机接收帧最大尺寸 long-unsigned
    uint8_t  client_win_size;           //客户机接收帧最大窗口尺寸 unsigned
    uint16_t client_apdu_size;          //客户机最大可处理帧尺寸 long-unsigned
    uint32_t connect_time;              //期望的应用连接超时时间 double-long-unsigned，
    uint8_t  connect_mode;              //认证模式enum connect_mode_e
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



struct connect_request_t cfg_connect = {
    0x0016, 
    0xFF,0xFF,0xFF,0xff,0xc0,0x00,0x00,0x00,
    0xFF,0xFE,0xC4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0200,
    0x0200,
    0x01,
    0x07d0,
    0x00001C20,
    0x00
};
    
    


static int __common_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __password_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __symmetry_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
static int __signature_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);


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

typedef int(*pFunc_security_process)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
     
const pFunc_security_process Func_security[] = 
{
    #define DEF_X(a,b)  b,
    Security_FUNC
    #undef DEF_X
};


static int __common_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 1;
}

static int __password_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    uint8_t *p_password = cache->buf_ptr;
    uint8_t password[33] = {0};
    uint32_t oad = 0;

    if (fr_698_get(instance, oad, password, sizeof(password), NULL) < 0){
        return DAR_OTHERS_ERROR;
    }

    if(0 == mem_compare(password, p_password, p_password[0])){
        return DAR_SUCCESS;
    }
    return DAR_OTHERS_ERROR;
}

static int __symmetry_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_cb;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct fifo_t* cache = ctx->apdu;

    uint8_t *p_apdu = cache->buf_ptr;
	uint8_t buf[70];
    uint8_t len_len = 0;
    uint8_t cilent_index = 0;
    uint32_t esam_session_timeout = 0xffffffffU;

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
    
    apdu_rx += 2;
    
    len_len = esam_consult_key(instance, CA_TYPE_TER(ctx->CA) , buf+1 );

    cache->wr = cache->wr - 3;
    *(p_apdu+cache->wr) |= 0x82;  //应答
    cache->wr++;
    *(p_apdu+cache->wr) = ctx->PIID;  //PIID
    cache->wr++;
    memset((p_apdu+cache->wr), 0x30, 32);//厂家信息
    cache->wr = cache->wr + 32;
    
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.pro_ver, 2);//版本号
    cache->wr = cache->wr + 2;
    apdu_rx += 2;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.protocal_conformance, 8);//期望协议一致性块
    cache->wr = cache->wr + 8;
    apdu_rx += 8;
    ndv_memcopy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.function_conformance, 16);  //期望的功能一致性块 
    cache->wr = cache->wr + 16;
    apdu_rx += 16;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.send_frm_size, 2);    //发送最大尺寸
    cache->wr = cache->wr + 2;
    apdu_rx += 2;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.recv_frm_size, 2);    //接收最大尺寸
    cache->wr = cache->wr + 2;
    apdu_rx += 2;
    *(p_apdu+cache->wr) = cfg_connect.client_win_size;//客户机接收帧最大窗口尺寸
    cache->wr = cache->wr + 1;
    apdu_rx += 1;
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&cfg_connect.client_apdu_size, 2);    //客户机最大可处理帧尺寸
    cache->wr = cache->wr + 2;
    apdu_rx += 2;
    
    mbase_cb->connect_mode = cfg_connect.connect_mode;
    
     //期望的应用连接超时时间 
    mbase_cb->connect_timeout[ctx->channel][cilent_index] = buf_to_uint32_msb(apdu_rx);
    if (   (mbase_cb->connect_timeout[ctx->channel][cilent_index] == 0) 
        || (mbase_cb->connect_timeout[ctx->channel][cilent_index] > cfg_connect.connect_time))
    {
        mbase_cb->connect_timeout[ctx->channel][cilent_index] = cfg_connect.connect_time;
    }
    
    mem_swap_cpy((p_apdu+cache->wr), (uint8_t*)&mbase_cb->connect_timeout[ctx->channel][cilent_index], 4);     //期望的应用连接超时时间    
    cache->wr = cache->wr + 4;
    apdu_rx += 4;


    if(len_len == 52)
    {
        *(p_apdu+cache->wr) = 0;//允许建立应用连接
        cache->wr++;
        *(p_apdu+cache->wr) = 1;//认证附加信息
        cache->wr++;
        *(p_apdu+cache->wr) = 48;//长度
        cache->wr++;
        memcpy((p_apdu+cache->wr), buf+1,48);//随机数
        cache->wr = cache->wr + 48;
        *(p_apdu+cache->wr) = 4;
        cache->wr++;
        memcpy((p_apdu+cache->wr), buf+49,4);//MAC
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
        
        mem_swap_cpy((uint8_t*)&mbase_cb->authen_tmr, (uint8_t*)&buf[0], 4);
        mbase_cb->authen_tmr = mbase_cb->authen_tmr * 60;

        if (esam_session_timeout == 0)
        {
            esam_session_timeout = mbase_cb->connect_timeout[ctx->channel][cilent_index];
        }

        if (mbase_cb->connect_timeout[ctx->channel][cilent_index] > esam_session_timeout)
        {
            mbase_cb->connect_timeout[ctx->channel][cilent_index] = esam_session_timeout;
        }
        
    
        mbase_cb->esam_connect_timeout[ctx->channel][cilent_index] = esam_session_timeout;
        
        mbase_cb->current_connect_channel[cilent_index] = ctx->channel;
        mbase_cb->connect_status[ctx->channel][cilent_index] = true;
        mbase_cb->connect_timeout_cnt[ctx->channel][cilent_index] = 0;

        
        if(1 == cilent_index)//建立终端应用链接时，要失效终端身份认证
        {
            mbase_cb->term_auth_timeout = 0;
        }

//        mbase_cb->clientaddr = ctx->CA;

        
        MeterWr_obj_list_clear(instance, pipeline, mesg);
        return DAR_SUCCESS;
    }
    else
    {
        *(p_apdu+cache->wr) = 1;//允许建立应用连接
        cache->wr++;
        *(p_apdu+cache->wr) = 0;//认证附加信息
        cache->wr++;

        mbase_cb->connect_status[ctx->channel][cilent_index] = false;
        mbase_cb->connect_timeout[ctx->channel][cilent_index]   = 0;
        return DAR_ESAM_CHECK_FAILED;
    }
}

static int __signature_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 1;
}

static uint32_t __get_connect_info(uint32_t val, uint32_t cfg_val)
{
    if (val > cfg_val){
        return cfg_val;
    }
    return val;
}

static void __consistency_block(uint8_t *data, const uint8_t* cfg_data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++){
        data[i] = data[i] & cfg_data[i];
    }
}

static void ___get_consult_info(struct connect_request_t *connect, const struct connect_request_t *p_connect)
{
    __consistency_block(connect->protocal_conformance, p_connect->protocal_conformance, 8);
    __consistency_block(connect->function_conformance, p_connect->function_conformance, 16);

    connect->send_frm_size      = __get_connect_info(connect->send_frm_size, p_connect->recv_frm_size);
    connect->recv_frm_size      = __get_connect_info(connect->recv_frm_size, p_connect->send_frm_size);
    connect->client_win_size    = __get_connect_info(connect->client_win_size, p_connect->client_apdu_size);
    connect->client_apdu_size   = __get_connect_info(connect->client_apdu_size, p_connect->client_apdu_size);
    connect->connect_time       = __get_connect_info(connect->connect_time, p_connect->connect_time);
}


int fr_handle_disconnect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct mbase_control_block_t* mbase_cb;
    uint8_t cilent_index = 0;
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    uint16_t offset= 0;
    
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
   
    
    if (CA_TYPE_TER(ctx->CA))//终端
    {
        cilent_index = 1;
    }
    else
    {
        cilent_index = 0;
    }
    
    
    mbase_cb->authen_tmr = 0;
    mbase_cb->irauthen_tmr = 0;
    
    mbase_cb->connect_timeout[ctx->channel][cilent_index] = 0;  
    mbase_cb->esam_connect_timeout[ctx->channel][cilent_index] = 0;
    mbase_cb->connect_status[ctx->channel][cilent_index] = false;
    
    if ((mbase_cb->connect_status[ctx->channel][0] == false)&& (mbase_cb->connect_status[ctx->channel][1] == false))
    {
        MeterWr_obj_list_clear(instance, pipeline, mesg);
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




int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    //struct fifo_t* cache = ctx->apdu;
    uint8_t *apdu_rx = ctx->apdu_d_rx;
    struct connect_request_t connect = {0};
    int result = DAR_OTHERS_ERROR;
    uint32_t temp=0;
    
    // 通配地址 拒绝服务
    if(SA_TYPE_GENERAL == SA_TYPE(ctx->SA)) {
        result = DAR_OTHERS_ERROR;
    }
    
    memcpy(&connect, apdu_rx+ctx->apdu_rx_offset, sizeof(struct connect_request_t));
    
    mem_swap_cpy((uint8_t*)&temp, (uint8_t*)&connect.send_frm_size, 2);
    if(cfg_connect.send_frm_size >= temp)//发送帧最大尺寸
    {
        cfg_connect.send_frm_size = temp;
    }
    temp=0;
    mem_swap_cpy((uint8_t*)&temp, (uint8_t*)&connect.recv_frm_size, 2);
    if(cfg_connect.recv_frm_size >= temp)//接收帧最大尺寸
    {
        cfg_connect.recv_frm_size = temp;
    }
    
    cfg_connect.connect_mode = connect.connect_mode;
    
    ctx->apdu_rx_offset += sizeof(struct connect_request_t);
    
    if (connect.connect_mode > 4){
        result = DAR_OTHERS_ERROR;
    }

    ___get_consult_info(&connect, &cfg_connect);
    //memcpy((p_apdu+cache->wr), (uint8_t*)&connect, sizeof(struct connect_request_t));  
    
    if (Security_MAX > connect.connect_mode){
        if(Func_security[connect.connect_mode]){
            result = Func_security[connect.connect_mode](instance, pipeline, mesg);
        }
    }
	return 1;

}


int get_CONNECT_Request_len(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
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
    struct message_t mesg;
    //uint8_t result = DAR_SUCCESS;
    struct mbase_control_block_t* mbase_cb;
    mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
   
    if (cilent_index > 1)//终端
    {
        return -1;
    }

    if (channel > 3)//终端
    {
        return -1;
    }

 
    mbase_cb->authen_tmr = 0;
    mbase_cb->irauthen_tmr = 0;
    
    mbase_cb->connect_timeout[channel][cilent_index] = 0;  
    mbase_cb->esam_connect_timeout[channel][cilent_index] = 0;
    mbase_cb->connect_status[channel][cilent_index] = false;
    
    if ((mbase_cb->connect_status[channel][0] == false)&& (mbase_cb->connect_status[channel][1] == false))
    {
        MeterWr_obj_list_clear(instance, pipeline, &mesg);
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
