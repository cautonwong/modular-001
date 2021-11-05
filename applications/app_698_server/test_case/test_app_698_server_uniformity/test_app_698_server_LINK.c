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
    struct message_t mesg = {0};
    struct security_t {
        uint32_t oad;
        uint8_t cmd;
    };
    //68 17 00 43 05 01 00 00 00 00 00 10 26 F6 05 01 2F F1 01 02 00 00 C4 EE 16
    //68 1B 00 C3 05 01 00 00 00 00 00 00 8F C0 85 01 2F F1 01 02 00 01 16 00 00 00 EE 81 16
    mesg.magic = MSG_MAGIC;
    mesg.from = kernel;
    mesg.to = pid_client_tx_698;
    mesg.ctrl = GET_Request;;
    mesg.qos = 0; 
    mesg.len = 0;
    mesg.id = 0xF1010200;
    mesg.oad_n = 1;
    mesg.check = nd_get_crc16((uint8_t *)&mesg, sizeof(struct message_t) - 2);

    send_mesg(instance, NULL, &mesg, NULL);
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

//2 错误帧起始符测试
void test_app_698_server_LINK_get_2_header_err(void** state)
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
    mesg.err_no = FRM_HEADER_ERR;
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

        assert_int_equal(ctx->fr_err, FRM_CTRL_ERR);
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

        assert_int_equal(ctx->fr_err, FRM_CTRL_ERR);
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
{}
//2、错误帧起始符测试
void test_app_698_server_LINK_set_2_header_err(void** state)
{}
//3、错误长度测试
void test_app_698_server_LINK_set_3_len_err(void** state)
{}
//4、错误控制域测试
void test_app_698_server_LINK_set_4_ctrl_err(void** state)
{}
//5、错误通信地址测试
void test_app_698_server_LINK_set_5_addr_err(void** state)
{}
//6、错误客户机地址测试
void test_app_698_server_LINK_set_6_CA_err(void** state)
{}
//7、错误逻辑地址测试
void test_app_698_server_LINK_set_7_logic_addr_err(void** state)
{}
//8、错误HCS帧头校验测试
void test_app_698_server_LINK_set_8_hcs(void** state)
{}
//9、错误SET-Request测试
void test_app_698_server_LINK_set_9_err(void** state)
{}
//10、错误FCS帧校验测试
void test_app_698_server_LINK_set_10_fcs(void** state)
{

}
//11、无结束字符测试
void test_app_698_server_LINK_set_11_end(void** state)
{
}



//控制字测试用例
//1、传输方向位DIR与启动标志位PRM测试
void test_app_698_server_LINK_ctrl_dir_prm(void** state)
{
}

//2、分帧标志测试
void test_app_698_server_LINK_ctrl_frm(void** state)
{
}

//3、分帧传输其他测试项
void test_app_698_server_LINK_ctrl_frm_other(void** state)
{

}

//4、扰码标志位测试
void test_app_698_server_LINK_ctrl_sc(void** state)
{
}
