#include "nidavellir.h"
#include "applications.h"
#include "resource_tool.h"



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

struct pipeline_t pl_plc_rx_698 =
{
    pid_plc_rx_698,
    "pl_plc_rx_698",
    NULL,
    NULL,
    NULL,
    app_rx_server_698_entry,
    sizeof(fr_rx_698) / sizeof(fr_rx_698[0]),
    fr_rx_698,
};

struct pipeline_t pl_ir_rx_698 =
{
    pid_ir_rx_698,
    "pl_ir_rx_698",
    NULL,
    NULL,
    NULL,
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

struct pipeline_t pl_plc_tx_698 =
{
    pid_plc_tx_698,
    "pl_plc_tx_698",
    NULL,
    NULL,
    NULL,
    app_tx_server_698_entry,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};

struct pipeline_t pl_ir_tx_698 =
{
    pid_ir_tx_698,
    "pl_ir_tx_698",
    NULL,
    NULL,
    NULL,
    app_tx_server_698_entry,
    sizeof(fr_tx_698) / sizeof(fr_tx_698[0]),
    fr_tx_698,
};

const struct filter_t fr_lowpower[1] =
{
    {0x01,app_meter_lowpower_wakeup_entry,0,13},
};

struct pipeline_t pl_lowpower_wakeup =
{
    pid_lowpower_wakeup,
    "pl_lowpower_wakeup",
    NULL,
    NULL,
    NULL,
    app_meter_lowpower_wakeup_entry,
    sizeof(fr_lowpower) / sizeof(fr_lowpower[0]),
    fr_lowpower,
};

struct pipeline_t* pipelines[] =
{
    &pl_rx_698,
    &pl_tx_698,
    &pl_plc_rx_698,
    &pl_plc_tx_698,
    &pl_ir_rx_698,
    &pl_ir_tx_698,
    &pl_meter_base,
    &pl_resource_manager,
    &pl_lowpower_wakeup,
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

int communication_channel_init(struct instance_t* p_instance, struct pipeline_t *pl_rx, struct pipeline_t *pl_tx, uint8_t chl_type)
{
    struct context_698_t* ctx_698 = NULL;
    
    //初始化pl_rx_698初始化
    pl_rx->ctx = nd_malloc(p_instance, control_block_698_name, sizeof(struct context_698_t));
    ctx_698 = pl_rx->ctx;
#if 1
    ctx_698->apdu = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(ctx_698->apdu, nd_malloc(p_instance, control_block_698_name, Max698_apdu), Max698_apdu);
    ctx_698->apdu_rx = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(ctx_698->apdu_rx, nd_malloc(p_instance, control_block_698_name, Max698_apdu_rx), Max698_apdu_rx);
#endif
    
    pl_rx->in_queue = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_rx->in_queue, nd_malloc(p_instance, control_block_698_name, Max698len), Max698len);
    pl_rx->out_queue = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_rx->out_queue, nd_malloc(p_instance, control_block_698_name, Max698len), Max698len);

    //初始化pl_tx_698初始化
    pl_tx->ctx = ctx_698;
    pl_tx->in_queue = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_tx->in_queue, nd_malloc(p_instance, control_block_698_name, pl_tx_698_len), pl_tx_698_len);
    pl_tx->out_queue = pl_rx->out_queue;
    
    communication_chlx_init((struct fifo_t*)pl_tx->out_queue, (struct fifo_t*)pl_rx->in_queue, chl_type);
    return true;
}



uint8_t nd_memory[23 * 1024 + 800]; //26

int baremetal_instance_entry_main(int argc, char** argv)
{
    struct instance_t* instance = NULL;
    struct recipe_t *recipe = NULL;

    instance = create_instance(RES_DEV_RAM, (uint32_t)&nd_memory, 26 * 1024, 2 * 1024, resource_block_name);//用资源描述创建instance
    if (instance == NULL)
    {
        return -1;
    }

    instance->recipe = nd_malloc(instance, instance->resource_block, sizeof(struct recipe_t));
    recipe = instance->recipe;
    if (recipe == NULL)
    {
        return -1;
    }
    recipe->pipelines = &pipelines[0];
    recipe->pipeline_n = sizeof(pipelines) / sizeof(pipelines[0]);
    recipe->name = 0;

    //初始化pl_resource_manager
    pl_resource_manager.in_queue = nd_malloc(instance, instance->resource_block, sizeof(struct fifo_t));
    fifoBuf_init(pl_resource_manager.in_queue, nd_malloc(instance, instance->resource_block, 256), 256);

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
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_cache, (1024*3)))
    {
        return -1;
    }
    
    instance->event_flags = nd_malloc(instance,control_block_cache,event_flags_group_size);
    memset(instance->event_flags, 0, event_flags_group_size);
    instance->event_group_size = event_flags_group_size;//初始化软时钟

    uint8_t* buf = nd_malloc(instance, control_block_cache, 4);
    fifoBuf_init(&instance->pipe_rtc, buf, 4);

    buf = nd_malloc(instance, control_block_cache, 8);
    fifoBuf_init(&instance->pipe_key, buf, 8);
    
    buf = nd_malloc(instance, control_block_cache, 8);
    fifoBuf_init(&instance->pipe_uart, buf, 8);
    instance->apdu_cache = nd_malloc(instance, control_block_cache, cache_max);
        
    /////////////////////////////////////申请一个698应用的BLOCK = 03/////////////////////////////////////////////////
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_698_name, 13 * 1024))//16
    {
        return -1;
    }
    
    communication_channel_init(instance, &pl_rx_698, &pl_tx_698, UART_RS4851_CHL);
    //communication_channel_init(instance, &pl_plc_rx_698, &pl_plc_tx_698 , UART_MODULE_CHL);
    //communication_channel_init(instance, &pl_ir_rx_698, &pl_ir_tx_698, UART_IR_CHL);

    ////////////////////////////申请一个mbase应用的BLOCK = 04/////////////////////////////////////////////
    if (NULL == add_block(instance, RES_DEV_RAM, mbase_control_block_name, 6 * 1024)) //剩余4k
    {
        return -1;
    }
    //初始化pl_meter_base初始化
    pl_meter_base.ctx = nd_malloc(instance, mbase_control_block_name, sizeof(struct mbase_control_block_t));
    pl_meter_base.in_queue = nd_malloc(instance, mbase_control_block_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_meter_base.in_queue, nd_malloc(instance, mbase_control_block_name, 256), 256);
    pl_meter_base.out_queue = NULL;
    pl_lowpower_wakeup.ctx = pl_meter_base.ctx;
    pl_lowpower_wakeup.in_queue = nd_malloc(instance, mbase_control_block_name, sizeof(struct fifo_t));
    fifoBuf_init(pl_lowpower_wakeup.in_queue, nd_malloc(instance, mbase_control_block_name, 256), 256);
    pl_lowpower_wakeup.out_queue = NULL;
    //mbase分三个区
    //mbase_block_static
    size_t mbase_block_static_size = offsetof(struct mbase_control_block_t, static_section_end) - offsetof(struct mbase_control_block_t, static_section_start);
    uint32_t static_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_static, mbase_block_static_size, 0x00);
    //mbase_block_cache
    size_t mbase_block_cache_size = offsetof(struct mbase_control_block_t, cache_section_end) -  offsetof(struct mbase_control_block_t, cache_section_start);
    uint32_t cache_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_cache, mbase_block_cache_size, mbase_block_static_size);
    //mbase_block_persistent
    size_t mbase_block_persistent_size = offsetof(struct mbase_control_block_t, persistent_section_end) - offsetof(struct mbase_control_block_t, persistent_section_start);
    uint32_t persistent_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_persistent, mbase_block_persistent_size, mbase_block_static_size + mbase_block_cache_size);

    //注册EPROM  
    add_device(instance, RES_DEV_EEPROM, &eeprom_dev,0x00);
    //注册FLASH
    add_device(instance, RES_DEV_FLASH, &nor_flash_dev,4096);
    //申请一块参数保存区的EPROM区域
    add_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, mbase_block_static_size);
    //申请一块掉电保存的EPROM区域
    add_block(instance, RES_DEV_EEPROM, mbase_block_persistent_eprom, mbase_block_persistent_size);
    //注册事件的管理区
    register_parameter(instance, RES_DEV_EEPROM, mbase_block_parameter, 20 * 1024, supply_parameter_info);
    //注册事件
    register_log(instance, RES_DEV_EEPROM, mbase_block_event, RES_DEV_EEPROM, mbase_block_parameter, 43 * 1024, supply_event_info);

    //注册冻结1
    register_log(instance, RES_DEV_FLASH, mbase_block_event_flash1, RES_DEV_EEPROM, mbase_block_parameter, 830 * 4 * 1024, supply_event_flash_1_info);

    //注册冻结2
    register_log(instance, RES_DEV_FLASH, mbase_block_event_flash2, RES_DEV_EEPROM, mbase_block_parameter, 37 * 4 * 1024, supply_event_flash_2_info);
    //申请一块参数保存区的EPROM区域
    add_block(instance, RES_DEV_FLASH, public_block_flash, 4096*2);
    //注册一个698的MAP
    register_map(instance, res_map_698, resource_map_698);
#ifdef WIN32
    init_timer(instance);
#endif
    uint32_t addr = 0;
    get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
    nd_get_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, (uint8_t*)addr, mbase_block_static_size); //将EPROM中的数据恢复到RAM;  
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

