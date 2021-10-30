#include "nidavellir.h"

struct fifo_t stdin_698;
struct fifo_t pipe_tx_698_req;
struct fifo_t stdout_698;
struct context_698_t ctx_698;

struct pipeline_t* pipelines[] =
{
    &pl_rx_698,
    &pl_tx_698,
    //#ifdef NIDAVELLIR_SYSTEM_TEST
    &pl_tx_to_serial,
    //#endif
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
    int8_t(*fun)(struct instance_t* instance, uint32_t id, uint8_t mode, struct resources_obj_t* obj));
extern int8_t resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern struct instance_t* create_instance(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size, uint8_t resource_block_name);
int baremetal_instance_entry_main(int argc, char** argv)
{
    uint8_t mbase_control_block_name;
    //初始化硬件
    //初始化固件
    //一、初始化resource_manager_ctx;（申请了1K的大小，名称是0x01）
    struct instance_t* instance = create_instance(0x01, &nd_memory, 32 * 1024, 2 * 1024, 0x01);//用资源描述创建instance
    if (instance == NULL)
    {
        return -1;
    }
    memset(instance, 0, sizeof(struct instance_t));
    instance->ram_device_name = 0x01;
    instance->resource_block_name = 0x01;
#if 0
    //初始化软件  task的创建方式有两种:静态创建和动态创建
    uint8_t byte_pool[40*1024];
    byte_pool_create(byte_pool);//给task创建一个字节池
    struct task_t task_meterbase;
    //分别是task的ctrl_block的三段的大小  三段:上电清零段+上电恢复但掉电不保存段+掉电保存且上电恢复段
    create_block_from_e2prom();
    create_task(&task_meterbase,0x01,300,300,300);//创建一个task
    set_task_std_in();//设置task的标准输入
    set_task_std_out();//设置task的标准输出
    set_task_setting();//设置task的设置参数
    set_task_ctrl_block();//设置task的控制块
    add_task(instance);//将此task添加到容器里
#endif
    //每个application的control block如何初始化?且如何绑定到instance里?
    //每个application如何访问自己的control block?
    //链接和绑定硬件 固件 和 软件
    //运行主循环

    struct recipe_t recipe;
    recipe.pipelines = &pipelines[0];
    recipe.pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe.name = 0;

    instance->recipe = &recipe;
    instance->ms = 0;
    //注册一个MAP
    register_map(instance, 0x01, resource_map_698);
    //申请一个BLOCK
    mbase_control_block_name = 0x02;
    if (0 > add_block(instance, 0x01, mbase_control_block_name, 8 * 1024))
    {
        return -1;
    }
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
    fifoBuf_init(&stdout_698, nd_malloc(instance, mbase_control_block_name, 512), 512);
    fifoBuf_init(&stdin_698, nd_malloc(instance, mbase_control_block_name, 512), 512);
    fifoBuf_init(&pipe_tx_698_req, nd_malloc(instance, mbase_control_block_name, 512), 512);
    extern int open_serial_port(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    open_serial_port(instance, &pl_rx_from_serial, NULL);
    //#endif
    //指向cache space
    instance->cache = nd_malloc(instance, mbase_control_block_name, 256);
    instance->cache2 = nd_malloc(instance, mbase_control_block_name, sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    uint8_t* buf = nd_malloc(instance, mbase_control_block_name, 2048);
    ctx_698.apdu = nd_malloc(instance, mbase_control_block_name, sizeof(struct fifo_t));
    fifoBuf_init(ctx_698.apdu, buf, 2048);

    buf = nd_malloc(instance, mbase_control_block_name, 4);
    fifoBuf_init(&instance->pipe_rtc, buf, 4);

    return baremetal_instance_entry(instance);
}


int app_main(int argc, char** argv)
{
    baremetal_instance_entry_main(argc, argv);
}

