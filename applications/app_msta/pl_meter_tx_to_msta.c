#include "nidavellir.h"

extern int fr_tx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_encode_645_86(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_tx_645_end(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
int fr_meter_tx_to_msta_gatekeeper(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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

//电表主动上报
struct filter_t fr_meter_tx_to_msta[] =
{
    {0,fr_tx_645_entry,1,13},
    {1,fr_meter_tx_to_msta_gatekeeper,3,13},
};
//msta接收到来自meter的报文处理流水线
struct pipeline_t pl_meter_tx_to_msta =
{
    2,
    "meter_tx_to_msta",
    NULL,
    &pipe_sta_tx_to_msta_req,//来自测试用例或上位机
    &pipe_msta_rx_from_sta,
    sizeof(fr_meter_tx_to_msta) / sizeof(fr_meter_tx_to_msta[0]),
    fr_meter_tx_to_msta,
};

