#include "nidavellir.h"

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
struct pipeline_t *pipelines[]= 
{
    &pl_rx_dlms,
    &pl_tx_dlms,
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
//#ifdef NIDAVELLIR_SYSTEM_TEST
    &pl_tx_to_serial,
//#endif
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

int update_instance_rtc(struct instance_t* instance)
{
    uint32_t ts;
    ts = rtc_mktime(&instance->time);
    ts += 1;
    rtc_localtime(ts, &instance->time);
    return 0;
}

int main(int argc, char** argv)
{
    struct recipe_t recipe;//这个东东可以文本化
    recipe.layout = memories;
    recipe.layout_n = sizeof(memories) / sizeof(struct layout_t);
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines)/sizeof(pipelines[0]);
    recipe.name = 0;
    struct instance_t* instance = create_nidavellir(&recipe);
    instance->after_poweron = after_msta_poweron;
    add_tasklet(instance, 2, 6, tasklet_bauterate);//添加波特率自适应延期任务  重发三次  每次间隔2秒
    return nidavellir_loop(instance);//可以同时运行多实例
}
