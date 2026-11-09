#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void test_sta_tx_07_to_meter(void **state) 
{
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    struct context_645_t *ctx;
    instance = (struct instance_t*)*state;
    struct message_t mesg;
    mesg.ctrl = 0x07;
    generate_data(&mesg.id, 8, 4, 0);
    fifoBuf_putData(&pipe_sta_tx_to_msta_req,&mesg,sizeof(struct message_t));
    //fifoBuf_putData(&pipe_msta, &ipc_object, sizeof(struct ipc_object_t));
    mock_id_data_ok(mesg.id,4);

    nidavellir_run(instance);
    nidavellir_run(instance);
    ctx = &ctx_sta_rx_from_msta;
    assert_true(0x91== ctx->ctrl);
    for (size_t i = 0; i < instance->recipe->pipeline_n; i++)
    {
        pipeline = instance->recipe->pipelines[i];
        if (pipeline->in_queue)
            assert_true(fifoBuf_getUsed(pipeline->in_queue) == 0);
        if (pipeline->out_queue)
            assert_true(fifoBuf_getUsed(pipeline->out_queue) == 0);
    }
}
