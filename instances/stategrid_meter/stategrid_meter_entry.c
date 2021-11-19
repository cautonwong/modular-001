#include "nidavellir.h"
#include "applications.h"
extern int app_rx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_tx_server_698_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int app_meter_base_entry(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int sync_data(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
extern int16_t register_event_info(uint16_t event_flag, uint32_t* addr, uint16_t* size, uint16_t* depth);
extern int baremetal_instance_entry(struct instance_t* instance);
#ifdef WIN32
extern const struct driver_ops_t  eeprom_driver;
const struct dev_info_t eeprom_dev = 
{
    0x00,0x01,0xFFFF,0x00,0x00, NULL, &eeprom_driver, NULL, NULL, NULL, NULL,".//e2prom.dat"
};
const struct dev_info_t nor_flash_dev = 
{
    0x01,0x01,0xFFFF,0x00,0x00, NULL, NULL, NULL, NULL, NULL, NULL,".//flash.dat"
};
#endif // WIN32

const struct filter_t fr_resource_manager[] =
{
    {0x01,sync_data,0,13},
};

struct pipeline_t pl_resource_manager =
{
    pid_resource_manager,
    "app_resource_manager",
    NULL,
    NULL,
    NULL,
    NULL,
    sizeof(fr_resource_manager) / sizeof(fr_resource_manager[0]),
    fr_resource_manager,
};

struct filter_t fr_meter_base[] =
{
    {0,app_meter_base_entry,1,13},
};

struct pipeline_t pl_meter_base =
{
    pid_meter_base,
    "app_meter_base",
    NULL,
    NULL,
    NULL,
    app_meter_base_entry,
    sizeof(fr_meter_base) / sizeof(fr_meter_base[0]),
    fr_meter_base,
};

const struct filter_t fr_rx_698[1] =
{
    {0x01,app_rx_server_698_entry,0,13},
};

struct pipeline_t pl_rx_698 =
{
    pid_rx_698,
    "pl_rx_698",
    NULL,
    NULL,//serial driver isr put byte into this 
    NULL,//pl_rx_698 put apdu into this 
    app_rx_server_698_entry,
    sizeof(fr_rx_698) / sizeof(fr_rx_698[0]),
    fr_rx_698,
};

const struct filter_t fr_tx_698[1] =
{
    {0x01,app_tx_server_698_entry,0,13},
};

struct pipeline_t pl_tx_698 =
{
    pid_tx_698,
    "pl_tx_698",
    NULL,
    NULL,
    NULL,
    app_tx_server_698_entry,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};


struct pipeline_t* pipelines[] =
{
    &pl_rx_698,
    &pl_tx_698,
    &pl_meter_base,
    &pl_resource_manager,
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

uint8_t nd_memory[20 * 1024];
extern uint32_t* add_inside_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint8_t inside_block_name, uint32_t block_size);
extern void* nd_malloc(struct instance_t* instance, uint8_t block_name, uint32_t size);
extern uint32_t* add_block(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size);
extern struct resource_map_t* register_map(struct instance_t* instance,uint8_t map_name,
    int (*fun)(struct instance_t *instance, uint32_t id,uint8_t *buf, uint8_t mode, struct resources_obj_t* obj));
extern int resource_map_698(struct instance_t* instance, uint32_t id, uint8_t* buf, uint8_t mode, struct resources_obj_t* obj);
extern struct instance_t* create_instance(uint8_t device_name, uint32_t first_addr, uint32_t size, uint32_t resource_blok_size, uint8_t resource_block_name);
extern int8_t add_device(struct instance_t* instance, uint8_t device_name,const struct dev_info_t *dev);
extern uint32_t* register_event(struct instance_t* instance, uint8_t device_name, uint8_t block_name, uint32_t block_size,
    int16_t(*fun)(uint16_t id, uint32_t *addr, uint16_t* len, uint16_t* depth));
extern void dev_init(void);
extern void irq_system_init(void);
extern void irq_transform_to_instance(struct instance_t* instance);

int baremetal_instance_entry_main(int argc, char** argv)
{
    struct instance_t* instance = NULL;
    struct recipe_t *recipe = NULL;
    struct context_698_t* ctx_698 = NULL;

    instance = create_instance(RES_DEV_RAM, (uint32_t)&nd_memory, 20 * 1024, 2 * 1024, 0x01);//用资源描述创建instance
    if (instance == NULL)
    {
        return -1;
    }

    instance->recipe = nd_malloc(instance, instance->resource_block_name, sizeof(struct recipe_t));
    recipe = instance->recipe;
    if (recipe == NULL)
    {
        return -1;
    }
    recipe->pipelines = &pipelines[0];
    recipe->pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe->name = 0;

    //初始化pl_resource_manager
    pl_resource_manager.in_queue = nd_malloc(instance, instance->resource_block_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_resource_manager.in_queue, nd_malloc(instance, instance->resource_block_name, 256), 256);

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
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_cache, 500))
    {
        return -1;
    }
    //指向cache space
    instance->cache = nd_malloc(instance, control_block_cache, 256);
    instance->cache2 = nd_malloc(instance, control_block_cache, sizeof(struct cache_t));
    init_cache(instance->cache2, instance->cache, 256);
    
    instance->event_flags = nd_malloc(instance,control_block_cache,event_flags_group_size);
    memset(instance->event_flags, 0, event_flags_group_size);
    instance->event_group_size = event_flags_group_size;//初始化软时钟

    uint8_t* buf = nd_malloc(instance, control_block_cache, 4);
    fifoBuf_init(&instance->pipe_rtc, buf, 4);

    /////////////////////////////////////申请一个698应用的BLOCK = 03/////////////////////////////////////////////////
    uint8_t control_block_698_name = 0x03;
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_698_name, 8 * 1024))
    {
        return -1;
    }
    //初始化pl_rx_698初始化
    pl_rx_698.ctx = nd_malloc(instance, control_block_698_name, sizeof(struct context_698_t));
    ctx_698 = pl_rx_698.ctx;
    ctx_698->apdu = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(ctx_698->apdu, nd_malloc(instance, control_block_698_name, 512), 512);
    pl_rx_698.in_queue = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_rx_698.in_queue, nd_malloc(instance, control_block_698_name, 512), 512);
    pl_rx_698.out_queue = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_rx_698.out_queue, nd_malloc(instance, control_block_698_name, 512), 512);

    //初始化pl_tx_698初始化
    pl_tx_698.ctx = ctx_698;
    pl_tx_698.in_queue = nd_malloc(instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_tx_698.in_queue, nd_malloc(instance, control_block_698_name, 512), 512);
    pl_tx_698.out_queue = pl_rx_698.out_queue;
    
    communication_chlx_init(pl_tx_698.out_queue , pl_rx_698.in_queue , UART_RS4851_CHL);
    communication_chlx_init(pl_tx_698.out_queue , pl_rx_698.in_queue , UART_MODULE_CHL);
    communication_chlx_init(pl_tx_698.out_queue , pl_rx_698.in_queue , UART_IR_CHL);

    ////////////////////////////申请一个mbase应用的BLOCK = 04/////////////////////////////////////////////
    uint8_t mbase_control_block_name = 0x04;
    if (NULL == add_block(instance, RES_DEV_RAM, mbase_control_block_name, 8 * 1024))
    {
        return -1;
    }
    //初始化pl_meter_base初始化
    pl_meter_base.ctx = nd_malloc(instance, mbase_control_block_name, sizeof(struct mbase_control_block_t));
    pl_meter_base.in_queue = nd_malloc(instance, mbase_control_block_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_meter_base.in_queue, nd_malloc(instance, mbase_control_block_name, 256), 256);
    pl_meter_base.out_queue = NULL;
    //mbase分三个区
    //mbase_block_static
    uint8_t mbase_block_static = 0x05;
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_start) - offsetof(struct mbase_control_block_t, static_section_end);
    uint32_t static_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_static, mbase_block_static_size);
    //mbase_block_cache
    uint8_t mbase_block_cache = 0x06;
    size_t mbase_block_cache_size = offsetof(struct mbase_control_block_t, cache_section_start) - offsetof(struct mbase_control_block_t, cache_section_end);
    uint32_t cache_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_cache, mbase_block_cache_size);
    //mbase_block_persistent
    uint8_t mbase_block_persistent = 0x07;
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_start) - offsetof(struct mbase_control_block_t, persistent_section_end);
    uint32_t persistent_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_persistent, mbase_block_persistent_size);

    //注册EPROM  
    add_device(instance, RES_DEV_EEPROM, &eeprom_dev);
    //注册FLASH
    add_device(instance, RES_DEV_FLASH, &nor_flash_dev);
    //申请一块掉电保存的EPROM区域
    uint8_t mbase_block_persistent_eprom = 0x08;
    add_block(instance, RES_DEV_EEPROM, mbase_block_persistent_eprom, mbase_block_persistent_size);
    //注册事件
    uint8_t mbase_block_event = 0x09;
    register_event(instance, RES_DEV_EEPROM, mbase_block_event, 50 * 1024, register_event_info);

    //注册一个698的MAP
    uint8_t res_map_698 = 0x01;
    register_map(instance, res_map_698, resource_map_698);
#ifdef WIN32
    init_timer(instance);
#endif
//    uint8_t s_buf[100];
//    nd_get(instance, 0x20000000, s_buf, 0x00);
    return baremetal_instance_entry(instance);
}

#include "log.h"

int main(int argc, char** argv)
{
#ifndef WIN32
    mcu_init(NULL);
    mcu_main_power_on_config();
    ndv_timer_init();
    //初始化硬件
    dev_init();

    //初始化固件
    irq_system_init(); 
#endif // !WIN32
#ifdef WIN32
    uint8_t buf[1024] = { 0,1,2,3,4 };
    log_trace("Hello %s", "world");
    log_info("Hello %s", "world");
    log_hex(buf, sizeof(buf));
    log_trace("Hello %s", "world");
    log_info("Hello %s", "world");
    eeprom_dev.dev_driver_ops->dev_initialize(&eeprom_dev);
#endif
    baremetal_instance_entry_main(argc, argv);
}

