#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
void test_sta_tx_11_to_msta(void **state) 
{
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    struct context_645_t *ctx;
    instance = (struct instance_t*)*state;
    struct message_t mesg;
    init_test_environment();
    clock_t start;
    start = clock();

    for (size_t i = 0; i < 10; i++)
    {
        generate_data(&mesg.id, 8, 4, 0);//todo 查询所有的待测ID及其长度
        mesg.len = 4;
        mesg.from = kernel;
        mesg.to = sta_tx_to_msta;
        mesg.ctrl = 0x11;
        mesg.len = 4;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        mock_id_data_ok(mesg.id, 4);
        nidavellir_loop_n(instance, 4);
    }
    printf("match %f\n", (double)((clock() - start) / CLOCKS_PER_SEC));
}
