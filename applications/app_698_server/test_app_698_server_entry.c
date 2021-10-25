#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "testutil\cmocka.h"
#include <stdio.h>
extern int app_plc_rx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_plc_tx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int open_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int close_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int fr_tx_data_to_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern struct context_serial_t* create_serial_port(char* filename);

//测试步骤指南
//1.找到被测试函数
// 比如app_698_server_rx_entry这个函数才是真正要测试验证的函数
//2.找到桩函数   
//  比如资源管理器api
//3.打桩桩函数  打桩方法见test_case_1
//注意:
//1.测试工程跟app工程分开  

//桩函数  测试时不能依赖resource manager
//在调用nd_get之前先调用以下代码打桩：
//uint8_t* mock_data[];
//will_return(nd_get, mock_data);
//will_return(nd_get, sizeof(mock_data);
int8_t nd_get(struct instance_t* instance, uint32_t id, uint8_t* buf, uint16_t buf_size)
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
    {0x01,app_plc_rx_entry,0,13},
};

struct pipeline_t pl_rx_698 =
{
    pid_rx_698,
    "pl_rx_698",
    &ctx_698,
    &stdin_698,//serial driver isr put byte into this 
    &stdout_698,//pl_rx_698 put apdu into this 
    app_plc_rx_entry,
    sizeof(fr_rx_698) / sizeof(fr_rx_698[0]),
    fr_rx_698,
};

const struct filter_t fr_tx_698[1] =
{
    {0x01,app_plc_tx_entry,0,13},
};

struct pipeline_t pl_tx_698 =
{
    pid_tx_698,
    "pl_tx_698",
    &ctx_698,
    &pipe_tx_698_req,
    &stdout_698,
    app_plc_tx_entry,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};

struct fifo_t stdin_698;
struct fifo_t pipe_tx_698_req;
struct fifo_t stdout_698;
struct context_698_t ctx_698;
struct context_serial_t ctx_serial_1 = { L"\\\\.\\com5",NULL};

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

//本app包含的pipeline
struct pipeline_t* pipelines[] =
{
    &pl_rx_698,
    &pl_tx_698,
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
    uint8_t* buf = malloc(2048);
    ctx_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_698.apdu, buf, 2048);

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
    will_return(nd_get, &data);
    will_return(nd_get, 4);
    int r = nd_get(instance, id, buf, 4);
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
        app_plc_rx_entry(instance, &pl_rx_698,&mesg);
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

int main(void)
{
    //这是所有的测试用例列表
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_case_1),
        //cmocka_unit_test(test_case_running_by_self),
        cmocka_unit_test(test_case_running_with_kernel),
    };

    //可以用下面两个函数对测试用例进行过滤
    //cmocka_set_test_filter("test_app_698_server");
    //cmocka_set_skip_filter("test_698_rx_entry");

    return cmocka_run_group_tests(tests, test_app_setup, NULL);
}