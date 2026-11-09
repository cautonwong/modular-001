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

void test_app_698_server_CONNECT_Request(void** state)
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



//1、期望的应用层协议版本号测试
void test_app_698_server_CONNECT_Request_1_version(void** state)
{
#if 0
    struct instance_t* instance = (struct instance_t*)*state;
    struct fifo_t* fifo;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    test_app_698_server_CONNECT_Request(state);
#define frm_num  2
    uint8_t frm_1[frm_num][128] = {
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
#define FRM_BUF_SIZE 512
    uint8_t frm_buf[FRM_BUF_SIZE] = { 0 };
    char frm[frm_num][FRM_BUF_SIZE*2] = {
        {//协议版本14
            "68 5E 00 43 05 01 00 00 00 00 00 11 91 C3 \
            02 26 00 14 FF FF FF FF C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 02 00 01 07 D0 00 00 1C 20 02 20 13 4B E1 B5 A9 EC DC 99 90 AA C8 AE 71 C3 B2 D3 C4 CA 6C F5 EA EB A9 A3 3A 9A 47 A3 7E 38 29 D4 04 35 FC C6 EA 00 A0 3F 16"
        },
        {
            //协议版本13
            "68 5E 00 43 05 01 00 00 00 00 00 11 91 C3 \
            02 26 00 13 FF FF FF FF C0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 02 00 01 07 D0 00 00 1C 20 02 20 13 4B E1 B5 A9 EC DC 99 90 AA C8 AE 71 C3 B2 D3 C4 CA 6C F5 EA EB A9 A3 3A 9A 47 A3 7E 38 29 D4 04 35 FC C6 EA 00 8b b0 16"
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
        hex2bin(frm_buf, frm[i], sizeof(frm[i])/2);
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
#endif
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
