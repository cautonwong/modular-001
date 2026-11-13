#include "nidavellir.h"
#include "application/adapters/dl645/fr_645_context.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void test_msta_tx_11_to_meter(void **state) 
{
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    struct context_645_t *ctx;
    ctx = &ctx_msta_rx_from_meter;
    instance = (struct instance_t*)*state;
    struct message_t mesg;
    init_test_environment();
    for (size_t i = 0; i < 10; i++)
    {
        generate_data(&mesg.id, 8, 4, 0);//todo 查询所有的待测ID及其长度
        mesg.len = 4;
        mesg.from = kernel;
        mesg.to = msta_tx_to_meter;
        mesg.ctrl = 0x11;
        mesg.len = 4;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        mock_id_data_ok(mesg.id, 4);
        nidavellir_loop_n(instance, 4);
    }
}
