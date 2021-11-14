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

enum connect_mode_e
{
    NullSecurity        = 0,    //公共连接NullSecurity∷=NULL
    PasswordSecurity    = 1,    //一般密码PasswordSecurity∷=visible-string
    SymmetrySecurity    = 2,    //对称加密SymmetrySecurity∷=SEQUENCE                              
                                //{
                                //  密文 1 octet-string，
                                //  客户机签名 1 octet-string
                                //};
    SignatureSecurity   = 3,    //数字签名    SignatureSecurity∷=SEQUENCE
                                //{ 
                                //      密文 2 octet-string，
                                //      客户机签名 2 octet-string
                                //}
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
#if 0
const uint8_t CODE esam_series[6]         = {0x80, 0x36, 0x00, 0x02, 0x00, 0x00};	//序列号
const uint8_t CODE esam_version[6]        = {0x80, 0x36, 0x00, 0x01, 0x00, 0x00};	//版本号
const uint8_t CODE esam_key_ver[6]        = {0x80, 0x36, 0x00, 0x04, 0x00, 0x00};	//对称密钥版本
const uint8_t CODE esam_time[6]           = {0x80, 0x36, 0x00, 0x05, 0x00, 0x00};	//会话时效门限,会话时效剩余时间
const uint8_t CODE esam_count[6]          = {0x80, 0x36, 0x00, 0x06, 0x00, 0x00};	//计数器
const uint8_t CODE esam_symauth[6]        = {0x81, 0x02, 0x00, 0x00, 0x00, 0x24};	//会话协商
const uint8_t CODE esam_symauth_term[6]   = {0x82, 0x02, 0x00, 0x00, 0x00, 0x24};	//会话协商-终端

const uint8_t CODE esam_random8[6]        = {0x80, 0x04, 0x00, 0x08, 0x00, 0x00};	//取ESAM 8 字节随机数
const uint8_t CODE encrypt_random8[6]     = {0x80, 0x08, 0x08, 0x01, 0x00, 0x10};	//ESAM加密 8 字节随机数
const uint8_t CODE software_encrypt[6]    = {0x80, 0x0A, 0x48, 0x00, 0x00, 0x58};	//ESAM软件比对加密数据
const uint8_t CODE esam_dataget[6]        = {0x80, 0x2c, 0x00, 0x00, 0x00, 0x05};
const uint8_t CODE esam_wallet_sub[6]     = {0x80, 0x46, 0x00, 0x00, 0x00, 0x04};//ESAM 钱包扣减
const uint8_t CODE esam_wallet_set[6]     = {0x83, 0x3e, 0x00, 0x00, 0x00, 0x08};//ESAM 钱包文件预制
const uint8_t CODE esam_ir_auth[6]        = {0x80, 0x06, 0x00, 0x01, 0x00, 0x08};//ESAM 红外认证 
const uint8_t CODE esam_term_auth_request[6]    = {0x80, 0x08, 0x00, 0x04, 0x00, 0x08};//ESAM 红外认证请求

const uint8_t CODE esam_wallet_credit[5]  = {0x80, 0x42, 0x00, 0x00, 0x0C };//ESAM 钱包文件充值
const uint8_t CODE esam_wallet_red[7]     = {0x80, 0x48, 0x00, 0x00, 0x00, 0x01,0x00};//ESAM 钱包文件读
const uint8_t CODE esam_file_write[4]     = {0x80, 0x2a, 0x00, 0x07};  //ESAM明文写返写信息文件
const uint8_t CODE encrypt_1[4]           = {0x81, 0x1c, 0x00, 0x11};
const uint8_t CODE encrypt_2[4]           = {0x81, 0x1c, 0x00, 0xa6};
const uint8_t CODE encrypt_3[4]           = {0x81, 0x1c, 0x00, 0xa7};
const uint8_t CODE encrypt_4[4]           = {0x80, 0x0e, 0x40, 0x02};
const uint8_t CODE encrypt_5[4]           = {0x80, 0x0a, 0x40, 0x02};
const uint8_t CODE encrypt_6[4]           = {0x80, 0x10, 0x40, 0x02};
const uint8_t CODE encrypt_term_1[8]           = {0x82, 0x24, 0x00, 0x11,0x80, 0x24, 0x11, 0x00};
const uint8_t CODE encrypt_term_2[8]           = {0x82, 0x24, 0x00, 0x12};
const uint8_t CODE encrypt_term_3[8]           = {0x82, 0x24, 0x00, 0x13,0x80, 0x24, 0x13, 0x00};

const uint8_t CODE esam_report_request[4]         = {0x80, 0x14, 0x01, 0x03};//上报
const uint8_t CODE esam_report_response[4]        = {0x80, 0x0e, 0x40, 0x81};//上报确认

#endif
//int fr_698_get(struct instance_t* instance, uint32_t view_id, uint8_t* buf, uint16_t buf_size)
//int fr_698_set(struct instance_t* instance, uint32_t view_id, uint8_t* buf, uint16_t buf_size)


uint8_t __password_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    uint8_t *p_password = cache->buf_ptr;
    //uint8_t *p_password = cache + sizeof(struct connect_request_t);

    uint8_t password[33] = {0};
    uint32_t oad = 0;

    if (fr_698_get(instance, oad, password, sizeof(password)) < 0)
    {
        return DAR_OTHERS_ERROR;
    }

    if(0 == memcmp(password, p_password, p_password[0]))
    {
        return DAR_SUCCESS;
    }
    return DAR_OTHERS_ERROR;
}

uint8_t __symmetry_security_process(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* cache = ctx->apdu;
    //uint8_t *p_apdu = cache + sizeof(struct connect_request_t);
    uint8_t *p_apdu = cache->buf_ptr;
    uint8_t *p_password = cache->buf_ptr;
	uint8_t  buf[70];
    uint8_t len_len = 0;
    uint16_t data_len = 0;
    uint16_t offset = 0;

    if(CA_TYPE_TER(ctx->CA))//from 终端
    {
	    //memcpy((uint8_t*)&buf[offset],(uint8_t*)&esam_symauth_term[0],6);
		//dectypt->rn[0] = 64;
    }
    else //from 主站
    {
	   // memcpy((uint8_t*)&buf[offset],(uint8_t*)&esam_symauth[0],6);
		//dectypt->rn[0] = 48;
    }
    offset += 6;

    //  密文 1 octet-string，
    len_len = axdr_data_len_decode(TYPE_octet_string, p_apdu, &data_len);
    //memcpy((uint8_t*)&buf[offset], p_apdu + len_len, data_len);
    offset += data_len;
    p_apdu += len_len + data_len;

    //  客户机签名 1 octet-string
    len_len = axdr_data_len_decode(TYPE_octet_string, p_apdu, &data_len);
	//memcpy((uint8_t*)&buf[offset], p_apdu + len_len, data_len);
    offset += data_len;
    p_apdu += len_len + data_len;


    uint32_t oad = 0;
    offset = 20;
    if (fr_698_action(instance, oad, buf, offset) < 0)
    {
        return DAR_OTHERS_ERROR;
    }

    if ( false != fr_698_action(instance, oad, buf, offset))
	{
		//memcpy(&dectypt->rn[1],&buf[0],dectypt->rn[0]);
        
		//dectypt->mac[0] = 4;
		//memcpy(&dectypt->mac[1],&buf[dectypt->rn[0]],4);
        
		return DAR_SUCCESS;
	}
    
    return DAR_ESAM_CHECK_FAILED;
}


uint32_t __get_connect_info(uint32_t val, uint32_t cfg_val)
{
    //if ((connect.client_win_size < session->send_buffer_size) || (connect.client_win_size > para->client_process_apdu_size))
    {
    //    connect.client_win_size = para->client_process_apdu_size;
    }
    return 1;
}

static int __consistency_block(uint8_t *data, uint8_t* cfg_data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)//商定的协议一致性块
    {
        //temp = dl698_byte_swap(*p++);
        //session->protocal_conformance[i] = temp & para->protocal_conformance[i];
        data[i] = data[i] & cfg_data[i];
    }
    return 1;
}

int fr_handle_connect(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    struct connect_request_t connect = {0};
    struct connect_cfg_info_t *p_connect = ctx->connect_info;
    struct connect_cfg_info_t cfg = {
        0
    };
    struct connect_cfg_info_t* pcfg_connect = &cfg;
    uint8_t result = DAR_SUCCESS;

    if(SA_TYPE_GENERAL == SA_TYPE(ctx->SA)) // 通配地址 拒绝服务
    {
        result = DAR_OTHERS_ERROR;
        goto err;
    }

    fifoBuf_getData(fifo, p, sizeof(struct connect_request_t));

    memcpy(&connect, p, sizeof(struct connect_request_t));

    if (connect.connect_mode > 4)
    {
        result = DAR_OTHERS_ERROR;
        goto err;
    }

    //if(connect.pro_ver != p_connect->version)//期望的应用层协议版本号 不做判断

    //商定的协议一致性块
    __consistency_block(connect.protocal_conformance, pcfg_connect->protocal_conformance, 8);
    //商定的功能一致性块
    __consistency_block(connect.function_conformance, pcfg_connect->function_conformance, 16);

    //客户机发送帧最大尺寸
    connect.send_frm_size = __get_connect_info(connect.send_frm_size, pcfg_connect->link_recv_buf_size);
    // 客户机接收帧最大尺寸
    connect.recv_frm_size = __get_connect_info(connect.recv_frm_size, pcfg_connect->link_send_buf_size);
    // 客户机接收帧最大窗口尺寸
    connect.client_win_size = __get_connect_info(connect.client_win_size, pcfg_connect->client_process_apdu_size);
    // 客户机最大可处理帧尺寸
    connect.client_apdu_size = __get_connect_info(connect.client_apdu_size, pcfg_connect->client_process_apdu_size);
    /* 期望的应用连接超时时间 */
    connect.connect_time = __get_connect_info(connect.connect_time, pcfg_connect->connect_timeout);

    if(0 == connect.connect_mode) //公共连接[0] NullSecurity
    {
    }

    if(1 == connect.connect_mode) //一般密码[1] PasswordSecurity
    {
        if (DAR_SUCCESS != __password_process(instance, pipeline, mesg))
        {
            result = CONNECT_RESULT_PASSWORD_ERR;
        }
    }

    if(2 == connect.connect_mode) //对称加密[2] SymmetrySecurity
    {
        if (DAR_SUCCESS != __symmetry_security_process(instance, pipeline, mesg))
        {
            result = CONNECT_RESULT_PASSWORD_ERR;
        }
    }  

    if(3 == connect.connect_mode) //数字签名[3] SignatureSecurity ESAM不支持
    {
        result = DAR_OTHERS_ERROR;
        goto err;
    }  

err:

    //uint8_t oad_n = mesg->oad_n;
    //if (oad_n == 0) oad_n = 1;
    //while (oad_n-- > 0)
    {
        //todo  apdu分帧
        //if (fifoBuf_getFree(cache) < size(oad))
        {
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;
            //fr_serve_one_object(instance, pipeline, mesg);
        }
    }

    //fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}

