#include "components/protocol/dlms/include/nd_dlms_security.h"

#include <stdint.h>

int nd_dlms_security_accept_fc(nd_dlms_security_ctx_t *ctx, uint32_t frame_counter,
                               int broadcast)
{
    if (ctx == NULL) {
        return -1;
    }

    uint32_t *last = broadcast ? &ctx->fc_rx_broadcast : &ctx->fc_rx_unicast;
    uint8_t *valid = broadcast ? &ctx->fc_rx_broadcast_valid
                               : &ctx->fc_rx_unicast_valid;

    if (*valid == 0) {
        *last = frame_counter;
        *valid = 1;
        return 0;
    }
    if (frame_counter <= *last) {
        return -1;
    }
    *last = frame_counter;
    return 0;
}

uint32_t nd_dlms_security_next_tx(nd_dlms_security_ctx_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    uint32_t current = ctx->fc_tx;
    if (ctx->fc_tx != UINT32_MAX) {
        ctx->fc_tx++;
    }
    return current;
}
