#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
#include "fr_698.h"

#include "test_app_698_server_uniformity.h"

extern int app_698_rx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_698_tx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
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

//准备测试环境
const struct filter_t fr_rx_698[1] =
{
    {0x01,app_698_rx_entry,0,13},
};

struct pipeline_t pl_rx_698 =
{
    pid_rx_698,
    "pl_rx_698",
    &ctx_698,
    &stdin_698,//serial driver isr put byte into this 
    &stdout_698,//pl_rx_698 put apdu into this 
    app_698_rx_entry,
    sizeof(fr_rx_698) / sizeof(fr_rx_698[0]),
    fr_rx_698,
};

const struct filter_t fr_tx_698[1] =
{
    {0x01,app_698_tx_entry,0,13},
};

struct pipeline_t pl_tx_698 =
{
    pid_tx_698,
    "pl_tx_698",
    &ctx_698,
    &pipe_tx_698_req,
    &stdout_698,
    app_698_tx_entry,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};

struct fifo_t stdin_698;
struct fifo_t pipe_tx_698_req;
struct fifo_t stdout_698;

struct fifo_t stdin_client_698;
struct fifo_t stdout_client_698;

struct context_698_t ctx_698;
struct context_698_t ctx_client_698;

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


const struct filter_t plc_tx_to_meter[1] =
{
    {0x01,app_698_tx_entry,0,13},
};

struct pipeline_t pl_plc_tx_to_meter =
{
    pid_client_tx_698,
    "pl_plc_tx_to_meter",
    &ctx_client_698,
    &stdout_client_698,
    &stdin_698,
    app_698_tx_entry,
    sizeof(plc_tx_to_meter) / sizeof(plc_tx_to_meter[0]),
    plc_tx_to_meter,
};

const struct filter_t plc_rx_from_meter[1] =
{
    {0x01,app_698_rx_entry,0,13},
};

struct pipeline_t pl_plc_rx_from_meter =
{
    pid_client_rx_698,
    "pl_plc_rx_from_meter",
    &ctx_client_698,//
    &stdout_698,//
    NULL,//
    app_698_rx_entry,
    sizeof(plc_rx_from_meter) / sizeof(plc_rx_from_meter[0]),
    plc_rx_from_meter,
};



//本app包含的pipeline
struct pipeline_t* pipelines[] =
{
    &pl_plc_tx_to_meter,   
    &pl_rx_698,
    &pl_tx_698,
    &pl_plc_rx_from_meter,

};

//*********************上述的初始化方法要删掉 wong*********************//
uint8_t stdout_698_server_buf[512] = { 0 };
uint8_t stdin_698_server_buf[512] = { 0 };
uint8_t stdin_698_client_buf[512] = { 0 };
uint8_t stdout_698_client_buf[512] = { 0 };

struct recipe_t recipe;
int test_app_setup(void** state)
{
    struct instance_t* instance = malloc(sizeof(struct instance_t));
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe.name = 0;
    instance->recipe = &recipe;
    //fifoBuf_init(&stdout_698, malloc(512), 512);
    //fifoBuf_init(&stdin_698, malloc( 512), 512);
    fifoBuf_init(&stdout_698, stdout_698_server_buf, 512);
    fifoBuf_init(&stdin_698, stdin_698_server_buf, 512);
    fifoBuf_init(&pipe_tx_698_req, malloc(512), 512);
    instance->cache = malloc(256);
    instance->cache2 = malloc(sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    uint8_t* buf = malloc(2048);
    ctx_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_698.apdu, buf, 2048);

    //fifoBuf_init(&stdin_client_698, malloc(512), 512);
    //fifoBuf_init(&stdout_client_698, malloc(512), 512);
    fifoBuf_init(&stdin_client_698, stdin_698_client_buf, 512);
    fifoBuf_init(&stdout_client_698, stdout_698_client_buf, 512);

    ctx_client_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_client_698.apdu, malloc(2048), 2048);

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
        app_698_rx_entry(instance, &pl_rx_698,&mesg);
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



#if 1
int main(void)
{
    //这是所有的测试用例列表
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_case_1),

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
            cmocka_unit_test(test_app_698_server_LINK_ctrl_frm),
            //3、分帧传输其他测试项
            cmocka_unit_test(test_app_698_server_LINK_ctrl_frm_other),
            //4、扰码标志位测试
            cmocka_unit_test(test_app_698_server_LINK_ctrl_sc),

        //1.2 建立应用连接测试用例
        //1.3 断开应用连接测试用例
        //1.4 Get-Requeset数据类型测试用例
        //1.5 Get-Response数据类型测试用例
        //1.6 Set-Requeset数据类型测试用例
        //1.7 Set-Responset数据类型测试用例
        //1.8 Action-Request数据类型测试用例
        //1.9 Action-Respons数据类型测试用例

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
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe.name = 0;
    instance->recipe = &recipe;

    fifoBuf_init(&stdout_698, malloc(512), 512);
    fifoBuf_init(&stdin_698, malloc(512), 512);
    fifoBuf_init(&pipe_tx_698_req, malloc(512), 512);

    instance->cache = malloc(256);
    instance->cache2 = malloc(sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    uint8_t* buf = malloc(2048);
    ctx_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_698.apdu, buf, 2048);

    open_serial_port(instance, &pl_rx_from_serial, NULL);

    while (1)
    {
        nidavellir_run_baremetal(instance);
    }
}
#endif