#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
#include "application/adapters/dl645/fr_645_message.h"

int fr_rx_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg);
int fr_gatekeeper_645_server(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl645_msg_t* mesg);

static void message_from_legacy(const struct message_t* src, nd_dl645_msg_t* dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->magic = src->magic;
    dst->from = src->from;
    dst->to = src->to;
    dst->ctrl = src->ctrl;
    dst->qos = src->qos;
    dst->id = src->id;
    dst->len = src->len;
    dst->oad_n = src->oad_n;
    dst->x = src->x;
    dst->payload = src->payload;
    dst->check = src->check;
}


int app_rx_server_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    
    if(ctx == NULL){
        return 0;
    }

    nd_dl645_msg_t local;

    message_from_legacy(mesg, &local);
    ctx->fr_err = fr_rx_645_entry(instance, pipeline, &local);
    if (ctx->fr_err > 0)
    {
        ctx->fr_err = fr_gatekeeper_645_server(instance, pipeline, &local);
        return ctx->fr_err;
    }
    return ctx->fr_err;
}

int app_tx_server_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 0;
}


int app_rx_client_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 0;
}

int app_tx_client_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 0;
}



