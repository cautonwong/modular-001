#include "nidavellir.h"

extern int fr_tx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_11(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_14(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_enable_tx(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_08(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg);
extern int fr_encode_645_13(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

int fr_msta_tx_to_meter_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    int r = 0;

    switch (mesg->ctrl)
    {
    case 0x11:
        r = fr_encode_645_11(instance, pipeline, mesg);
        break;
    case 0x08:
        r = fr_encode_645_08(instance, pipeline, mesg);
        break;
    case 0x14:
        r = fr_encode_645_14(instance, pipeline, mesg);
        break;
    case 0x13:
        r = fr_encode_645_13(instance, pipeline, mesg);
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

struct filter_t fr_msta_tx_to_meter[] =
{
    {0,fr_tx_645_entry,1,13},
    {1,fr_msta_tx_to_meter_gatekeeper,3,13},
};

struct pipeline_t pl_msta_tx_to_meter =
{
    2,
    "msta_tx_to_meter",
    NULL,
    &pipe_msta_tx_to_meter_req,
    NULL,
    sizeof(fr_msta_tx_to_meter) / sizeof(fr_msta_tx_to_meter[0]),
    fr_msta_tx_to_meter,
};