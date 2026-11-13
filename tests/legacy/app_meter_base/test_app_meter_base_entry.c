#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "testutil/cmocka.h"
#include <stdio.h>
#include "cpu_surport.h"
#include "application/apps/app_entry.h"
#include "kernel/legacy_control_block.h"
extern int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

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
struct fifo_t meterbase_std_in;
struct mbase_control_block_t mbase_ctrl_block;
struct filter_t fr_meter_base[] =
{
    {0,app_meter_base_entry,1,13},
};

struct pipeline_t pl_meter_base =
{
    2,
    "app_meter_base",
    & mbase_ctrl_block,
    & meterbase_std_in,
    NULL,
    sizeof(fr_meter_base) / sizeof(fr_meter_base[0]),
    fr_meter_base,
};

//本app包含的pipeline
struct pipeline_t* pipelines[] =
{
    &pl_meter_base,
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
    fifoBuf_init(&meterbase_std_in, malloc(512), 512);
    instance->cache = malloc(256);
    instance->cache2 = malloc(sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    instance->event_flags = malloc(event_flags_group_size);
    memset(instance->event_flags, 0, event_flags_group_size);
    instance->event_group_size = event_flags_group_size;
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
    mbase_ctrl_block.Ia = 5000;
    while (1)
    {
        app_meter_base_entry(instance, &pl_meter_base,&mesg);
    }
}

//这是第3个测试用例  示例--跟kernel一起测试
void test_case_running_with_kernel(void** state)
{
    struct instance_t* instance = (struct instance_t*)*state;
    
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
        cmocka_unit_test(test_case_running_by_self),
        cmocka_unit_test(test_case_running_with_kernel),
    };

    //可以用下面两个函数对测试用例进行过滤
    //cmocka_set_test_filter("test_app_698_server");
    //cmocka_set_skip_filter("test_698_rx_entry");

    return cmocka_run_group_tests(tests, test_app_setup, NULL);
}