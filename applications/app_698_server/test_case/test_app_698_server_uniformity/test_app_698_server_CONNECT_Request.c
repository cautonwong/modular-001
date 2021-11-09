#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"


//建立应用连接测试用例
//1、期望的应用层协议版本号测试
//2、期望的协议一致性块测试
//3、期望的功能一致性块测试
//4、客户机发送帧最大尺寸
//5、客户机接收帧最大尺寸
//7、客户机接收帧最大窗口尺寸
//8、客户机最大可处理APDU尺寸
//9、期望的应用连接超时时间
//10、认证请求对象

//1、期望的应用层协议版本号测试
void test_app_698_server_CONNECT_Request_1_version(void** state)
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
#define frm_num  2
    uint8_t frm[frm_num][128] = {
        {
            //协议版本14
            0x68, 0x5E, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x91, 0xC3, 
            0x02, 0x26, 
            0x00, 0x14, 
            0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x02, 0x00, 0x02, 0x00, 0x01, 0x07, 0xD0, 0x00, 0x00, 0x1C, 0x20, 0x02, 0x20, 0x13, 
            0x4B, 0xE1, 0xB5, 0xA9, 0xEC, 0xDC, 0x99, 0x90, 0xAA, 0xC8, 0xAE, 0x71, 0xC3, 0xB2, 
            0xD3, 0xC4, 0xCA, 0x6C, 0xF5, 0xEA, 0xEB, 0xA9, 0xA3, 0x3A, 0x9A, 0x47, 0xA3, 0x7E, 
            0x38, 0x29, 0xD4, 0x04, 0x35, 0xFC, 0xC6, 0xEA, 0x00, 0xA0, 0x3F, 0x16
        },
        {
            //协议版本13
            0x68, 0x5E, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x91, 0xC3,
            0x02, 0x26, 
            0x00, 0x13, 
            0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x02, 0x00, 0x01, 0x07, 0xD0, 0x00, 0x00, 0x1C, 0x20, 0x02, 0x20, 0x13,
            0x4B, 0xE1, 0xB5, 0xA9, 0xEC, 0xDC, 0x99, 0x90, 0xAA, 0xC8, 0xAE, 0x71, 0xC3, 0xB2,
            0xD3, 0xC4, 0xCA, 0x6C, 0xF5, 0xEA, 0xEB, 0xA9, 0xA3, 0x3A, 0x9A, 0x47, 0xA3, 0x7E,
            0x38, 0x29, 0xD4, 0x04, 0x35, 0xFC, 0xC6, 0xEA, 0x00, 0x8b, 0xb0, 0x16
        }
    };

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;

    for (uint8_t i = 0; i < frm_num; i++)
    {
        mesg.len = frm[i][1] + 2;
        mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_client_rx_698);
        if (pl != NULL)
        {
            fifo = (struct fifo_t*)pl->out_queue;
            ctx = (struct context_698_t*)pl->ctx;
            if (ctx != NULL)
            {
                assert_int_equal(ctx->fr_err, FRM_OK);
                if (i == 0)
                {
                    assert_int_equal(is_scrambler_set(ctx->ctrl), 1);
                    assert_int_equal(ctx->service, 0x85);
                }
                else
                {
                    assert_int_equal(is_scrambler_set(ctx->ctrl), 0);
                    assert_int_equal(ctx->service, 0x85);
                }

                assert_int_equal(ctx->choice, 0x01);
            }
        }
    }
}


//2、期望的协议一致性块测试
void test_app_698_server_CONNECT_Request_2_pro(void** state)
{
}

//3、期望的功能一致性块测试
void test_app_698_server_CONNECT_Request_3_func(void** state)
{
}

//4、客户机发送帧最大尺寸
void test_app_698_server_CONNECT_Request_4_tx_max(void** state)
{
}

//5、客户机接收帧最大尺寸
void test_app_698_server_CONNECT_Request_5_rx_max(void** state)
{
}

//7、客户机接收帧最大窗口尺寸
void test_app_698_server_CONNECT_Request_7_win_max(void** state)
{
}

//8、客户机最大可处理APDU尺寸
void test_app_698_server_CONNECT_Request_8_apdu_max(void** state)
{
}

//9、期望的应用连接超时时间
void test_app_698_server_CONNECT_Request_9_timeout(void** state)
{
}

//10、认证请求对象
void test_app_698_server_CONNECT_Request_10_auth(void** state)
{
}
