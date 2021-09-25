#include "nidavellir.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include <stdio.h>

const struct layout_t memories[] =
{
    {//area_id == 0
        0x0000,
        0x400,//1K for work_queue
        0xdeadbeef,
        0xbeefdead,
    },
    {
        0x400,
        0x200,//512 for rt_root
        0xbeefdead,
        0xdeadbeef,
    },
    {
        0x600,
        0x200,//512 for workers
        0xdeadbeef,
        0xbeefdead,
    },
    {
        0x600,
        0x200,//1k for workers data
        0xdeadbeef,
        0xbeefdead,
    },
};

/*
管道和重定向的使用
==================
shell script
------------------
rtc | msta_tx_to_meter | meter_rx_from_msta| msta_rx_from_meter|rtc
rtc | msta_tx_to_meter | meter_rx_from_msta| msta_rx_from_meter > cache
rtc | msta_tx_to_meter | meter_rx_from_msta| msta_rx_from_meter > persistent data

bt_tx_to_msta | msta_rx_from_bt 

rtc|msta_tx_to_meter 
msta_rx_from_meter|rtc
*/
extern struct pipeline_t pl_rx_dlms;
struct pipeline_t *pipelines[]= 
{
//    &pl_rx_698,
  //  &pl_tx_698,
    & pl_rx_dlms,
    &pl_msta,
    &pl_sta_tx_to_msta,
    &pl_meter_tx_to_msta,
    &pl_bt_tx_to_msta,
    &pl_meter_rx_from_msta,
    &pl_bt_rx_from_msta,
    &pl_msta_rx_from_sta,
    &pl_sta_rx_from_msta,
    &pl_msta_rx_from_meter,
    &pl_msta_tx_to_meter,
    &pl_msta_tx_to_sta,
    &pl_msta_tx_to_bt,
    &pl_msta_rx_from_bt,
    &pl_tx_to_serial,
};

int after_msta_poweron(struct instance_t* instance)
{
    return 0;
}
int tasklet_bauterate(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint32_t bauterates[] = { 2400,4800,19200,38400,115200,9600 };
    struct rtc_context_t* ctx = (struct rtc_context_t*)pipeline->ctx;

    //修改跟基表之间的串口波特率  前提是上电时  已经以9600打开该串口了
    //set_bauterate(0, bauterates[ctx->bauterate]);
    ctx->bauterate++;
    if (ctx->bauterate >= sizeof(bauterates) / sizeof(uint32_t))
    {
        ctx->bauterate = 0;
    }

    mesg->ctrl = 0x13;
    mesg->id = 0x04000401;
    mesg->len = 4;
    mesg->to = msta_tx_to_meter;
    send_mesg(instance, pipeline, mesg, NULL);
    return 0;
}

//测试用例开始

struct recipe_t recipe;
int update_instance_rtc(struct instance_t* instance)
{
    uint32_t ts;
    ts = rtc_mktime(&instance->time);
    ts += 1;
    rtc_localtime(ts, &instance->time);
    ts = rtc_mktime(&instance->meter_rtc);
    ts += 1;
    rtc_localtime(ts, &instance->meter_rtc);
    ts = rtc_mktime(&instance->cco_tm);
    ts += 1;
    rtc_localtime(ts, &instance->cco_tm);
    return 0;
}

int setup(void** state)
{
    recipe.layout = memories;
    recipe.layout_n = sizeof(memories) / sizeof(struct layout_t);
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe.name = 0;
    struct instance_t* instance;
    instance = create_nidavellir(&recipe);
    add_tasklet(instance, 2, 6, tasklet_bauterate);//添加波特率自适应延期任务  重发三次  每次间隔2秒

    *state = instance;
    return 0;
}

extern void test_meter_tx_86_to_msta(void** state);
extern void test_msta_tx_86_to_sta(void** state);
extern void test_bt_tx_11_to_msta(void** state);
extern void test_msta_tx_11_to_meter(void** state);
extern void test_sta_tx_11_to_msta(void** state);
extern void test_sta_tx_07_to_meter(void** state);
extern void test_msta_read_meter_data_periodicly(void** state);
extern void demo_how_to_use_cmocka(void** state);
extern void integration_test_sync_time_when_poweron(void** state);
extern void test_tasklet(void** state);
extern void test_upgrade(void** state);
extern void test_698_rx_entry(void** state);
extern void test_dlms_rx_entry(void** state);
extern void test_msta_tx_91_to_bt(void** state);

//todo
// 1. [x] 通信地址的读取
// 2. [x] 波特率的切换  
// 3. [] 07控制码透传
// 4. [x] 14控制码升级
// 5. [] 11透传
// 6. [x] 超时机制(用于超时重发和事件延时判断)
// 7. [] 数据(费率电能 负荷 冻结 事件)的处理和读取和上报
// 8. [] 多ID抄读
// 9. [] 过滤不支持的报文
// 10. [] 长报文的处理
// 11. [] 645解析器改造
// 12. [x] windows串口

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlms_rx_entry),//测试698报文的解析
        cmocka_unit_test(test_msta_tx_91_to_bt),//测试msta发送数据给蓝牙
        cmocka_unit_test(test_698_rx_entry),//测试698报文的解析
        cmocka_unit_test(demo_how_to_use_cmocka),
        cmocka_unit_test(test_bt_tx_11_to_msta),//测试蓝牙抄读msta的数据
        cmocka_unit_test(test_msta_tx_86_to_sta),//测试msta主动上报数据给sta
        cmocka_unit_test(test_meter_tx_86_to_msta),//测试电表主动上报数据给msta
        cmocka_unit_test(test_msta_tx_11_to_meter),//测试msta抄读电表数据
        //cmocka_unit_test(test_sta_tx_07_to_meter),//测试sta透传数据给meter
        cmocka_unit_test(test_sta_tx_11_to_msta),//测试sta抄读msta的数据
        cmocka_unit_test(test_msta_read_meter_data_periodicly),//测试按周期抄读高频数据 
        cmocka_unit_test(integration_test_sync_time_when_poweron),//时钟获取,判断和对时
        cmocka_unit_test(test_upgrade),//测试固件升级
        cmocka_unit_test(test_tasklet),//测试延期任务的写法
    };
    cmocka_set_test_filter("test_dlms_rx_entry");
    //cmocka_set_skip_filter("test_698_rx_entry");
    return cmocka_run_group_tests(tests, setup, NULL);
}

