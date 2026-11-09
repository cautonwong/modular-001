#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"


//数据链路层合法性
//读取一个对象属性（GET）数据链路层测试用例

//1 客户机发起正确GET请求测试 如读取读取安全模式参数
void test_app_698_server_LINK_get_1_ok(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint8_t frm_num = 1;
    uint8_t frm[1][64] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x16
            //68 1B 00 C3 05 01 00 00 00 00 00 00 8F C0 85 01 2F F1 01 02 00 01 16 00 00 00 EE 81 16
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
            ctx = (struct context_698_t*)pl->ctx;
            assert_int_equal(ctx->service, 0x85);
            assert_int_equal(ctx->choice, 0x01);

        }
    }
}

//2 错误帧起始符测试
void test_app_698_server_LINK_get_2_header_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[1][64] = {
        {
            0x69, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x16
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

    for (uint8_t i = 0; i < 1; i++)
    {
        mesg.len = frm[i][1] + 2;
        mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}

//3 错误长度测试
void test_app_698_server_LINK_get_3_len_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = {0};
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = GET_Request;
    mesg.qos = 0;
    mesg.len = 0;
    mesg.id = 0xF1010200;
    mesg.oad_n = 1;
    mesg.err_no = FRM_LEN_ERR2;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    send_mesg(instance, NULL, &mesg, NULL);
    nidavellir_run(instance);
    //判断回应帧是否正常
    pl = get_pipeline(instance, pid_rx_698);
    if (pl != NULL)
    {
        ctx = (struct context_698_t*)pl->ctx;

        assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
        //同时无数据
    }
}

//4 错误控制域测试
void test_app_698_server_LINK_get_4_ctrl_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = {0};
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    //控制码逐个测试00--255 用例需要补充，
    //控制子码也需要00--255 用例需要补充
    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = 245;
    mesg.qos = 0;
    mesg.len = 0;
    mesg.id = 0xF1010200;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    send_mesg(instance, NULL, &mesg, NULL);
    nidavellir_run(instance);
    //判断回应帧是否正常
    pl = get_pipeline(instance, pid_rx_698);
    if (pl != NULL)
    {
        ctx = (struct context_698_t*)pl->ctx;

        assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
        //同时无数据
    }
}

//5 错误通信地址测试
void test_app_698_server_LINK_get_5_addr_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = {0};
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = 245;
    mesg.qos = 0;
    mesg.len = 0;
    mesg.id = 0xF1010200;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    send_mesg(instance, NULL, &mesg, NULL);
    nidavellir_run(instance);
    //判断回应帧是否正常
    pl = get_pipeline(instance, pid_rx_698);
    if (pl != NULL)
    {
        ctx = (struct context_698_t*)pl->ctx;

        assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
        //同时无数据
    }
}

//6 错误客户机地址测试---no
void test_app_698_server_LINK_get_6_CA_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = {0};
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[2][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6,
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x16
        },
        {
            0x68, 0x17, 0x00, 0x43, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6,
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x16
        }
    };
    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < 2; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//7 错误逻辑地址测试
void test_app_698_server_LINK_get_7_logic_addr_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[1][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x15, 0x14, 0x86, 0x32, 0x00, 0x00, 0x80, 0x11, 0x70, 0xaa, 
            0x05, 0x01, 0x2f, 0xf1, 0x01, 0x02, 0x00, 0x00, 0xc4, 0xee, 0x16
        }
    };
    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < 1; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//8 错误HCS帧头校验测试
void test_app_698_server_LINK_get_8_hcs_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[1][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF7, 
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x16
        }
    };
    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < 1; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//9 错误GET-Request测试
void test_app_698_server_LINK_get_9_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[1][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 
            0x0A, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0x76, 0x5f, 0x16
        }
    };
    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < 1; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//10 错误属性测试
void test_app_698_server_LINK_get_10_attribute_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[1][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6,
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x05, 0x00, 0x00, 0xc4, 0xee, 0x16
        }
    };

    //属性错误判断，不回应？？

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < 1; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//11 错误FCS帧校验测试
void test_app_698_server_LINK_get_11_fcs_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };

    uint8_t frm[1][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6,
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xc4, 0xef, 0x16
        }
    };

    //属性错误判断，不回应？？

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < 1; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//12 结束字符测试
void test_app_698_server_LINK_get_12_end(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint8_t frm_num = 2;
    uint8_t frm[2][25] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xaf, 0xe7, 
            0x05, 0x01, 0x09, 0xf1, 0x01, 0x02, 0x00, 0x00, 0x5e, 0x73

        },
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x19
    }
    };

    //属性错误判断，不回应？？

    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = TEST_CASE_Request;
    mesg.qos = 0;
    mesg.len = 25;
    mesg.id = TEST_CASE_ID;
    mesg.oad_n = 1;
    mesg.err_no = 0;
    mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);

    for (uint8_t i = 0; i < frm_num; i++)
    {
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}

///////////////////////////////////////////////////////////////////////
//设置一个对象属性（SET）数据链路层测试用例
//1、客户机发起正确SET请求测试
void test_app_698_server_LINK_set_1_ok(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint8_t frm_num = 1;
    uint8_t frm[1][64] = {
        {
            0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x11, 0x05, 0x5B, 
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1C, 0x07, 0xE3, 
            0x03, 0x0C, 0x0D, 0x26, 
            0x16, 0x00, 0x4B, 0x58, 
            0x16
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
            ctx = (struct context_698_t*)pl->ctx;
            assert_int_equal(ctx->service, 0x86);
            assert_int_equal(ctx->choice, 0x01);
        }
    }
}
//2、错误帧起始符测试
void test_app_698_server_LINK_set_2_header_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint8_t frm_num = 1;
    uint8_t frm[1][64] = {
        //2、错误帧起始符测试
        {
            0x69, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B, 
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1C, 0x07, 0xE3, 0x03, 0x0C, 0x0D, 0x26, 
            0x16, 0x00, 0x4B, 0x58, 0x16
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
        mesg.len = frm[i][1]+2;
        mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }

}
//3、错误长度测试
void test_app_698_server_LINK_set_3_len_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint8_t frm_num = 1;
    uint8_t frm[1][64] = {
        //3、错误长度测试
        {
            0x69, 0x1E, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B, 
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1C, 0x07, 0xE3, 0x03, 0x0C, 0x0D, 0x26, 
            0x16, 0x00, 0x4B, 0x58, 0x16
        },
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
        mesg.len = 0x1F + 2;
        mesg.check = nd_get_crc16((uint8_t*)&mesg, sizeof(struct message_t) - 2);
        send_mesg(instance, NULL, &mesg, frm[i]);
        nidavellir_run(instance);
        //判断回应帧是否正常
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//4、错误控制域测试
void test_app_698_server_LINK_set_4_ctrl_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    uint8_t frm_num = 4;
    uint8_t frm[4][64] = {
        {//DIR和PRM组合意义为服务器对客户机请求的响应
            0x68, 0x1f, 0x00, 0xc3, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xd2, 0x9f,
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0C, 0x0d, 0x26, 0x16, 0x00, 0x4b, 0x58, 0x16
        },
        {//DIR和PRM组合意义为服务器发起的上报
            0x68, 0x1f, 0x00, 0x83, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xb1, 0x79,
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0C, 0x0d, 0x26, 0x16, 0x00, 0x4b, 0x58, 0x16
        },
        
        {//DIR和PRM组合意义为客户机对服务器上报的响应
            0x68, 0x1f, 0x00, 0x03, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x66, 0xbd, 
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0C, 0x0d, 0x26, 0x16, 0x00, 0x4b, 0x58, 0x16
        },
        {//功能码为非3（应用连接管理及数据交换服务）
            0x68, 0x1f, 0x00, 0x41, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xff, 0xc0, 
            0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0C, 0x0d, 0x26, 0x16, 0x00, 0x4b, 0x58, 0x16
        },
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//5、错误通信地址测试
void test_app_698_server_LINK_set_5_addr_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  2
    uint8_t frm[frm_num][64] = {
        {
        //单地址错误
        0x68, 0x1f, 0x00, 0x43, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x6b, 0xf3, 
        0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0c, 0x0d, 0x26, 
        0x16, 0x00, 0x4b, 0x58, 0x16
        },
        {
        //地址长度错误
        0x68, 0x1f, 0x00, 0x43, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x6b, 0xf3, 
        0x06, 0x01, 0x03, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0c, 0x0d, 0x26, 
        0x16, 0x00, 0x4b, 0x58, 0x16
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//6、错误客户机地址测试
void test_app_698_server_LINK_set_6_CA_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  2
    uint8_t frm[frm_num][64] = {
        {
        //设置日期时间参数错误客户机地址
        0x68, 0x1f, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x9e, 0x69, 
        0x06, 0x01, 0x04, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0c, 0x0e, 0x2c, 
        0x0e, 0x00, 0x2a, 0x25, 0x16
        },
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
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->service, 0x86);
            assert_int_equal(ctx->choice, 0x01);
        }
    }
}
//7、错误逻辑地址测试
void test_app_698_server_LINK_set_7_logic_addr_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  1
    uint8_t frm[frm_num][64] = {
        {
        //设置日期时间参数错误逻辑地址
        0x68, 0x1f, 0x00, 0x43, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xe6, 0x32, 
        0x06, 0x01, 0x04, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0c, 0x0e, 0x2c, 
        0x0e, 0x00, 0x2a, 0x25, 0x16
        },
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//8、错误HCS帧头校验测试
void test_app_698_server_LINK_set_8_hcs(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  1
    uint8_t frm[frm_num][64] = {
        {
        //设置日期时间参数错误HCS
        0x68, 0x1f, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x8e, 0x69, 
        0x06, 0x01, 0x04, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0c, 0x0e, 0x2c, 
        0x0e, 0x00, 0xa5, 0xd3, 0x16
        },
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//9、错误SET-Request测试
void test_app_698_server_LINK_set_9_err(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  1
    uint8_t frm[frm_num][64] = {
        {
        //设置日期时间参数错误SET - Request
        0x68, 0x1f, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5b, 
        0x08, 0x01, 0x04, 0x40, 0x00, 0x02, 0x00, 0x1c, 0x07, 0xe3, 0x03, 0x0c, 0x0e, 0x2c, 
        0x0e, 0x00, 0x2a, 0x25, 0x16
        },
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//10、错误FCS帧校验测试
void test_app_698_server_LINK_set_10_fcs(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  1
    uint8_t frm[frm_num][64] = {
        {
        //设置安全模式参数错误FCS
        0x69, 0x19, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xFB, 0xBF, 
        0x06, 0x01, 0x30, 0xF1, 0x01, 0x02, 0x00, 0x16, 0x01, 0x00, 0xCB, 0x78, 0x16
        },
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}
//11、无结束字符测试
void test_app_698_server_LINK_set_11_end(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  2
    uint8_t frm[frm_num][64] = {
        {
        //设置表地址,无结束字符
        0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B, 
        0x06, 0x01, 0x03, 0x40, 0x01, 0x02, 0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0xA6
        },
        {
        //1）设置表地址,结束字符错误
        0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B, 
        0x06, 0x01, 0x03, 0x40, 0x01, 0x02, 0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0xA6, 0x19
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}



//控制字测试用例
//1、传输方向位DIR与启动标志位PRM测试
void test_app_698_server_LINK_ctrl_dir_prm(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  1
    uint8_t frm[frm_num][64] = {
        {
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x26, 0xF6, 
            0x05, 0x01, 0x2F, 0xF1, 0x01, 0x02, 0x00, 0x00, 0xC4, 0xEE, 0x16
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
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->service, 0x85);
            assert_int_equal(ctx->choice, 0x01);
        }
    }
}

//2、分帧标志测试
void test_app_698_server_LINK_ctrl_frm(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  2
    uint8_t frm[frm_num][64] = {
        {
            //设置表地址,无结束字符
            0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B,
            0x06, 0x01, 0x03, 0x40, 0x01, 0x02, 0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0xA6
            },
            {
                //1）设置表地址,结束字符错误
                0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B,
                0x06, 0x01, 0x03, 0x40, 0x01, 0x02, 0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0xA6, 0x19
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}

//3、分帧传输其他测试项
void test_app_698_server_LINK_ctrl_frm_other(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct context_698_t* ctx;
    struct pipeline_t* pl;
    struct message_t mesg = { 0 };
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    #define frm_num  2
    uint8_t frm[frm_num][64] = {
        {
            //设置表地址,无结束字符
            0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B,
            0x06, 0x01, 0x03, 0x40, 0x01, 0x02, 0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0xA6
            },
            {
                //1）设置表地址,结束字符错误
                0x68, 0x1F, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x05, 0x5B,
                0x06, 0x01, 0x03, 0x40, 0x01, 0x02, 0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0xA6, 0x19
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
        pl = get_pipeline(instance, pid_rx_698);
        if (pl != NULL)
        {
            ctx = (struct context_698_t*)pl->ctx;

            assert_int_equal(ctx->fr_err, FRM_HEADER_ERR);
            //同时无数据
        }
    }
}

//4、扰码标志位测试
void test_app_698_server_LINK_ctrl_sc(void** state)
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
    uint8_t frm[frm_num][64] = {
        {
            //开启扰码标志位链路层请求
            0x68, 0x17, 0x00, 0x4B, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x65, 0x98,
            0x38, 0x34, 0x55, 0x24, 0x34, 0x35, 0x33, 0x33, 0xC8, 0x1D, 0x16
        },
        {
            //未开启扰码标志位链路层请求
            0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xAF, 0xE7,
            0x05, 0x01, 0x1D, 0xF1, 0x01, 0x02, 0x00, 0x00, 0x42, 0x21, 0x16
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
