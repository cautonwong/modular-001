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

//msta rx from meter
struct fifo_t pipe_msta_rx_from_meter;//rx fifo
struct fifo_t pipe_msta_tx_to_meter;//tx fifo
struct context_645_t ctx_msta_rx_from_meter;//rx ctx
struct pipeline_t pl_msta_rx_from_meter;
struct fifo_t pipe_msta_tx_to_meter_req;
struct pipeline_t pl_msta_tx_to_meter;

//msta rx from bt
struct fifo_t pipe_msta_rx_from_bt;//rx fifo
struct fifo_t pipe_msta_tx_to_bt;//tx fifo
struct context_645_t ctx_msta_rx_from_bt;//rx ctx
struct pipeline_t pl_msta_rx_from_bt;

//msta tx to bt
struct fifo_t pipe_msta_tx_to_bt_req;
struct pipeline_t pl_msta_tx_to_bt;

struct context_645_t ctx_msta_proxy_to_bt;
struct context_645_t ctx_proxy;

//msta rx from sta
struct fifo_t pipe_msta_rx_from_sta;//rx fifo
struct fifo_t pipe_msta_tx_to_sta;//tx fifo
struct context_645_t ctx_msta_rx_from_sta;//rx ctx
struct pipeline_t pl_msta_rx_from_sta;

//msta tx to sta
struct fifo_t pipe_msta_tx_to_sta_req;
struct pipeline_t pl_msta_tx_to_sta;

//msta tx to bt
struct fifo_t pipe_msta_tx_to_bt_req;
struct pipeline_t pl_msta_tx_to_bt;

//meter rx from msta
struct fifo_t pipe_meter_rx_from_msta;//rx fifo
struct fifo_t pipe_meter_tx_to_msta;//tx fifo
struct context_645_t ctx_meter_rx_from_msta;//rx ctx
struct pipeline_t pl_meter_rx_from_msta;

//meter tx to msta //模拟基表
struct fifo_t pipe_meter_tx_to_msta_req;
struct pipeline_t pl_meter_tx_to_msta;

//sta rx from msta
struct fifo_t pipe_sta_rx_from_msta;//rx fifo
struct fifo_t pipe_sta_tx_to_msta;//tx fifo
struct context_645_t ctx_sta_rx_from_msta;//rx ctx
struct pipeline_t pl_sta_rx_from_msta;

//sta tx to msta
struct fifo_t pipe_sta_tx_to_msta_req;
struct pipeline_t pl_sta_tx_to_msta;

//bt rx from msta
struct fifo_t pipe_bt_rx_from_msta;//rx fifo
struct fifo_t pipe_bt_tx_to_msta;//tx fifo
struct context_645_t ctx_bt_rx_from_msta;//rx ctx
struct pipeline_t pl_bt_rx_from_msta;

//bt tx to msta
struct fifo_t pipe_bt_tx_to_msta_req;
struct pipeline_t pl_bt_tx_to_msta;
struct pipeline_t pl_kernel;

struct fifo_t stdin_698;
struct fifo_t pipe_tx_698_req;
struct fifo_t stdout_698;
struct context_698_t ctx_698;
struct pipeline_t pl_rx_dlms;
struct pipeline_t pl_tx_dlms;

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

int update_instance_rtc(struct instance_t* instance)
{
    uint32_t ts;
    ts = rtc_mktime(&instance->time);
    ts += 1;
    rtc_localtime(ts, &instance->time);
    return 0;
}

int baremetal_instance_entry(int argc, char** argv)
{
    //初始化硬件
    //初始化固件
    //初始化软件
        //每个application的control block如何初始化?且如何绑定到instance里?
        //每个application如何访问自己的control block?
    //链接和绑定硬件 固件 和 软件
    //运行主循环
 
    struct recipe_t recipe;
    recipe.layout = memories;
    recipe.layout_n = sizeof(memories) / sizeof(struct layout_t);
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines)/sizeof(pipelines[0]);
    recipe.name = 0;

    struct instance_t* instance = malloc(sizeof(struct instance_t));
    memset(instance, 0, sizeof(struct instance_t));
    instance->recipe = recipe;
    instance->ms = 0;
    init_memory(instance, recipe->layout, recipe->layout_n);
    init_ipc_matrix();
    struct tm time =
    {
        .tm_year = 2021 - 1900,   /* years are counted from 1900 */
        .tm_mon = 3,                      /* 0 = January, 11 = December */
        .tm_mday = 30,
        .tm_hour = 23,
        .tm_min = 59,
        .tm_sec = 57
    };
    memcpy(&instance->time, &time, sizeof(struct tm));
    //#ifdef NIDAVELLIR_SYSTEM_TEST
    extern int open_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    open_serial_port(instance, &pl_rx_from_serial, NULL);
    //#endif
    //指向cache space
    instance->cache = malloc(256);
    instance->cache2 = malloc(sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    uint8_t* buf = malloc(2048);
    ctx_698.apdu = malloc(sizeof(struct fifo_t));
    fifoBuf_init(ctx_698.apdu, buf, 2048);

    buf = malloc(4);
    fifoBuf_init(&instance->pipe_rtc, buf, 4);

    return baremetal_loop(instance);
}

