#include "nidavellir.h"

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

static const uint16_t crc16_tab[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t crc16(uint16_t init, uint8_t* buf, size_t len)
{
    uint16_t cs = init;/**<  0xffff; */

    while (len--)
    {
        cs = (cs >> 8) ^ crc16_tab[(cs ^ *buf++) & 0xff];
    }
    cs ^= 0xffff;
    return cs;
}

uint16_t update_crc16(uint16_t init, uint8_t byte)
{
    uint16_t cs = init;/**<  0xffff; */

    cs = (cs >> 8) ^ crc16_tab[(cs ^ byte) & 0xff];
    //cs ^= 0xffff;

    return cs;
}

uint16_t get_crc16(uint16_t cs)
{
    cs ^= 0xffff;
    return cs;
}

int update_hcs_fcs(struct context_698_t* ctx, uint8_t* byte)
{
    ctx->tx_len++;
    ctx->hcs = update_crc16(ctx->hcs, *byte);
    ctx->cs = update_crc16(ctx->cs, *byte);
    return 0;
}

int update_fcs(struct context_698_t* ctx, uint8_t* byte)
{
    ctx->tx_len++;
    if (is_scrambler_set(ctx->ctrl))
    {
        *byte += 0x33;
    }
    ctx->cs = update_crc16(ctx->cs, *byte);
    return 0;
}

int __pack_apdu_byte(struct pipeline_t* pipeline, uint8_t* byte)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;

    append_byte(fifo, *byte, update_fcs, ctx);
    return 0;
}

int matcher_698(struct context_698_t* ctx, uint8_t* b)
{
    uint8_t byte = *b;

    switch (ctx->step)
    {
    case 0:
        if (byte != 0x68)
        {
            ctx->err_n++;
            return -1;
        }
        ctx->step = 1;
        return 0;//没错误  继续  keep going
    case 1:
        ctx->hcs = update_crc16(0xffff, byte);
        ctx->cs = update_crc16(0xffff, byte);
        ctx->min_len = byte;
        ctx->step = 2;
        return 0;
    case 2:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->cs = update_crc16(ctx->cs, byte);
        ctx->min_len |= (byte << 8);
        ctx->min_len &= 0x3fff;
        if (ctx->min_len < 11)
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        //if ((len < 11) || (len + 2 > *bytes) || (0x16 != p[len + 1]))
        ctx->step = 3;
        return 0;
    case 3:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->cs = update_crc16(ctx->cs, byte);
        if ( (FC_CODE(byte) != 1) &&
             (FC_CODE(byte) != 3))
        {
            ctx->step = 0;
            return -1;
        }
        ctx->ctrl = byte;//控制域
        ctx->step = 4;
        return 0;
    case 4:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->cs = update_crc16(ctx->cs, byte);
        ctx->SA = byte;//SA
        ctx->addr_n = 0;
        ctx->step = 5;
        return 0;
    case 5:
        ctx->hcs = update_crc16(ctx->hcs, byte);
        ctx->cs = update_crc16(ctx->cs, byte);
        if (ctx->addr_n == SA_LEN(ctx->SA) + 1)
        {
            ctx->CA = byte;
            ctx->step = 6;
        }
        else
        {
            ctx->addr[ctx->addr_n++] = byte;
        }
        return 0;
    case 6:
        ctx->hcs = get_crc16(ctx->hcs);
        ctx->cs = update_crc16(ctx->cs, byte);
        if (byte != (ctx->hcs & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 7;
        return 0;
    case 7:
        ctx->cs = update_crc16(ctx->cs, byte);
        if (byte != ((ctx->hcs >> 8) & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->addr_n = 0;
        ctx->step = 8;
        return 0;
    case 8://用户链路数据
        if (is_scrambler_set(ctx->ctrl))
        {
            *b = byte - 0x33;
        }
        ctx->cs = update_crc16(ctx->cs, byte);
        if (ctx->addr_n == ctx->min_len - 11 - SA_LEN(ctx->SA))
        {
            ctx->step = 9;
            return 0;
        }
        ctx->addr_n++;
        return 0;
    case 9:
        ctx->cs = get_crc16(ctx->cs);
        if (byte != (ctx->cs & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 10;
        return 0;
    case 10:
        if (byte != ((ctx->cs >> 8) & 0xff))
        {
            ctx->step = 0;
            ctx->err_n++;
            return -1;
        }
        ctx->step = 11;
        return 0;
    case 11:
        if (byte != 0x16)
        {
            ctx->step = 0;
            return -1;
        }
        return 1;//接收到完整的一帧
    }

    assert(0);
    return -1;
}

int fr_serve_one_object(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;

    if (((mesg->ctrl & 0x7f) == 0x06) ||
        (mesg->ctrl & 0x7f) == 0x07)
    {
        uint8_t type = fifoBuf_getByte(fifo);
        if (type == 0x09)//octet-string
        {
            uint8_t len = fifoBuf_getByte(fifo);
            fifoBuf_getData(fifo, p + cache->wr, len);
            //todo write data
            p[cache->wr++] = 0x00;
        }
    }
    else
    {
        //if (oad->coder->size() > fifoBuf_getFree(cache))
        {
            //return -1;
        }

        //class的处理开始  找到对应class的编解码器
        uint32_t data = 0x12345678;
        will_return(__read, &data);
        will_return(__read, 4);
        //异常处理
        p[cache->wr++] = 0x01;
        p[cache->wr++] = 0x05;//type encoder
        int r = __read(mesg->id, p + cache->wr, 4);
        if (r < 0)
        {
            p[cache->wr++] = 0x00;//DAR
            p[cache->wr++] = -r;
        }
        else
        {
            cache->wr += r;
        }
    }

    return 1;
}

int fr_encode_optional(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* ofifo = (struct fifo_t*)pipeline->out_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;
    int r = 0;

    p[cache->wr++] = 0x00;//follow_report
    uint8_t time_tag = fifoBuf_getByte(fifo);
    p[cache->wr++] = time_tag;
    if (time_tag != 0)
    {
        r = fifoBuf_getData(fifo, p + cache->wr, 10);
        cache->wr += r;
    }
    
    return 1;
}

int fr_pack_698_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->cs = 0xffff;
    ctx->tx_len = 0;
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;
    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != pid_rx_698)
    {
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_tx_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint16_t len = mesg->len + SA_LEN(ctx->SA) + 10;
    ctx->hcs = 0xffff;
    ctx->cs = 0xffff;
    ctx->tx_len = 0;
    fifoBuf_putByte(fifo, 0x68);
    ctx->tx_len++;
    append_byte(fifo, (len & 0xff), update_hcs_fcs, ctx);
    append_byte(fifo, ((len >> 8) & 0xff), update_hcs_fcs, ctx);
    ctx->ctrl |= (1 << 7);//DIR=0表示由服务器发出
    if (mesg->from != pid_rx_698)
    {
        ctx->ctrl &= ~(1 << 6);//PRM=0表示由服务器发起
    }
    append_byte(fifo, ctx->ctrl, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->SA, update_hcs_fcs, ctx);
    append_data(fifo, ctx->addr, SA_LEN(ctx->SA) + 1, update_hcs_fcs, ctx);
    append_byte(fifo, ctx->CA, update_hcs_fcs, ctx);

    ctx->hcs = get_crc16(ctx->hcs);
    append_byte(fifo, (ctx->hcs & 0xff), update_fcs, ctx);
    append_byte(fifo, ((ctx->hcs >> 8) & 0xff), update_fcs, ctx);
    return 1;
}

int fr_pack_698_apdu(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    uint8_t byte;
    fifo_popData(ctx->apdu, mesg->len, __pack_apdu_byte, pipeline);
    return 1;
}

int fr_pack_698_tail(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    ctx->cs = get_crc16(ctx->cs);
    fifoBuf_putByte(fifo, ctx->cs & 0xff);
    ctx->tx_len++;
    fifoBuf_putByte(fifo, ((ctx->cs >> 8) & 0xff));
    ctx->tx_len++;
    fifoBuf_putByte(fifo, 0x16);
    ctx->tx_len++;
    return 1;
}

int fr_rx_698_end(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* ififo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->out_queue;
    struct message_t* n = (struct message_t*)instance->cache;

    fifoBuf_getByte(ififo);//fcs-0
    fifoBuf_getByte(ififo);//fcs-1
    assert(fifoBuf_getByte(ififo) == 0x16);//0x16
    fr_pack_698_head(instance, pipeline, mesg);
    fr_pack_698_apdu(instance, pipeline, mesg);
    fr_pack_698_tail(instance, pipeline, mesg);
    //使能发送
    return 0;
}

int fr_handle_get_request(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    uint8_t* p = cache->buf_ptr;

    uint8_t oad_n = mesg->oad_n;
    if (oad_n == 0) oad_n = 1;
    while (oad_n-- > 0)
    {
        //todo  apdu分帧
        //if (fifoBuf_getFree(cache) < size(oad))
        {
            fifoBuf_getData(fifo, p + cache->wr, 4);
            mesg->id = buf_to_uint32_msb(p + cache->wr);
            cache->wr += 4;
            fr_serve_one_object(instance, pipeline, mesg);
        }
    }

    fr_encode_optional(instance, pipeline, mesg);//follow-report and time-tag
    return 1;
}


int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    ctx->step = 0;
    int r = ringbuffer_filter(fifo, matcher_698, ctx);
    assert(r == ctx->min_len + 2);
    if (r > 0)
    {
#ifdef VERBOSE
        uint8_t buf[256];
        ctx->rx_count++;
        uint16_t len = fifoBuf_getDataPeek(fifo, buf, ctx->min_len + 2);
        printf("                   {%s,recv", get_pipeline_name(instance, pipeline->id));
        printf(",");
        for (size_t i = 0; i < r; i++)
        {
            printf("%02x", buf[i]);
        }
        printf(",apdu:");
        for (size_t i = 9 + SA_LEN(ctx->SA); i < len - 3; i++)
        {
            printf("%02x", buf[i]);
        }
        printf("}\n");
#endif
        return 1;
    }

    return 0;
}

int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    struct fifo_t* cache = ctx->apdu;
    //client  或者 主动上报 或者 延时读取发送
    if (recv_mesg_header(instance, pipeline, mesg))
    {
        mesg->len -= 4;
        ctx->ctrl = 0x43;
        ctx->SA = 0x05;
        ctx->addr[0] = 0x08;
        ctx->CA = 0x00;
        fifoBuf_putByte(cache, mesg->ctrl);//05
        fifoBuf_putByte(cache, mesg->qos);//01
        fifoBuf_putByte(cache, ctx->PIID); 
        fifoBuf_getData(fifo, cache->buf_ptr + cache->wr, mesg->len);
        cache->wr += mesg->len;
        mesg->len += 3;

        //区分  上报ID和上报内容
        fr_pack_698_head(instance, pipeline, mesg);
        fr_pack_698_apdu(instance, pipeline, mesg);
        fr_pack_698_tail(instance, pipeline, mesg);
#ifdef VERBOSE
        uint8_t buf[256];
        ctx->rx_count++;
        fifo = (struct fifo_t*)pipeline->out_queue;
        uint16_t len = fifoBuf_getDataPeek(fifo, buf, ctx->tx_len + 2);
        printf("                   {%s,send", get_pipeline_name(instance, pipeline->id));
        printf(",");
        for (size_t i = 0; i < len; i++)
        {
            printf("%02x", buf[i]);
        }
        printf(",apdu:");
        for (size_t i = 9 + SA_LEN(ctx->SA); i < len - 3; i++)
        {
            printf("%02x", buf[i]);
        }
        printf("}\n");
#endif
    }

    return 0;
}

int fr_gatekeeper_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;

    //todo 合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 1)
    {
        //自己是服务器  收到来自服务器的报文  丢掉
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }

    if (is_segment_set(ctx->ctrl))//分帧标志位
    {
        if (fifoBuf_getUsed(ctx->apdu) > 0)
        {
            mesg->len = fifoBuf_getUsed(ctx->apdu);
            if (mesg->len > 512)
            {
                set_segment(ctx->ctrl);
                set_middle_segment_no(ctx->segment);//中间帧
                mesg->len = 512;
            }
            else
            {
                set_segment(ctx->ctrl);
                set_last_segment_no(ctx->segment);//最后帧
            }

            fr_rx_698_end(instance, pipeline, mesg);
        }
        fifoBuf_removeData(fifo, ctx->min_len + 2);//删掉收到的确认帧
        return 0;
    }
    else
    {
        //收到的如果不是后续帧请求
        ringbuffer_clear(ctx->apdu);
    }

    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));
    uint8_t service = fifoBuf_getByte(fifo);
    uint8_t type = fifoBuf_getByte(fifo);
    
    mesg->from = pipeline->id;//从哪里来
    mesg->to = pid_tx_698;//要到哪里去
    mesg->ctrl = service|(1<<7);
    mesg->qos = type;
    mesg->id = 0x00;//暂时没用
    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD
    struct fifo_t* cache = ctx->apdu;
    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);
    if (type == 1)
    {
        mesg->oad_n = 0;
    }
    else
    {
        mesg->oad_n = fifoBuf_getByte(fifo);
        fifoBuf_putByte(cache, mesg->oad_n);
    }

    switch (service)
    {
    case 0x05:
    case 0x06:
    case 0x07:
        fr_handle_get_request(instance, pipeline, mesg);
        break;
    case 0x02://connect
    case 0x03://release
        fr_handle_get_request(instance, pipeline, mesg);
        break;
    case 0x08://上报应答 client
        break;
    case 136://上报通知 server
        break;
    case 110://异常响应client
        break;
    case 238://异常响应server
        break;
    case 16://安全请求
        break;
    }

    if (fifoBuf_getUsed(ctx->apdu)  > 0)
    {
        mesg->len = fifoBuf_getUsed(ctx->apdu);
        if (mesg->len > 512)
        {
            set_segment(ctx->ctrl);
            set_first_segment_no(ctx->segment);//第一帧
            mesg->len = 512;
        }

        fr_rx_698_end(instance, pipeline, mesg);
    }
    return 0;
}

int fr_gatekeeper_client(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;

    //todo 合法性和一致性判断
    if ((ctx->ctrl & (1 << 7)) == 0)
    {
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        return 0;
    }

    if (is_segment_set(ctx->ctrl))//分帧标志位
    {
        fifoBuf_removeData(fifo, ctx->min_len + 2);
        if (is_last_segment(ctx->segment) > 0)//收到的是最后帧
        {
            //收齐了  解析 todo
            //get-next
        }
        else
        {
            //发送分帧传输的确认帧
        }
        return 0;
    }
    else
    {
        //收到的如果不是后续帧请求
        ringbuffer_clear(ctx->apdu);
    }
    fifoBuf_removeData(fifo, ctx->min_len + 2);
    return 0;
    fifoBuf_removeData(fifo, 9 + SA_LEN(ctx->SA));
    uint8_t service = fifoBuf_getByte(fifo);
    uint8_t type = fifoBuf_getByte(fifo);

    mesg->from = pipeline->id;//从哪里来
    mesg->to = pid_tx_698;//要到哪里去
    mesg->ctrl = service | (1 << 7);
    mesg->qos = type;
    mesg->id = 0x00;//暂时没用
    ctx->PIID = fifoBuf_getByte(fifo);
    ctx->PIID |= (1 << 6);//PIID-ACD
    struct fifo_t* cache = ctx->apdu;
    fifoBuf_putByte(cache, mesg->ctrl);
    fifoBuf_putByte(cache, mesg->qos);
    fifoBuf_putByte(cache, ctx->PIID);
    if (type == 1)
    {
        mesg->oad_n = 0;
    }
    else
    {
        mesg->oad_n = fifoBuf_getByte(fifo);
        fifoBuf_putByte(cache, mesg->oad_n);
    }

    switch (service)
    {
    case 0x85:
    case 0x86:
    case 0x87:
        fr_handle_get_request(instance, pipeline, mesg);
        break;
    case 0x02://connect
    case 0x03://release
        fr_handle_get_request(instance, pipeline, mesg);
        break;
    case 0x08://上报应答 client
        break;
    case 136://上报通知 server
        break;
    case 110://异常响应client
        break;
    case 238://异常响应server
        break;
    case 144://安全响应
        break;
    }

    if (fifoBuf_getUsed(ctx->apdu) > 0)
    {
        mesg->len = fifoBuf_getUsed(ctx->apdu);
        if (mesg->len > 512)
        {
            set_segment(ctx->ctrl);
            set_first_segment_no(ctx->segment);//第一帧
            mesg->len = 512;
        }

        fr_rx_698_end(instance, pipeline, mesg);
    }
    return 0;
}

const struct filter_t fr_rx_698[] =
{
    {0x00,fr_rx_698_entry,1,13},
    {0x01,fr_gatekeeper_server,5,13},
};

struct pipeline_t pl_rx_698 =
{
    pid_rx_698,
    "pl_rx_698",
    &ctx_698,
    &stdin_698,//serial driver isr put byte into this 
    &stdout_698,//pl_rx_698 put apdu into this 
    sizeof(fr_rx_698) / sizeof(fr_rx_698[0]),
    fr_rx_698,
};

const struct filter_t fr_tx_698[] =
{
    {0x01,fr_tx_698_entry,1,13},
    //{0x01,fr_gatekeeper_698,5,13},
    //{0x0501,fr_handle_0501,0,13},
};

struct pipeline_t pl_tx_698 =
{
    pid_tx_698,
    "pl_tx_698",
    &ctx_698,
    &pipe_tx_698_req,
    &stdout_698,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};

// 1. [] 698-05
// 2. [] 698-06
// 3. [] 698-07
// 4. [] 698-安全认证
// 6. [] 698-分帧
// 7. [] 698数据模型和编码
// 8. [] 698-02
// 9. [] 698-03
// 10.[] 698-08


//内存消耗分析:  <3.5K
//串口接收FIFO 544  
//串口发送FIFO 544 
//接收别的pl的主动发送FIFO  200
//用来装apdu的cache fifo  2048
//ctx