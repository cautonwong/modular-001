#ifndef ND_APPLICATION_ADAPTERS_DL645_CONTEXT_H
#define ND_APPLICATION_ADAPTERS_DL645_CONTEXT_H

#include <stdint.h>
#include <time.h>


#include "fr_645_message.h"

struct pipeline_t;

typedef struct fr_645_fifo {
    uint8_t *buf_ptr;
    volatile uint16_t rd;
    volatile uint16_t wr;
    uint16_t buf_size;
} fr_645_fifo_t;

#define max_apdu_cache_645  1200 
struct context_645_t
{
    uint8_t channel;
    //作为从站方用的临时状态
    uint16_t addr_n;
    uint8_t addr[6];
    uint8_t SA; //表地址信息
    uint8_t ctrl;//接收到的控制字
    uint8_t step;
    uint8_t cs;
    uint8_t ver;
    uint16_t len;//to-be-deleted
    uint16_t min_len;//发送时和接收时的报文总长度
    uint32_t err_n;
    int32_t fr_err;
    //作为主站方时用的临时状态
    uint8_t dst_addr[6];//目的地址
    uint8_t seq;//当前发送的帧序号 每次作为主站方加一
    clock_t waiting_rx;//tx的时标 tx==0 表示空闲
    uint8_t times;//重发次数
    uint8_t seconds;//超时时间
    uint16_t tx_len;
    
    fr_645_fifo_t apdu;
    
    uint8_t apdu_tx[max_apdu_cache_645]; 
    
    uint8_t apdu_d_rx[544]; 
    uint16_t apdu_rx_len;
    uint16_t apdu_rx_offset;

    uint8_t segment_tx; 
    uint8_t segment_tx_no;
    uint8_t report_stat_follow_flag; //主动上报状态字后续帧标志
    uint8_t first_segment_MaxLen;       //首帧长度限制 
    uint8_t follow_segment_MaxLen;      //除首帧外的后续帧长度限制 
    uint8_t password[4];

    //接收和发送都要用的状态
    struct pipeline_t *pipeline;//to-be-deleted
    nd_dl645_send_fn forward;
};

extern struct context_645_t ctx_msta_rx_from_meter;//rx ctx
extern struct context_645_t ctx_msta_tx_to_meter;
extern struct context_645_t ctx_msta_rx_from_bt;//rx ctx
extern struct context_645_t ctx_msta_tx_to_bt;
extern struct context_645_t ctx_msta_rx_from_sta;//rx ctx
extern struct context_645_t ctx_msta_tx_to_sta;
extern struct context_645_t ctx_msta_tx_to_bt;
extern struct context_645_t ctx_meter_rx_from_msta;//rx ctx
extern struct context_645_t ctx_meter_tx_to_msta;
extern struct context_645_t ctx_sta_rx_from_msta;//rx ctx
extern struct context_645_t ctx_sta_tx_to_msta;
extern struct context_645_t ctx_bt_rx_from_msta;//rx ctx
extern struct context_645_t ctx_bt_tx_to_msta;
extern struct context_645_t ctx_msta_proxy_to_bt;
extern struct context_645_t ctx_proxy;


int fr_rx_645_entry(struct instance_t *instance, struct pipeline_t *pipeline,
                    nd_dl645_msg_t *mesg);
int fr_gatekeeper_645_server(struct instance_t *instance,
                             struct pipeline_t *pipeline,
                             nd_dl645_msg_t *mesg);

#endif
