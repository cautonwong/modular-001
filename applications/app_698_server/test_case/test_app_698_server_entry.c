#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"

#include "test_app_698_server_uniformity.h"

extern int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_tx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_rx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_tx_client_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);


extern int open_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int close_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_tx_data_to_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern struct context_serial_t* create_serial_port(char* filename);
extern int nidavellir_run_baremetal(struct instance_t* instance);


//测试步骤指南
//1.找到被测试函数
// 比如app_698_server_rx_entry这个函数才是真正要测试验证的函数
//2.找到桩函数   
//  比如资源管理器api
//3.打桩桩函数  打桩方法见test_case_1
//注意:
//1.测试工程跟app工程分开  

//桩函数  测试时不能依赖resource manager
//在调用_nd_get之前先调用以下代码打桩：
//uint8_t* mock_data[];
//will_return(_nd_get, mock_data);
//will_return(_nd_get, sizeof(mock_data);
int8_t _nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    uint8_t* p = mock_ptr_type(uint8_t*);
    int r = mock_type(int);
    if (r > 0)
    {
        memcpy(buf, p, r);
    }
    return r;
}

int8_t nd_set(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    return 0;
}

//测试工具函数helper
int let_nd_get_return_ok(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
{
    return 0;
}


struct fifo_t stdin_698;
struct fifo_t stdout_698;
struct fifo_t pipe_tx_698_req;

//表接收处理server_698
const struct filter_t fr_rx_server_698[1] =
{
    {0x01,app_rx_server_698_entry,0,13},
};

struct context_698_t ctx_rx_server_698;
struct pipeline_t pl_meter_rx_from_serial =
{
    pid_rx_698,
    "pl_rx_server_698",
    &ctx_rx_server_698,
    &stdin_698,//serial driver isr put byte into this 
    &stdout_698,//pl_rx_server_698 put apdu into this 
    app_rx_server_698_entry,
    sizeof(fr_rx_server_698) / sizeof(fr_rx_server_698[0]),
    fr_rx_server_698,
};

//表发送处理server_698
const struct filter_t fr_tx_server_698[1] =
{
    {0x01,app_tx_server_698_entry,0,13},
};

struct context_698_t ctx_tx_server_698;
struct pipeline_t pl_meter_tx_to_serial =
{
    pid_tx_698,
    "pl_tx_server_698",
    &ctx_tx_server_698,
    &pipe_tx_698_req,
    &stdout_698,
    app_tx_server_698_entry,
    sizeof(fr_tx_server_698) / sizeof(fr_tx_server_698[0]),
    fr_tx_server_698,
};

/////////////////////////////////////////////////////////
struct fifo_t stdin_client_698;
struct fifo_t stdout_client_698;

const struct filter_t serial_tx_to_meter[1] =
{
    {0x01,app_tx_client_698_entry,0,13},
};
struct context_698_t ctx_tx_client_698;
struct pipeline_t pl_serial_tx_to_meter =
{
    pid_client_tx_698,
    "pl_serial_tx_to_meter",
    &ctx_tx_client_698,
    &stdout_client_698,
    &stdin_698,
    app_tx_client_698_entry,
    sizeof(serial_tx_to_meter) / sizeof(serial_tx_to_meter[0]),
    serial_tx_to_meter,
};

const struct filter_t serial_rx_from_meter[1] =
{
    {0x01,app_rx_client_698_entry,0,13},
};

struct context_698_t ctx_rx_client_698;
struct pipeline_t pl_serial_rx_from_meter =
{
    pid_client_rx_698,
    "pl_serial_rx_from_meter",
    &ctx_rx_client_698,
    &stdout_698,
    &stdout_client_698,
    app_rx_client_698_entry,
    sizeof(serial_rx_from_meter) / sizeof(serial_rx_from_meter[0]),
    serial_rx_from_meter,
};

//本app包含的pipeline
struct pipeline_t* pipelines[] =
{
    &pl_serial_tx_to_meter,
    &pl_meter_rx_from_serial,
    &pl_meter_tx_to_serial,
    &pl_serial_rx_from_meter,
};


struct context_serial_t ctx_serial_1 = { {L"\\\\.\\com10"},NULL };

const struct filter_t fr_rx_from_serial[] =
{
    {0x01,fr_rx_data_from_serial,0x00,13},
};

const struct filter_t fr_tx_to_serial[] =
{
    {0x01,fr_tx_data_to_serial,0x00,13},
};

struct pipeline_t pl_rx_from_serial =
{
    pid_rx_from_serial,
    "pl_rx_from_serial",
    &ctx_serial_1,
    NULL,
    &stdin_698,
    fr_rx_data_from_serial,
    sizeof(fr_rx_from_serial) / sizeof(fr_rx_from_serial[0]),
    fr_rx_from_serial,
};

struct pipeline_t pl_tx_to_serial =
{
    pid_tx_to_serial,
    "pl_tx_to_serial",
    &ctx_serial_1,
    &stdout_698,
    NULL,
    fr_tx_data_to_serial,
    sizeof(fr_tx_to_serial) / sizeof(fr_tx_to_serial[0]),
    fr_tx_to_serial,
};

struct pipeline_t* sim_pipelines[] =
{
    &pl_meter_rx_from_serial,
    &pl_meter_tx_to_serial,
    &pl_tx_to_serial,
};


//*********************上述的初始化方法要删掉 wong*********************//
struct recipe_t recipe;
int test_app_setup(void** state)
{
    struct instance_t* instance = malloc(sizeof(struct instance_t));
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe.name = 0;
    instance->recipe = &recipe;

    fifoBuf_init(&stdout_698, malloc(512), 512);
    fifoBuf_init(&stdin_698, malloc( 512), 512);
    fifoBuf_init(&pipe_tx_698_req, malloc(512), 512);

    instance->cache = malloc(256);
    instance->cache2 = malloc(sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    
    ctx_rx_server_698.connect_info = malloc(128);//临时
    ctx_rx_server_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_rx_server_698.apdu, malloc(2048), 2048);


    ctx_tx_server_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_tx_server_698.apdu, malloc(2048), 2048);
    
    ctx_rx_client_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_rx_client_698.apdu, malloc(2048), 2048);

    ctx_tx_client_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_tx_client_698.apdu, malloc(2048), 2048);
    
    fifoBuf_init(&stdin_client_698, malloc(512), 512);
    fifoBuf_init(&stdout_client_698, malloc(512), 512);

    *state = instance;
    return 0;
}

//这是第一测试用例  示例--展示如何进行打桩和如何比对测试结果
void test_case_1(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    uint8_t buf[4];
    uint32_t id = 0x4000;
    uint32_t data = 0x12345678;
    will_return(_nd_get, &data);
    will_return(_nd_get, 4);
    int r = _nd_get(instance, id, buf, 4);
    assert_true(r == 4);
    assert_memory_equal(buf, &data, 4);
}

//这是第2个测试用例  示例--不需要kernel测试  即单个pipeline独立运行、测试和发布
//彰显package的可重排性
void test_case_running_by_self(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    struct message_t mesg;

    open_serial_port(instance, &pl_rx_from_serial, NULL);

    while (1)
    {
        fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);
        app_rx_server_698_entry(instance, &pl_meter_rx_from_serial, &mesg);
        fr_tx_data_to_serial(instance, &pl_tx_to_serial, &mesg);
    }
}

//这是第3个测试用例  示例--跟kernel一起测试
void test_case_running_with_kernel(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    
    open_serial_port(instance, &pl_rx_from_serial, NULL);
    while (1)
    {
        nidavellir_run_baremetal(instance);
    }
}



#if 0
int main(void)
{
    //这是所有的测试用例列表
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_case_1),

#if 0
        //1 test_app_698_server_uniformity
        //1.1数据链路层合法性
        //1.1.1 test_app_698_server_LINK_get
            //1 客户机发起正确GET请求测试 如读取读取安全模式参数
            cmocka_unit_test(test_app_698_server_LINK_get_1_ok),
            //2 错误帧起始符测试
            cmocka_unit_test(test_app_698_server_LINK_get_2_header_err),
            //3 错误长度测试
            cmocka_unit_test(test_app_698_server_LINK_get_3_len_err),
            //4 错误控制域测试
            cmocka_unit_test(test_app_698_server_LINK_get_4_ctrl_err),
            //5 错误通信地址测试
            cmocka_unit_test(test_app_698_server_LINK_get_5_addr_err),
            //6 错误客户机地址测试
            cmocka_unit_test(test_app_698_server_LINK_get_6_CA_err),
            //7 错误逻辑地址测试
            cmocka_unit_test(test_app_698_server_LINK_get_7_logic_addr_err),
            //8 错误HCS帧头校验测试
            cmocka_unit_test(test_app_698_server_LINK_get_8_hcs_err),
            //9 错误GET-Request测试
            cmocka_unit_test(test_app_698_server_LINK_get_9_err),
            //10 错误属性测试
            cmocka_unit_test(test_app_698_server_LINK_get_10_attribute_err),
            //11 错误FCS帧校验测试
            cmocka_unit_test(test_app_698_server_LINK_get_11_fcs_err),
            //12 结束字符测试
            cmocka_unit_test(test_app_698_server_LINK_get_12_end),

        //1.1.2 test_app_698_server_LINK_set
            //1、客户机发起正确SET请求测试
            cmocka_unit_test(test_app_698_server_LINK_set_1_ok),
            //2、错误帧起始符测试
            cmocka_unit_test(test_app_698_server_LINK_set_2_header_err),
            //3、错误长度测试
            cmocka_unit_test(test_app_698_server_LINK_set_3_len_err),
            //4、错误控制域测试
            cmocka_unit_test(test_app_698_server_LINK_set_4_ctrl_err),
            //5、错误通信地址测试
            cmocka_unit_test(test_app_698_server_LINK_set_5_addr_err),
            //6、错误客户机地址测试
            cmocka_unit_test(test_app_698_server_LINK_set_6_CA_err), 
            //7、错误逻辑地址测试
            cmocka_unit_test(test_app_698_server_LINK_set_7_logic_addr_err),
            //8、错误HCS帧头校验测
            cmocka_unit_test(test_app_698_server_LINK_set_8_hcs),
            //9、错误SET-Request测试
            cmocka_unit_test(test_app_698_server_LINK_set_9_err),
            //10、错误FCS帧校验测试
            cmocka_unit_test(test_app_698_server_LINK_set_10_fcs),
            //11、无结束字符测试
            cmocka_unit_test(test_app_698_server_LINK_set_11_end),

        //1.1.3 控制字测试用例
            //1、传输方向位DIR与启动标志位PRM测试
            cmocka_unit_test(test_app_698_server_LINK_ctrl_dir_prm),
            //2、分帧标志测试
            //cmocka_unit_test(test_app_698_server_LINK_ctrl_frm),
            //3、分帧传输其他测试项
            //cmocka_unit_test(test_app_698_server_LINK_ctrl_frm_other),
            //4、扰码标志位测试
            cmocka_unit_test(test_app_698_server_LINK_ctrl_sc),

        //1.2 建立应用连接测试用例
        cmocka_unit_test(test_app_698_server_CONNECT_Request),

            //1、期望的应用层协议版本号测试
            cmocka_unit_test(test_app_698_server_CONNECT_Request_1_version),
            //2、期望的协议一致性块测试
            cmocka_unit_test(test_app_698_server_CONNECT_Request_2_pro),
            //3、期望的功能一致性块测试
            cmocka_unit_test(test_app_698_server_CONNECT_Request_3_func),
            //4、客户机发送帧最大尺寸
            cmocka_unit_test(test_app_698_server_CONNECT_Request_4_tx_max),
            //5、客户机接收帧最大尺寸
            cmocka_unit_test(test_app_698_server_CONNECT_Request_5_rx_max),
            //7、客户机接收帧最大窗口尺寸
            cmocka_unit_test(test_app_698_server_CONNECT_Request_7_win_max),
            //8、客户机最大可处理APDU尺寸
            cmocka_unit_test(test_app_698_server_CONNECT_Request_8_apdu_max),
            //9、期望的应用连接超时时间
            cmocka_unit_test(test_app_698_server_CONNECT_Request_9_timeout),
            //10、认证请求对象
            cmocka_unit_test(test_app_698_server_CONNECT_Request_10_auth),

        //1.3 断开应用连接测试用例
#endif

#define GET_TEST
#ifdef GET_TEST
        //Get-Requeset数据类型测试用例
        //1、读取一个对象属性请求Get-Requeset数据类型测试 test_app_698_server_GET_Request_0501
        cmocka_unit_test(test_app_698_server_GET_Request_0501),
        //2、读取若干个对象属性请求Get-RequesetList数据类型测试 test_app_698_server_GET_Request_0502
        cmocka_unit_test(test_app_698_server_GET_Request_0502),

        //Get-Response数据类型测试用例
        //1、读取一个对象属性响应Get-Response数据类型测试 test_app_698_server_GET_Response_8501
        cmocka_unit_test(test_app_698_server_GET_Response_8501),
        //2、读取若干个对象属性响应Get-ResponseList数据类型测试 test_app_698_server_GET_Response_8502
        cmocka_unit_test(test_app_698_server_GET_Response_8502),

#endif

#define SET_TEST
#ifdef SET_TEST
        //1.6 Set-Requeset数据类型测试用例
        //1、设置一个对象属性请求Set-Requeset数据类型测试 test_app_698_server_SET_Request_0601
        cmocka_unit_test(test_app_698_server_SET_Request_0601),
        //2、设置若干个对象属性请求Set-RequesetList数据类型测试 test_app_698_server_SET_Request_0602
        cmocka_unit_test(test_app_698_server_SET_Request_0602),
        //3、设置后读取若干个对象属性SetThenGetRequestNormalList数据类型测试 test_app_698_server_SET_Request_0603
        cmocka_unit_test(test_app_698_server_SET_Request_0603),

        //Set-Responset数据类型测试用例
        //1、设置一个对象属性响应Set-Response数据类型测试 test_app_698_server_SET_Response_8601
        cmocka_unit_test(test_app_698_server_SET_Response_8601),
        //2、设置若干个对象属性响应Set-Response数据类型测试 test_app_698_server_SET_Response_8602
        cmocka_unit_test(test_app_698_server_SET_Response_8602),
        //3、设置后读取若干个对象属性响应SetThenGetResponseNormalList数据类型测试 test_app_698_server_SET_Response_8603
        cmocka_unit_test(test_app_698_server_SET_Response_8603),
#endif

#define ACTION_TEST
#ifdef ACTION_TEST
        //1.8 Action-Request数据类型测试用例
        //1、操作一个对象属性请求Action-Request数据类型测试 test_app_698_server_ACTION_Request_0701
        cmocka_unit_test(test_app_698_server_ACTION_Request_0701),
        //2、操作若干个对象属性请求Action-RequestNormalList数据类型测试 test_app_698_server_ACTION_Request_0702
        cmocka_unit_test(test_app_698_server_ACTION_Request_0702),
        //3、操作后读取若干个对象属性请求Action-ThenGetRequestNormalList数据类型测试 test_app_698_server_ACTION_Request_0703
        cmocka_unit_test(test_app_698_server_ACTION_Request_0703),
        //1.9 Action-Respons数据类型测试用例
        //1、操作一个对象属性响应Action-Response数据类型测试 test_app_698_server_ACTION_Response_8701
        cmocka_unit_test(test_app_698_server_ACTION_Response_8701),
        //2、操作若干个对象属性响应Action-ResponseNormalList数据类型测试 test_app_698_server_ACTION_Response_8702
        cmocka_unit_test(test_app_698_server_ACTION_Response_8702),
        //3、操作后读取若干个对象属性响应Action-ThenGetResponseNormalList数据类型测试 test_app_698_server_ACTION_Response_8703
        cmocka_unit_test(test_app_698_server_ACTION_Response_8703),
#endif

        //2 test_app_698_server_class_id
        //3 test_app_698_server_function
        //4 test_app_698_server_bug

        cmocka_unit_test(test_case_running_with_kernel),
    };

    //可以用下面两个函数对测试用例进行过滤
    //cmocka_set_test_filter("test_app_698_server");
    //cmocka_set_skip_filter("test_698_rx_entry");

    return cmocka_run_group_tests(tests, test_app_setup, NULL);
}
#else

int main(void)
{
    struct instance_t* instance = malloc(sizeof(struct instance_t));
    recipe.pipelines = &sim_pipelines[0];
    recipe.pipeline_n = sizeof(sim_pipelines) / sizeof(sim_pipelines[0]);
    recipe.name = 0;
    instance->recipe = &recipe;

    fifoBuf_init(&stdout_698, malloc(512), 512);
    fifoBuf_init(&stdin_698, malloc(512), 512);
    fifoBuf_init(&pipe_tx_698_req, malloc(512), 512);

    instance->apdu_cache = malloc(1800);
    instance->cache = malloc(256);
    instance->cache2 = malloc(sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);

    ctx_rx_server_698.connect_info = malloc(128);//临时
    ctx_rx_server_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_rx_server_698.apdu, malloc(2048), 2048);

    ctx_rx_server_698.apdu_rx = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_rx_server_698.apdu_rx, malloc(2048), 2048);


    ctx_tx_server_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_tx_server_698.apdu, malloc(2048), 2048);

    ctx_rx_client_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_rx_client_698.apdu, malloc(2048), 2048);

    ctx_tx_client_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_tx_client_698.apdu, malloc(2048), 2048);

    fifoBuf_init(&stdin_client_698, malloc(512), 512);
    fifoBuf_init(&stdout_client_698, malloc(512), 512);

    open_serial_port(instance, &pl_rx_from_serial, NULL);

    struct tm* p_tm;

    while (1)
    {
        time_t t = time(NULL);
        p_tm = localtime(&t);
        memcpy(&instance->time, p_tm, sizeof(struct tm));

        nidavellir_run_baremetal(instance);
    }
}
#endif