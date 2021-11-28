#ifndef __DLT698_H__
#define __DLT698_H__

#define is_scrambler_set(ctrl) ((((ctrl)>>3)&0x01) != 0)
#define DIR(ctrl)  ((((ctrl)>>7)&0x01))
#define PRM(ctrl)  ((((ctrl)>>6)&0x01))
#define is_cmd(ctrl)  ((ctrl&0x03) != 3)
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

#define SA_TYPE(SA)         ((SA&0xC0)>>6) //0-单地址 1-通配地址 2-组地址 3-广播地址
#define SA_LOGIC(SA)        ((SA&0x30)>>4)
#define SA_LOGIC_TYPE(SA)   ((SA&0x20)>>5)
#define SA_LEN(SA)          ((SA)&0x0f)

enum dl698_fr_err_t
{
    FRM_OK = 0,
    FRM_RSP_ERR = -1,
    FRM_HEADER_ERR = -1,
    FRM_LEN_ERR1 = -2,//长度太小
    FRM_LEN_ERR2 = -3,//长度超出
    FRM_CTRL_ERR = -4,//控制码错误

};


enum dlt698_command_e
{
    GET_Request             = 5,
    SET_Request             = 6,
    ACTION_Request          = 7,
};

struct context_698_t
{
    uint16_t len;
    uint16_t index;//临时计数器
    uint8_t SA;
    uint8_t LA;
    uint8_t ctrl;//接收到的控制字
    uint8_t step;
    uint8_t PIID;
    uint8_t len_unit;
    uint8_t service;
    uint8_t choice;
    uint16_t fcs;
    uint16_t hcs;
    uint16_t min_len;//发送时和接收时的报文总长度
    uint32_t err_n;
    uint8_t addr[16];
    uint8_t CA;
    int32_t fr_err;
    uint16_t segment;//分帧格式域


    //作为主站方时用的临时状态
    uint8_t dst_addr[6];//目的地址
    uint8_t seq;//当前发送的帧序号 每次作为主站方加一

    uint8_t times;//重发次数
    uint8_t seconds;//超时时间
    uint16_t tx_len;

    struct fifo_t* apdu;

};

int matcher_698(void* pctx, uint8_t* b);

#endif

