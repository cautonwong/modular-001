#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void test_sync_time(void **state) 
{
    //arrange
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    struct context_645_t *ctx;
    ctx = &ctx_bt_rx_from_msta;
    instance = (struct instance_t*)*state;
    struct ipc_object_t ipc_object;
    ipc_object.ctrl = 0x11;
    generate_data(&ipc_object.id, 8, 4, 0);
    fifoBuf_putData(&pipe_bt_tx_to_msta_req,&ipc_object,sizeof(struct ipc_object_t ));
    mock_id_data_ok(ipc_object.id);
    //1.设置msta的时间和meter的时间偏差
    //2.cco给msta发对时
    //3.msta读取meter的时间
    //4.msta给meter对时
    //5.meter的时候应该对过来了

    //action 
    nidavellir_run(instance);
    
    //assert
    ctx = &ctx_bt_rx_from_msta;
    assert_true(0x91 == ctx->ctrl);
    for (size_t i = 0; i < instance->recipe->pipeline_n; i++)
    {
        pipeline = instance->recipe->pipelines[i];
        if (pipeline->in_queue)
        {
            //assert_true(fifoBuf_getUsed(pipeline->in_queue) == 0);
            if (fifoBuf_getUsed(pipeline->in_queue) != 0)
            {
                printf("%s in unclean %d\n",pipeline->name, fifoBuf_getUsed(pipeline->in_queue));
            }
        }
        if (pipeline->out_queue)
        {
            // assert_true(fifoBuf_getUsed(pipeline->out_queue) == 0);
            if (fifoBuf_getUsed(pipeline->out_queue) != 0)
            {
                printf("%s out unclean %d\n", pipeline->name, fifoBuf_getUsed(pipeline->out_queue));
            }
        }
    }
}
