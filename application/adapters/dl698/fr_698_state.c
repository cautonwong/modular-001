#include <stdbool.h>
#include "application/ports/fifo.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "fr_698.h"
#include "fr_698_internal.h"
#include "meter_base_view.h"

int fr_ctx_clear(struct instance_t*, struct pipeline_t* pipeline, nd_dl698_msg_t*)
{
    struct context_698_t* ctx = (struct context_698_t*)nd_fr698_pipeline_ctx(pipeline);
    ctx->apdu_len_valid = 0;
    ctx->segment_tx = 0;
    ctx->resp_choice = 0;
    ctx->err_n = 0;
    ringbuffer_clear(ctx->apdu);
    ctx->apdu_rx_offset = 0;
    return 0;
}
int fr698_inital(struct instance_t* instance, struct pipeline_t*)
{

    nd_mb_current_connect_channel(instance)[0] = 0;
    nd_mb_current_connect_channel(instance)[1] = 0;
    return 1;
}
