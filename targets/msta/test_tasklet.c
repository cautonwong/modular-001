#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>
extern struct rtc_context_t msta_context;

uint8_t called_count = 0;
int test_func(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    called_count++;
    return 0;
}
extern int add_tasklet(struct instance_t* instance, uint8_t interval, uint8_t count,
    int (*deffered_work)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg));

void test_tasklet(void **state) 
{
    struct instance_t* instance;
    struct pipeline_t *pipeline;
    instance = (struct instance_t*)*state;
    init_test_environment();
    msta_context.CCO = 0;
//1. 几秒后执行某个动作
//2. 每几秒执行某个动作
//4. 可以只执行一次
 
    //可重复执行
    int tid = add_tasklet(instance, 2, 0, test_func);
    assert_true(tid == 0);
    nidavellir_loop_n_with_rtc_running(instance, 10);
    assert_true(called_count == 0);

    start_tasklet(instance, tid);
    nidavellir_loop_n_with_rtc_running(instance, 10);
    assert_true(called_count == 5);
    nidavellir_loop_n_with_rtc_running(instance, 10);
    assert_true(called_count == 10);

    //可停止 可取消  
    stop_tasklet(instance, tid);
    nidavellir_loop_n_with_rtc_running(instance, 10);
    assert_true(called_count == 10);

    called_count = 0;
    //可改变执行的动作
    tid = add_tasklet(instance, 2, 2, test_func);
    start_tasklet(instance, tid);
    nidavellir_loop_n_with_rtc_running(instance, 10);
    assert_true(called_count == 2);
    nidavellir_loop_n_with_rtc_running(instance, 10);
    assert_true(called_count == 2);
    stop_tasklet(instance, tid);
    nidavellir_run(instance);
}