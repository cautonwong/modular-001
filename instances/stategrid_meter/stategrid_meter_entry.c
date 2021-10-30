#include "nidavellir.h"
#include "applications.h"
extern int app_698_rx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_698_tx_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);

struct filter_t fr_meter_base[] =
{
    {0,app_meter_base_entry,1,13},
};

struct pipeline_t pl_meter_base =
{
    2,
    "app_meter_base",
    NULL,
    NULL,
    NULL,
    sizeof(fr_meter_base) / sizeof(fr_meter_base[0]),
    fr_meter_base,
};

const struct filter_t fr_rx_698[1] =
{
    {0x01,app_698_rx_entry,0,13},
};

struct pipeline_t pl_rx_698 =
{
    pid_rx_698,
    "pl_rx_698",
    NULL,
    NULL,//serial driver isr put byte into this 
    NULL,//pl_rx_698 put apdu into this 
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
    NULL,
    NULL,
    NULL,
    app_698_tx_entry,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};


struct pipeline_t* pipelines[] =
{
    &pl_rx_698,
    &pl_tx_698,
    &pl_meter_base,
};

//cautonwong  以上的内容  应该放在instance的入口函数里面去分配和初始化

int update_instance_rtc(struct instance_t* instance)
{
    uint32_t ts;
    ts = rtc_mktime(&instance->time);
    ts += 1;
    rtc_localtime(ts, &instance->time);
    return 0;
}

uint8_t nd_memory[32 * 1024];
extern void* nd_malloc(struct instance_t* instance, uint8_t block_name, uint32_t size);
extern uint32_t* add_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size);
extern struct resource_map_t* register_map(struct instance_t* instance, uint8_t map_name,
    int8_t(*fun)(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj));
extern int8_t resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern struct instance_t* create_instance(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size, uint8_t resource_block_name);
int baremetal_instance_entry_main(int argc, char** argv)
{
    struct instance_t* instance = NULL;
    struct recipe_t *recipe = NULL;
    struct context_698_t* ctx_698 = NULL;

    //初始化硬件
    //初始化固件
    //每个application的control block如何初始化?且如何绑定到instance里?
    //每个application如何访问自己的control block?
    //链接和绑定硬件 固件 和 软件
    //运行主循环
#if 0
   //初始化软件  task的创建方式有两种:静态创建和动态创建
    uint8_t byte_pool[40 * 1024];
    byte_pool_create(byte_pool);//给task创建一个字节池
    struct task_t task_meterbase;
    //分别是task的ctrl_block的三段的大小  三段:上电清零段+上电恢复但掉电不保存段+掉电保存且上电恢复段
    create_block_from_e2prom();
    create_task(&task_meterbase, 0x01, 300, 300, 300);//创建一个task
    set_task_std_in();//设置task的标准输入
    set_task_std_out();//设置task的标准输出
    set_task_setting();//设置task的设置参数
    set_task_ctrl_block();//设置task的控制块
    add_task(instance);//将此task添加到容器里
#endif
    //一、初始化resource_manager_ctx;（申请了1K的大小，名称是0x01）
    instance = create_instance(0x01, &nd_memory, 32 * 1024, 2 * 1024, 0x01);//用资源描述创建instance
    if (instance == NULL)
    {
        return -1;
    }
    instance->ram_device_name = 0x01;
    instance->resource_block_name = 0x01;

    instance->recipe = nd_malloc(instance, instance->resource_block_name, sizeof(struct recipe_t));
    recipe = instance->recipe;
    if (recipe == NULL)
    {
        return -1;
    }
    recipe->pipelines = &pipelines[0];
    recipe->pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe->name = 0;
    //初始化时间
    instance->ms = 0;
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

    ///////////////////////////////申请一个cache的BLOCK = 02/////////////////////////////////////////////
    uint8_t control_block_cache = 0x02;
    if (0 > add_block(instance, 0x01, control_block_cache, 500))
    {
        return -1;
    }
    //指向cache space
    instance->cache = nd_malloc(instance, control_block_cache, 256);
    instance->cache2 = nd_malloc(instance, control_block_cache, sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);

    //初始化软时钟
    uint8_t* buf = nd_malloc(instance, control_block_cache, 4);
    fifoBuf_init(&instance->pipe_rtc, buf, 4);

    /////////////////////////////////////申请一个698应用的BLOCK = 03/////////////////////////////////////////////////
    uint8_t control_block_698_name = 0x03;
    if (0 > add_block(instance, 0x01, control_block_698_name, 8 * 1024))
    {
        return -1;
    }
    //初始化pl_rx_698初始化
    pl_rx_698.ctx = nd_malloc(instance, control_block_698_name, sizeof(struct context_698_t));
    ctx_698 = pl_rx_698.ctx;
    fifoBuf_init(ctx_698->apdu, nd_malloc(instance, control_block_698_name, 2048), 2048);
    pl_rx_698.in_queue = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_rx_698.in_queue, nd_malloc(instance, control_block_698_name, 512), 512);
    pl_rx_698.out_queue = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_rx_698.out_queue, nd_malloc(instance, control_block_698_name, 512), 512);

    //初始化pl_tx_698初始化
    pl_tx_698.ctx = ctx_698;
    pl_tx_698.in_queue = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_tx_698.in_queue, nd_malloc(instance, control_block_698_name, 512), 512);
    pl_tx_698.out_queue = pl_rx_698.out_queue;


    ////////////////////////////申请一个mbase应用的BLOCK = 04/////////////////////////////////////////////
    uint8_t mbase_control_block_name = 0x04;
    if (0 > add_block(instance, 0x01, mbase_control_block_name, 8 * 1024))
    {
        return -1;
    }
    //初始化pl_meter_base初始化
    pl_meter_base.ctx = nd_malloc(instance, mbase_control_block_name, sizeof(struct mbase_control_block_t));
    pl_meter_base.in_queue = nd_malloc(instance, mbase_control_block_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_meter_base.in_queue, nd_malloc(instance, mbase_control_block_name, 256), 256);
    pl_meter_base.out_queue = NULL;

    //注册一个698的MAP
    register_map(instance, 0x01, resource_map_698);

    return baremetal_instance_entry(instance);
}


int main(int argc, char** argv)
{
    baremetal_instance_entry_main(argc, argv);
}

