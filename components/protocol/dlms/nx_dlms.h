#ifndef ND_APPLICATION_ADAPTERS_DLMS_NX_DLMS_H
#define ND_APPLICATION_ADAPTERS_DLMS_NX_DLMS_H

#include <stdint.h>
#include "__enums__.h"

struct fifo_t;
struct pipeline_t;

struct object_t {
    uint32_t id;
};

struct hdlc_control_block_t {
    int rx_count;
    uint16_t len_rxed;
    uint8_t c_addr_n;
    uint8_t s_addr_n;
    uint8_t step;

    uint16_t rx_len;
    uint16_t info_len;
    uint8_t ctrl;
    uint16_t hcs;
    uint8_t ca;
    uint8_t addr[16];
    uint16_t fcs;

    uint8_t dst_addr[6];
    uint16_t tx_len;

    struct fifo_t *apdu;
    struct fifo_t *apdu_ctx;
    struct object_t object;
    struct pipeline_t *pipeline;
};

struct client_t {
    uint16_t clientAddress;
    DLMS_CONNECTION_STATE connected;
};

typedef struct {
    DLMS_SECURITY security;
    DLMS_SECURITY_SUITE suite;
    uint8_t encrypt;
    uint8_t blockCipherKey[16];
    uint8_t systemTitle[8];
    uint32_t invocationCounter;
    uint8_t authenticationKey[16];
    uint8_t dedicatedKey[16];
} ciphering;

#endif
