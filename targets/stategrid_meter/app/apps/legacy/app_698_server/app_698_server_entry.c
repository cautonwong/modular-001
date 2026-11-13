#include "nidavellir.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "application/adapters/dl698/fr_698_message.h"
#include "lib_ana.h"

int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_gatekeeper_698_server(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);
int fr_gatekeeper_698_client(struct instance_t* instance, struct pipeline_t* pipeline, nd_dl698_msg_t* mesg);

static void message_from_legacy(const struct message_t* src, nd_dl698_msg_t* dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->id = src->id;
    dst->len = src->len;
    dst->oad_n = src->oad_n;
    dst->ctrl = src->ctrl;
    dst->from = src->from;
    dst->to = src->to;
    dst->qos = src->qos;
    dst->payload = src->payload;
}


int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    
    if(ctx == NULL){
        return 0;
    }
    nd_dl698_msg_t local;
    message_from_legacy(mesg, &local);
    ctx->fr_err = fr_rx_698_entry(instance, pipeline, &local);
    if (ctx->fr_err > 0)
    {
        ctx->fr_err = fr_gatekeeper_698_server(instance, pipeline, &local);
        return ctx->fr_err;
    }
    return 0;
}


int app_tx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    
    if(ctx == NULL){
        return 0;
    }
    nd_dl698_msg_t local;
    message_from_legacy(mesg, &local);
    if (fr_tx_698_entry(instance, pipeline, &local))
    {
        return fr_gatekeeper_698_client(instance, pipeline, &local);
    }
    return 0;
}


int app_rx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    if (ctx == NULL) {
        return 0;
    }
    nd_dl698_msg_t local;
    message_from_legacy(mesg, &local);
    ctx->fr_err = 0;
    ctx->fr_err = fr_rx_698_entry(instance, pipeline, &local);
    if (ctx->fr_err > 0)
    {
        ctx->fr_err = fr_gatekeeper_698_client(instance, pipeline, &local);
        return ctx->fr_err;
    }

    return 0;
}

int app_tx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    if (ctx == NULL) {
        return 0;
    }
    nd_dl698_msg_t local;
    message_from_legacy(mesg, &local);
    if (fr_tx_698_entry(instance, pipeline, &local))
    {
        return fr_gatekeeper_698_client(instance, pipeline, &local);
    }
    return 0;
}



