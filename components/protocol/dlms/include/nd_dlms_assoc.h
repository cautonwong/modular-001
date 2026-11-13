#ifndef ND_DLMS_ASSOC_H
#define ND_DLMS_ASSOC_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_dlms_security.h"

typedef enum {
    ND_DLMS_ASSOC_IDLE = 0,
    ND_DLMS_ASSOC_AARQ,
    ND_DLMS_ASSOC_HLS_CHALLENGE,
    ND_DLMS_ASSOC_ASSOCIATED,
    ND_DLMS_ASSOC_RELEASED
} nd_dlms_assoc_state_t;

typedef enum {
    ND_DLMS_AUTH_NONE = 0,
    ND_DLMS_AUTH_LLS,
    ND_DLMS_AUTH_HLS_GMAC,
    ND_DLMS_AUTH_HLS_SHA256,
    ND_DLMS_AUTH_HLS_ECDSA
} nd_dlms_auth_t;

typedef struct {
    uint8_t client_sap;
    uint8_t auth;
    uint8_t security_policy;
    uint8_t security_suite;
    uint16_t max_rx_pdu;
    uint16_t max_tx_pdu;
    uint8_t client_system_title[8];
    uint8_t auth_value[8];
    uint8_t auth_value_len;
} nd_dlms_aarq_t;

typedef struct {
    nd_dlms_assoc_state_t state;
    uint8_t client_sap;
    uint8_t auth;
    uint8_t security_policy;
    uint8_t security_suite;
    uint16_t max_rx_pdu;
    uint16_t max_tx_pdu;
    uint32_t invoke_id;
    nd_dlms_security_ctx_t *security;
} nd_dlms_assoc_t;

int nd_dlms_assoc_init(nd_dlms_assoc_t *assoc,
                       nd_dlms_security_ctx_t *security);
int nd_dlms_assoc_begin(nd_dlms_assoc_t *assoc, const nd_dlms_aarq_t *aarq);
int nd_dlms_assoc_hls_verify(nd_dlms_assoc_t *assoc, int verified);
int nd_dlms_assoc_release(nd_dlms_assoc_t *assoc);
int nd_dlms_assoc_can_access(const nd_dlms_assoc_t *assoc, uint8_t client_sap);

#endif
