#include "application/adapters/dl698/fr_698_esam_input.h"

#include <string.h>

#include "fr_698.h"
#include "fr_698_esam.h"
#include "fr_698_env.h"

void fr_698_esam_input_init(struct context_698_t* ctx,
                            struct esam_698_input_t* input, uint8_t cmd_type)
{
    input->cmd_type = cmd_type;
    input->security_choice = ctx->security_choice;
    input->verification_choice = ctx->verification_choice;
    input->data = ctx->security_data;
    input->data_len = ctx->security_data_len;
}

void fr_698_esam_input_buffers(struct instance_t* instance,
                               struct esam_698_input_t* input)
{
    input->data_buf = nd_fr698_apdu_cache(instance);
    input->data_buf_len = cache_max;
}

int fr_698_esam_input_sid(struct context_698_t* ctx,
                          struct esam_698_input_t* input)
{
    uint8_t* apdu_rx = ctx->apdu_d_rx;
    uint16_t offset = 0;
    uint16_t len = 0;

    input->sid_id = buf_to_uint32_msb(apdu_rx + ctx->apdu_rx_offset);
    ctx->apdu_rx_offset += 4;
    input->AttachData_len = get_octet_string(ctx, &offset, &len);
    input->AttachData = apdu_rx + offset;
    ctx->apdu_len_valid += 4 + len + input->AttachData_len;
    return 1;
}

int fr_698_esam_input_octet(struct context_698_t* ctx, uint16_t* offset,
                            uint8_t** data, uint16_t* payload_len)
{
    uint16_t len = 0;
    int total = get_octet_string(ctx, offset, &len);

    *data = ctx->apdu_d_rx + *offset;
    *payload_len = len;
    return total;
}

void fr_698_esam_input_store_rn(struct context_698_t* ctx, const uint8_t* data,
                                uint16_t length)
{
    ctx->RN_len = length;
    if (ctx->RN_len > 64) {
        ctx->RN_len = 64;
    }
    memcpy(ctx->RN_data, data, ctx->RN_len);
}
