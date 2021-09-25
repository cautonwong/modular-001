#include "nidavellir.h"

extern int fr_tx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_tx_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_86(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_enable_tx(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);

int fr_msta_tx_to_sta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;

    switch (mesg->ctrl)
    {
    case 0x86:
        r = fr_encode_645_86(instance, pipeline, mesg);
        break;
    default:
        //todo  不认识的报文  
        break;
    }

    if (r == 1)
    {
        fr_tx_645_end(instance, pipeline, mesg);
    }

    return 0;
}

struct filter_t fr_msta_tx_to_sta[] =
{
    {0,fr_tx_645_entry,1,13},
    {1,fr_msta_tx_to_sta_gatekeeper,3,13},
};

struct pipeline_t pl_msta_tx_to_sta =
{
    2,
    "msta_tx_to_sta",
    NULL,
    &pipe_msta_tx_to_meter_req,
    NULL,
    sizeof(fr_msta_tx_to_sta) / sizeof(fr_msta_tx_to_sta[0]),
    fr_msta_tx_to_sta,
};
