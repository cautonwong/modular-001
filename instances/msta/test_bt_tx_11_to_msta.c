#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

void test_bt_tx_11_to_msta(void** state)
{
    struct instance_t* instance;
    struct pipeline_t* pipeline;
    instance = (struct instance_t*)*state;
    struct message_t mesg;
    init_test_environment();
    uint8_t buf[] = { 0xfe,0xfe, 0xfe, 0xfe, 0x68, 0x00, 0x00, 0xff, 0xff, 0xee, 0xee, 0x68, 0x11, 0x04, 0x34, 0x33, 0xb3, 0x41, 0x1a, 0x16 };
    fifoBuf_putData(&pipe_msta_rx_from_bt, buf, sizeof(buf));
    mock_id_data_ok(0x0e800001, 25);
    nidavellir_loop_n(instance, 4);

    /*
    for (size_t i = 0; i < 20; i++)
    {
        generate_data(&mesg.id, 8, 4, 0);//todo 查询所有的待测ID及其长度
        mesg.len = 4;
        mesg.from = kernel;
        mesg.to = bt_tx_to_msta;
        mesg.ctrl = 0x11;
        mesg.len = 4;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        mock_id_data_ok(mesg.id, 4);
        nidavellir_loop_n(instance, 4);
    }
    */
}

void test_msta_tx_91_to_bt(void** state)
{
    struct instance_t* instance;
    struct pipeline_t* pipeline;
    instance = (struct instance_t*)*state;
    struct message_t mesg;
    uint8_t buf[200];
    uint8_t len;
    init_test_environment();
    for (size_t i = 0; i < 10; i++)
    {
        generate_data(&mesg.id, 8, 4, 0);//todo 查询所有的待测ID及其长度
        mesg.from = kernel;
        mesg.to = msta_tx_to_bt;
        mesg.ctrl = 0x91;
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
        mesg.to = msta_tx_to_bt;
        mesg.ctrl = 0x91;
        mesg.len = 4 + len;

        send_mesg(instance, &pl_kernel, &mesg, buf);
        expect_value(__write, id, mesg.id);
        expect_memory(__write, buf, buf, len);
        expect_value(__write, len, len);

        nidavellir_loop_n(instance, 4);
    }
}