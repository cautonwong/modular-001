#define DL645_PA_PLAIN_02    0x02
#define DL645_PA_PLAIN_04    0x04


#define DL645_PA_SC_CIPHER    0x98
#define DL645_PA_SC_PLAINMAC  0x99





//安全认证错误信息字定义 
#define     SC_ERR_OTHER             0x0001 // 其它错误 
#define     SC_ERR_USED              0x0002 // 重复充值 
#define     SC_ERR_ESAM              0x0004 // ESAM验证失败 
#define     SC_ERR_AUTHEN            0x0008 // 身份认证失败 
#define     SC_ERR_USERNUMBER        0x0010 // 客户编号不匹配 
#define     SC_ERR_BUYTIME           0x0020 // 充值次数错误 
#define     SC_ERR_TUNJI             0x0040 // 购电超囤积 
#define     SC_ERR_ADDROVER          0x0080 // 地址异常 
#define     SC_ERR_HALT              0x0100 // 挂起暂停 




extern int8_t security_decode_645(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);

