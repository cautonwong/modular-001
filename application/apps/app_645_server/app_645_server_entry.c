#include "nidavellir.h"

int fr_rx_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
int fr_gatekeeper_645_server(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

int app_rx_server_645_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct context_645_t* ctx = (struct context_645_t*)pipeline->ctx;
    
    if(ctx == NULL){
        return 0;
    }

    ctx->fr_err = fr_rx_645_entry(instance, pipeline, mesg);
    if (ctx->fr_err > 0)
    {
        ctx->fr_err = fr_gatekeeper_645_server(instance, pipeline, mesg);
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



