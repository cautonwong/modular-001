#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void test_msta_tx_86_to_sta(void **state) 
{
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    instance = (struct instance_t*)*state;
    struct message_t mesg;
    uint8_t buf[200];
    uint8_t len;
    init_test_environment();
    for (size_t i = 0; i < 10; i++)
    {
        generate_data(&mesg.id, 8, 4, 0);//todo 查询所有的待测ID及其长度
        mesg.from = kernel;
        mesg.to = msta_tx_to_sta;
        mesg.ctrl = 0x86;
        mesg.len = 4;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        mock_id_data_ok(mesg.id, 4);
        nidavellir_loop_n(instance, 4);
    }
    for (size_t i = 0; i < 10; i++)
    {
        len = 4 * (i + 1);
        generate_data(&mesg.id, 8, 4, 0);//todo 查询所有的待测ID及其长度
        generate_data(buf, 200, len, 0);//生成要上报的数据内容
        mesg.from = kernel;
        mesg.to = msta_tx_to_sta;
        mesg.ctrl = 0x86;
        mesg.len = 4+len;

        send_mesg(instance, &pl_kernel, &mesg, buf);
        expect_value(__write, id, mesg.id);
        expect_memory(__write, buf, buf, len);
        expect_value(__write, len, len);

        nidavellir_loop_n(instance, 4);
    }
}