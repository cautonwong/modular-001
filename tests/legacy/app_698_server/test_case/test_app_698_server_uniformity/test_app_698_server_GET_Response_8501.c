#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"

//1.4 Get-Response数据类型测试用例
//1、读取一个对象属性响应Get-Response数据类型测试 test_app_698_server_GET_Response_8501
//2、读取若干个对象属性响应Get-ResponseList数据类型测试 test_app_698_server_GET_Response_8502
//3、读取一个记录型对象响应GetResponseRecord数据类型测试 test_app_698_server_GET_Response_8503
//4、读取若干个记录型对象属性响应GetResponseRecordList数据类型测试 test_app_698_server_GET_Response_8504
//5、读取分帧响应的下一个数据块响应GetResponseNext数据类型测试  不支持
//6、读取一个对象属的MD5值响应GetResponseMD5数据类型测试


void test_app_698_server_GET_Response_8501(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct fifo_t* fifo;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint16_t suites_num = 0;
    uint8_t frm_buf[512] = { 0 };
    uint8_t apdu[512] = { 0 };
    int cmd = 0;
    int auth = 0;
    int response = 0;

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;

    suites_num = get_test_suites_num(__FILE__);

    for (uint8_t i = 0; i < suites_num; i++)
    {
        if (get_test_suite_frame(__FILE__, frm_buf, i, TEST_source) < 0)
            continue;

        cmd = get_test_suite_cmd(__FILE__, frm_buf, i, TEST_source);
        if (cmd == TEST_frame_check)//是否对帧校验进行检查
        {

        }
        auth = get_test_suite_auth(__FILE__, frm_buf, i, TEST_source);
        if (auth == 0)//是否对apdu进行加密
        {
            get_test_suite_apdu(__FILE__, apdu, i, TEST_source);//重新数据帧
        }

        mesg.len = frm_buf[1] + 2;
        mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);
        send_mesg(instance, NULL, &mesg, frm_buf);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_client_rx_698);
        if (pl != NULL)
        {
            fifo = (struct fifo_t*)pl->out_queue;
            ctx = (struct context_698_t*)pl->ctx;
            if (ctx != NULL)
            {
                response = get_test_suite_frame(__FILE__, frm_buf, i, TEST_expectation);
                if (response == TEST_response_no)//是否有数据帧回应
                {
                    assert_int_equal(ctx->fr_err, FRM_RSP_ERR);
                    continue;
                }
                //控制命令判断
                cmd = get_test_suite_cmd(__FILE__, frm_buf, i, TEST_source);
                
                assert_int_equal(ctx->service, (cmd&0xff00)>>8);
                assert_int_equal(ctx->choice, cmd&0x00ff);

                assert_memory_equal(frm_buf, ctx->apdu, 0);

            }
        }
    }
}

