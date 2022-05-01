#include "nidavellir.h"

int fr_rx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_tx_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_gatekeeper_698_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_gatekeeper_698_client(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    
    if(ctx == NULL){
        return 0;
    }
    ctx->fr_err = fr_rx_698_entry(instance, pipeline, mesg);
    if (ctx->fr_err > 0)
    {
        ctx->fr_err = fr_gatekeeper_698_server(instance, pipeline, mesg);
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
    if (fr_tx_698_entry(instance, pipeline, mesg))
    {
        return fr_gatekeeper_698_client(instance, pipeline, mesg);
    }
    return 0;
}


int app_rx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_698_t* ctx = (struct context_698_t*)pipeline->ctx;
    if (ctx == NULL) {
        return 0;
    }
    ctx->fr_err = 0;
    ctx->fr_err = fr_rx_698_entry(instance, pipeline, mesg);
    if (ctx->fr_err > 0)
    {
        ctx->fr_err = fr_gatekeeper_698_client(instance, pipeline, mesg);
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
    if (fr_tx_698_entry(instance, pipeline, mesg))
    {
        return fr_gatekeeper_698_client(instance, pipeline, mesg);
    }
    return 0;
}



