#ifndef ND_APPLICATION_ADAPTERS_DL698_CONTEXT_H
#define ND_APPLICATION_ADAPTERS_DL698_CONTEXT_H

#include <stdint.h>
#include <time.h>

#include "fr_698_message.h"
#include "components/core/buf/nd_buf.h"

struct connect_cfg_info_t;
struct factory_version_t;
struct fifo_t;
struct pipeline_t;

#define Max698len       544
#define Max698_apdu     2000
#define Max698_apdu_rx  2048

#define FR_698_PID_RX 14
#define FR_698_PID_TX 15
#define FR_698_PID_PLC_RX 24
#define FR_698_PID_PLC_TX 25
#define FR_698_PID_IR_RX 26
#define FR_698_PID_4852_RX 28

#define FR_698_CHL_IR 0
#define FR_698_CHL_RS485_1 1
#define FR_698_CHL_PLC 2
#define FR_698_CHL_RS485_2 3
#define FR_698_CHL_NUM 4


#define pl_tx_698_len   128

#define cache_max   2048

#define Reserved_EsamENCode_Len 64 //预留esam加密 数据变长问题

struct context_698_t
{
    int magic_num;
    int rx_count;//暂时用下to-be-deleted
    //配置信息
    struct connect_cfg_info_t *connect_info;
    struct factory_version_t *factory_version;
    //作为从站方用的临时状态
    
    uint8_t channel;
    uint16_t index;//临时计数器
    uint8_t SA; //0-单地址 1-通配地址 2-组地址 3-广播地址
    uint8_t LA;
    uint8_t ctrl;//接收到的控制字
    uint8_t step;
    uint8_t PIID;
    uint8_t len_unit;
    uint8_t service;
    uint8_t choice;
    uint8_t resp_choice;
    
    uint16_t fcs;
    uint16_t hcs;
    uint16_t min_len;//发送时和接收时的报文总长度
    uint8_t err_type;
    uint8_t CA;
      
    uint32_t err_n;
    uint8_t addr[16];
  
    int32_t fr_err;
    uint16_t segment_rx; 
    uint16_t segment;
    uint16_t last_segment;
    uint16_t segment_no;
      
    uint8_t next_index; 
    uint8_t next_last_frm;
    uint8_t next_index1_frm;
    uint8_t next_index2_frm;
    uint8_t next_choice; 
    uint8_t next_que; 
    
    
    uint8_t timeTag_valid;
    uint8_t timeTag[11]; //date_time_s + TI

    uint8_t security_choice;
    uint8_t verification_choice;
    uint16_t verification_len;
    uint8_t RN_len;
    uint8_t RN_data[64];
    uint8_t *security_data;
    uint16_t security_data_len;
    
    //作为主站方时用的临时状态
    uint8_t dst_addr[6];//目的地址
    uint8_t seq;//当前发送的帧序号 每次作为主站方加一
    clock_t waiting_rx;//tx的时标 tx==0 表示空闲
    uint8_t times;//重发次数
    uint8_t seconds;//超时时间
    uint16_t tx_len;

    uint16_t segment_tx; 
    uint16_t segment_tx_no;
    


    uint16_t apdu_len_valid;
    uint16_t apdu_offset;
    
    uint8_t apdu_d_rx[Max698_apdu_rx]; 
    uint16_t apdu_rx_len;
    uint16_t apdu_l_len;
    uint16_t apdu_rx_offset;
    uint16_t apdu_len_after_decode;
    
    struct fifo_t* apdu;

   
    //接收和发送都要用的状态
    struct pipeline_t* pipeline;//to-be-deleted
    nd_dl698_send_fn send;
    nd_dl698_send_fn forward;

    nd_block_t rx_block;
    nd_frame_t rx_frames[6];
    uint8_t rx_frame_count;
};

extern struct context_698_t ctx_698;
extern struct context_698_t client_ctx_698;

#endif
