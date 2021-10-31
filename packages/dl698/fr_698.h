#ifndef __FR_698_H__
#define __FR_698_H__

#include "fr_axdr.h"
#include "fr_698_comm.h"

#define is_scrambler_set(ctrl) ((((ctrl)>>3)&0x01) != 0)
#define DIR(ctrl)  ((((ctrl)>>7)&0x01))
#define PRM(ctrl)  ((((ctrl)>>6)&0x01))
#define is_segment_set(ctrl)  ((((ctrl)>>5)&0x01) != 0)
#define set_segment(ctrl)  (ctrl |= (1<<5))
#define segment_no(s)  (s & 0xfff)
#define set_segment_no(s)  (s=(s & 0xfff)+1)
#define is_confirm_segment(s)  ((((s)>>14)&0x03) == 2)
#define is_first_segment(s)  ((((s)>>14)&0x03) == 0)
#define is_middle_segment(s)  ((((s)>>14)&0x03) == 3)
#define is_last_segment(s)  ((((s)>>14)&0x03) == 1)
#define set_first_segment_no(s)  do{set_segment_no(s);(s |= (0<<14));}while(0)
#define set_middle_segment_no(s)  do{set_segment_no(s);(s |= (3<<14));}while(0)
#define set_last_segment_no(s)  do{set_segment_no(s);(s |= (1<<14));}while(0)
#define set_confirm_segment(s)  do{(s |= (2<<14));}while(0)
#define FC_CODE(ctrl)  ((ctrl)&0x07)

#define CA_TYPE_TER(ca)     ((ca&0xF0) == 0)

#define get_OAD_Class(oad)      (uint8_t)((oad&0xF0000000)>>28)
#define get_OAD_Attribute(oad)  (uint8_t)((oad&0x00000F00)>>8)


/** data-assess-result */
enum dl698_dar_t
{
    DAR_SUCCESS = 0,                        /**< 成功 （ 0）， */
    DAR_HARDWARE_INVALID = 1,               /**< 硬件失效 （ 1）， */
    DAR_TEMP_INVALID = 2,                   /**< 暂时失效 （ 2）， */
    DAR_RW_REJECT = 3,                      /**< 拒绝读写 （ 3）， */
    DAR_OBJECT_UNDEF = 4,                   /**< 对象未定义 （ 4）， */
    DAR_IC_ERROR = 5,                       /**< 对象接口类不符合 （ 5）， */
    DAR_OBJECT_NOT_EXIST = 6,               /**< 对象不存在 （ 6）， */
    DAR_TYPE_NOT_MATCH = 7,                 /**< 类型不匹配 （ 7）， */
    DAR_BUFFER_OVERFLOW = 8,                /**< 越界 （ 8）， */
    DAR_BLOCK_DATA_UNUSED = 9,              /**< 数据块不可用 （ 9）， */
    DAR_BLOCK_TRANSFER_CANCEL = 10,         /**< 分帧传输已取消 （ 10）， */
    DAR_NOT_IN_BLOCK_TRANSFER_STATE = 11,   /**< 不处于分帧传输状态 （ 11）， */
    DAR_BLOCK_WRITE_CANCEL = 12,            /**< 块写取消 （ 12）， */
    DAR_BLOCK_WRITE_STATE_NOT_EXIST = 13,   /**< 不存在块写状态 （ 13）， */
    DAR_BLOCK_NUM_INVALID = 14,             /**< 数据块序号无效 （ 14）， */
    DAR_UNAUTHORIZED = 15,                  /**< 密码错/未授权 （ 15）， */
    DAR_COMM_SPEED_CANNOT_CHANGE = 16,      /**< 通信速率不能更改 （ 16）， */
    DAR_TIME_ZONE_EXCEED = 17,              /**< 年时区数超 （ 17）， */
    DAR_DAY_INTERVAL_EXCEED = 18,           /**< 日时段数超 （ 18）， */
    DAR_TARIFF_NUM_EXCEED = 19,             /**< 费率数超 （ 19）， */
    DAR_SECURITY_AUTH_NOT_MATCH = 20,       /**< 安全认证不匹配       （20）， */
    DAR_REPEAT_TOPUP = 21,                  /**< 重复充值 */
    DAR_ESAM_CHECK_FAILED = 22,             /**< ESAM验证失败        （22）, */
    DAR_SECURITY_AUTH_FAILED = 23,          /**< 安全认证失败        （23）， */
    DAR_CUSTOMER_NUM_NOT_MATCH = 24,        /**< 客户编号不匹配      （24）， */
    DAR_TOPUP_TIMES_ERROR = 25,             /**< 充值次数错误        （25）， */
    DAR_TOPUP_EXCEED_MAX_VALUE = 26,        /**< 购电超囤积 */
    DAR_ADDRESS_ABNORMAL = 27,              /**< 地址异常 */
    DAR_SYMMETRIC_DESCRYPT_ERROR = 28,      /**< 对称解密错误        （28）， */
    DAR_ASYMMETRIC_DESCRYPT_ERROR = 29,     /**< 非对称解密错误      （29）， */
    DAR_SIGNATURE_ERROR = 30,               /**< 签名错误            （30）， */
    DAR_METER_SUSPEND = 31,                 /**< 电能表挂起 */
    DAR_TIME_TAG_INVALID = 32,              /**< 时间标签无效         (32)， */
    DAR_REQUEST_TIMEOUT = 33,               /**< 请求超时             （33），*/
    DAR_ESAM_P1P2_ERROR = 34,               /**< ESAM的P1P2不正确    （34），*/
    DAR_ESAM_LC_ERROR = 35,                 /**< ESAM的LC错误        （35），*/

    /*自扩*/
    DAR_EX_TAG_ERROR = 251,                 /**< tag错误（252），*/
    DAR_EX_OI_INVALID = 252,                /**< OI无效   （252），*/
    DAR_EX_STORAGE_DEPTH_ERROR = 253,       /**< 分钟冻结存储空间不足（253），*/
    DAR_EX_READ_WRITE_ERROR = 254,          /**< fs读写函数错        （254），*/
    /*自扩结束*/

    DAR_OTHERS_ERROR = 255,                 /**< 其它 （ 255） */
};




enum dlt698_command_e
{
    //LINK-APDU
    LINK_Request            = 1,
    LINK_Response           = 129,
    //Client-APDU
    CONNECT_Request         = 2,
    RELEASE_Request         = 3,
    GET_Request             = 5,
    SET_Request             = 6,
    ACTION_Request          = 7,
    REPORT_Response         = 8,
    PROXY_Request           = 9,
    COMPACT_GET_Request     = 69,
    COMPACT_SET_Request     = 70,
    COMPACT_PROXY_Request   = 73,
    ERROR_C_Response        = 110,
    //Server-APDU
    CONNECT_Response        = 130,
    RELEASE_Response        = 131,
    RELEASE_Notification    = 132,
    GET_Response            = 133,
    SET_Response            = 134,
    ACTION_Response         = 135,
    REPORT_Notification     = 136,
    PROXY_Response          = 137,
    COMPACT_GET_Response    = 197,
    COMPACT_SET_Response    = 198,
    COMPACT_PROXY_Response  = 201,
    ERROR_S_Response        = 238,
    //SECURITY-APDU
    SECURITY_Request        = 16,
    SECURITY_Response       = 144,
};

enum sa_type_e
{
    SA_TYPE_SINGLE      = 0,    //0-单地址 
    SA_TYPE_GENERAL     = 1,    //1-通配地址 
    SA_TYPE_GROUP       = 2,    //2-组地址 
    SA_TYPE_BROADECAST  = 3,    //3-广播地址
};

#define WILDCARD_NULL               0x00
#define WILDCARD_YEAR               0x01
#define WILDCARD_MONTH              0x02
#define WILDCARD_DAY                0x04
#define WILDCARD_WEEK               0x08
#define WILDCARD_HOUR               0x10
#define WILDCARD_MINUTE             0x20
#define WILDCARD_SECOND             0x40
#define WILDCARD_MILLISECOND        0x80
#define WILDCARD_ALL                0xFF

#pragma pack(1)
struct oad_t
{
    union
    {
        //00H表示整个属性全部内容。
        //如果属性是一个结构或数组，01H指向对象属性的第一个元素；
        //如果属性是一个记录型的存储区，非零值n表示最近第n次的记录。
        uint8_t  index;
        uint8_t  no : 5; //属性编号  取值0…31，其中0表示整个对象属性，即对象的所有属性；
        uint8_t  attr : 3;//属性特征
        uint16_t oi;
    }s;

    uint32_t id;
    struct coder_t* coder;
};


extern int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern uint16_t axdr_get_data_len(const uint8_t *data);

#endif //__FR_698_H__