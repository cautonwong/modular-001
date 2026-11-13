#include "components/protocol/dlms/include/nd_dlms_assoc.h"

#include <string.h>

static int valid_aarq(const nd_dlms_aarq_t *aarq)
{
    return aarq->auth <= ND_DLMS_AUTH_HLS_ECDSA
        && aarq->security_policy <= 3
        && aarq->security_suite <= 1
        && aarq->max_rx_pdu > 0
        && aarq->max_tx_pdu > 0;
}

int nd_dlms_assoc_init(nd_dlms_assoc_t *assoc,
                       nd_dlms_security_ctx_t *security)
{
    if (assoc == NULL) {
        return -1;
    }
    memset(assoc, 0, sizeof(*assoc));
    assoc->state = ND_DLMS_ASSOC_IDLE;
    assoc->security = security;
    return 0;
}

int nd_dlms_assoc_begin(nd_dlms_assoc_t *assoc, const nd_dlms_aarq_t *aarq)
{
    if (assoc == NULL || aarq == NULL || !valid_aarq(aarq)
        || (assoc->state != ND_DLMS_ASSOC_IDLE
            && assoc->state != ND_DLMS_ASSOC_RELEASED)) {
        return -1;
    }

    assoc->state = ND_DLMS_ASSOC_AARQ;
    assoc->client_sap = aarq->client_sap;
    assoc->auth = aarq->auth;
    assoc->security_policy = aarq->security_policy;
    assoc->security_suite = aarq->security_suite;
    assoc->max_rx_pdu = aarq->max_rx_pdu;
    assoc->max_tx_pdu = aarq->max_tx_pdu;

    if (aarq->auth == ND_DLMS_AUTH_HLS_GMAC
        || aarq->auth == ND_DLMS_AUTH_HLS_SHA256
        || aarq->auth == ND_DLMS_AUTH_HLS_ECDSA) {
        assoc->state = ND_DLMS_ASSOC_HLS_CHALLENGE;
    } else {
        assoc->state = ND_DLMS_ASSOC_ASSOCIATED;
    }
    return 0;
}

int nd_dlms_assoc_hls_verify(nd_dlms_assoc_t *assoc, int verified)
{
    if (assoc == NULL || assoc->state != ND_DLMS_ASSOC_HLS_CHALLENGE) {
        return -1;
    }
    if (!verified) {
        assoc->state = ND_DLMS_ASSOC_IDLE;
        return -1;
    }
    assoc->state = ND_DLMS_ASSOC_ASSOCIATED;
    return 0;
}

int nd_dlms_assoc_release(nd_dlms_assoc_t *assoc)
{
    if (assoc == NULL) {
        return -1;
    }
    assoc->state = ND_DLMS_ASSOC_RELEASED;
    return 0;
}

int nd_dlms_assoc_can_access(const nd_dlms_assoc_t *assoc, uint8_t client_sap)
{
    return assoc != NULL && assoc->state == ND_DLMS_ASSOC_ASSOCIATED
        && assoc->client_sap == client_sap;
}
