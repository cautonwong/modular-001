#ifndef __NX_DLMS_H__
#define __NX_DLMS_H__
#include <stdint.h>
#include "kernel/object.h"

#ifdef NX_DLMS_USER_DEFINE_FILE
#include "nx_dlms_user.h"
#endif
/*
## 资源消耗情况
  - 内存消耗分析:  <3.5K
    - 串口接收FIFO 544
    - 串口发送FIFO 544
    - 接收别的pl的主动发送FIFO  200
    - 用来装apdu的cache fifo  2048
    - ctx
*/
struct hdlc_control_block_t
{
    int rx_count;//暂时用下to-be-deleted
    uint16_t len_rxed;//已解析的长度
    uint8_t c_addr_n;
    uint8_t s_addr_n;
    uint8_t step;

    uint16_t rx_len;//接收时的报文长度域 不含7e
    uint16_t info_len;//信息域的长度
    uint8_t  ctrl;//接收到的控制字
    uint16_t hcs;//链路层的头部校验
    uint8_t  ca;//client address
    uint8_t  addr[16];
    uint16_t fcs;

    //作为主站方时用的临时状态
    uint8_t dst_addr[6];//目的地址
    uint16_t tx_len;

    struct fifo_t* apdu;
    struct fifo_t* apdu_ctx;
    //物理层参数
    //链路层参数
    //设备参数

    struct object_t object;//对象标识
    struct pipeline_t* pipeline;//to-be-deleted
};


struct client_t
{
    uint16_t clientAddress;
    DLMS_CONNECTION_STATE connected;
};

typedef struct
{
    /**
    * Used security.
    */
    DLMS_SECURITY security;
    DLMS_SECURITY_SUITE suite;

    /**
    * Is data encrypted.
    */
    uint8_t encrypt;

    /**
    * Block cipher key.
    */
    uint8_t blockCipherKey[16];

    /**
    * System title.
    */
    uint8_t systemTitle[8];

    /**
    * Invocation (Frame) counter.
    */
    uint32_t invocationCounter;
    uint8_t authenticationKey[16];

    //Dedicated key.
    uint8_t dedicatedKey[16];

} ciphering;


#endif