#include "targets/stategrid_meter/board/uart_driver.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/board/board_io.h"
#include "application/adapters/dl698/fr_698.h"
#include "application/ports/pipeline_view.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "components/core/bus/nd_bus.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "targets/stategrid_meter/board/meter_config.h"
#include "targets/stategrid_meter/board/meter_status.h"
#include "targets/stategrid_meter/mapper_698/factory_cfg.h"
#include "targets/stategrid_meter/app/dl698_parameter_adapter.h"
#include "targets/stategrid_meter/app/platform_adapter.h"
#include "targets/stategrid_meter/app/backlight_adapter.h"
#include "targets/stategrid_meter/app/record_storage_adapter.h"
#include "targets/stategrid_meter/app/relay_status_adapter.h"
#include "targets/stategrid_meter/app/metering_input_adapter.h"
#include "targets/stategrid_meter/app/runtime_adapter.h"
#include "targets/stategrid_meter/app/timer_adapter.h"
#include "resource_tool.h"
#include "cpu_surport.h"
#include "targets/stategrid_meter/app/app_entry.h"
#include "targets/stategrid_meter/app/pids.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/app/app_config.h"
#include "targets/stategrid_meter/board/board_flash.h"
#include "infra/kernel/resource_blocks.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/driver_entry.h"
#include "targets/stategrid_meter/app/iar_linker_symbols.h"
#include "targets/stategrid_meter/mapper_698/resource_map.h"
#include "time_link.h"
#include "infra/kernel/key_config.h"
#include "components/protocol/dl698/nd_dl698.h"
#include "infra/channel/fifo_channel.h"
#include "application/adapters/dl645/fr_645_context.h"

/* The supply callbacks take void *instance; register_* declares a typed
 * instance pointer. The ABI is identical, so match it here. */
typedef int16_t (*resource_supply_fn)(stategrid_context_t *, uint16_t,
                                      uint32_t *, uint32_t *, uint32_t *);
typedef int (*resource_map_fn)(stategrid_context_t *, uint32_t, uint8_t *,
                               uint16_t, void *, uint16_t);


#ifdef ND_FIRMWARE_SIM
int nd_sim_should_stop(stategrid_context_t * instance);
void nd_sim_after_run(stategrid_context_t * instance);
void nd_sim_event(uint8_t from, uint8_t to, uint8_t ctrl, uint16_t len);
void nd_sim_filter(const char *pipeline, uint8_t filter_id, int ret);
#endif


struct mbase_control_block_t* mbase_cb_do_not_operation;//只用于仿真调试监控

/* What the pipeline container used to hold: one 698 context and the inbound
 * and outbound FIFO pair for a serial channel. */
typedef struct
{
    uint8_t id;
    struct context_698_t *ctx;
    struct fifo_t *in;
    struct fifo_t *out;
} stategrid_698_link_t;

static stategrid_698_link_t link_4851;
static stategrid_698_link_t link_plc;
static stategrid_698_link_t link_ir;

static struct context_645_t ctx_645_server;
static nd_pipeline_view_t rx645_view;


void csstack_base_init(void)
{
#ifdef ND_FIRMWARE_SIM
    return;
#else
    uint8_t *p = (uint8_t *)&CSTACK$$Base;

    char *stack_check = "stack overflow\n";

    ndv_memcopy(p,stack_check,15);
#endif
}



int update_instance_rtc(stategrid_context_t * instance)
{
    uint32_t ts;
    struct tm *time = &((stategrid_context_t *)instance)->time;

    ts = rtc_mktime(time);
    ts += 1;
    rtc_localtime(ts, time);
    return 0;
}

static nd_dl698_t *dl698_components[3];
static nd_dl698_t *dl698_plc_component;
static size_t dl698_component_count;
static nd_fifo_channel_t dl698_input_storage[3];
static nd_fifo_channel_t dl698_output_storage[3];
static nd_channel_t dl698_input_channel[3];
static nd_channel_t dl698_output_channel[3];

static uint32_t dl698_now_ms(void *context)
{
    return context != NULL ? ((stategrid_context_t *)context)->ms : 0;
}

static int stategrid_dl698_send(void *context,
                                void *pipeline,
                                nd_dl698_msg_t *mesg)
{
    (void)context;
    (void)pipeline;
    if (dl698_plc_component == NULL) {
        return -1;
    }
    return nd_dl698_send(dl698_plc_component, mesg);
}

static int communication_channel_init(stategrid_context_t * p_instance, stategrid_698_link_t *link, uint8_t chl_type)
{
    struct context_698_t* ctx_698 = NULL;
    stategrid_context_t *ctx = ((stategrid_context_t *)p_instance);

    //初始化698接收上下文
    link->ctx = nd_malloc(p_instance, control_block_698_name, sizeof(struct context_698_t));
    ctx_698 = link->ctx;

    ctx_698->apdu = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(ctx_698->apdu, nd_malloc(p_instance, control_block_698_name, Max698_apdu), Max698_apdu);

    link->in = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(link->in, nd_malloc(p_instance, control_block_698_name, Max698len), Max698len);
    link->out = nd_malloc(p_instance, control_block_698_name, sizeof(struct fifo_t));
    fifoBuf_init(link->out, nd_malloc(p_instance, control_block_698_name, Max698len), Max698len);

    if(chl_type<4){
        uint8_t* buf = nd_malloc(p_instance, control_block_cache, 8);
        fifoBuf_init(&ctx->pipe_uart[chl_type], buf, 8);
    }
    communication_chlx_init(link->out, link->in, chl_type , p_instance);

    if (dl698_component_count < 3)
    {
        size_t index = dl698_component_count;
        nd_dl698_config_t config;

        nd_fifo_channel_init(&dl698_input_channel[index],
                             &dl698_input_storage[index],
                             link->in);
        nd_fifo_channel_init(&dl698_output_channel[index],
                             &dl698_output_storage[index],
                             link->out);

        config.instance = p_instance;
        config.context = link->ctx;
        config.pid = link->id;
        config.now_ms = dl698_now_ms;
        config.input = &dl698_input_channel[index];
        config.output = &dl698_output_channel[index];
        config.context->send = stategrid_dl698_send;

        dl698_components[index] = nd_dl698_create(&config);
        if (dl698_components[index] != NULL)
        {
            if (link->id == STG_PID_PLC_RX_698)
            {
                dl698_plc_component = dl698_components[index];
            }
            dl698_component_count++;
        }
    }
    return true;
}

/* RS485-1 carries both 698 and 645. A 645 frame is 68 | 6-byte address | 68,
 * so byte 7 separates it from a 698 frame, whose length bytes occupy 1..3. */
static void dispatch_645_rs4851(stategrid_context_t * instance)
{
    struct fifo_t* irq = &((stategrid_context_t *)instance)->pipe_uart[UART_RS4851_CHL];
    struct fifo_t* rx = link_4851.in;
    nd_dl645_msg_t mesg;

    if ((rx645_view.ctx == NULL) || (fifoBuf_getUsed(irq) == 0)
        || (fifoBuf_getUsed(rx) < 8)
        || (fifoBuf_peekByteB(rx, 0) != 0x68)
        || (fifoBuf_peekByteB(rx, 7) != 0x68))
    {
        return;
    }
    fifoBuf_getByte(irq);
    memset(&mesg, 0, sizeof(mesg));
    if (fr_rx_645_entry(instance, &rx645_view, &mesg) > 0)
    {
        fr_gatekeeper_645_server(instance, &rx645_view, &mesg);
    }
}

__no_init uint8_t nd_memory[24 * 1024]; //26

static void trace_event(uint8_t to, uint8_t ctrl)
{
#ifdef ND_FIRMWARE_SIM
    nd_sim_event(STG_PID_KERNEL, to, ctrl, 0);
#else
    (void)to;
    (void)ctrl;
#endif
}

static void push_meter_event(stategrid_context_t *instance, uint8_t ctrl)
{
    fifoBuf_putByte(((stategrid_context_t *)instance)->meter_events, ctrl);
    trace_event(STG_PID_METER_BASE, ctrl);
}

static void push_lowpower_event(stategrid_context_t *instance, uint8_t ctrl)
{
    fifoBuf_putByte(((stategrid_context_t *)instance)->lowpower_events, ctrl);
    trace_event(STG_PID_LOWPOWER_WAKEUP, ctrl);
}

/* The record adapter posts here; the resource tick consumes one per pass,
 * matching the kernel's one-message-per-pass dispatch order. */
void stategrid_record_enqueue(stategrid_context_t *instance, uint16_t record_id)
{
    fifoBuf_putData(((stategrid_context_t *)instance)->records, &record_id,
                    sizeof(record_id));
    trace_event(STG_PID_RESOURCE_MANAGER, 0);
}

static void publish_event(stategrid_context_t *instance, nd_event_topic_t topic)
{
    nd_event_t event = { 0 };

    event.topic = topic;
    event.timestamp_ms = ((stategrid_context_t *)instance)->ms;
    nd_bus_publish(&event);
}

static void publish_power_event(stategrid_context_t *instance, uint8_t on)
{
    nd_event_t event = { 0 };

    event.topic = ND_EVENT_POWER_CHANGE;
    event.timestamp_ms = ((stategrid_context_t *)instance)->ms;
    event.data.power.on = on;
    nd_bus_publish(&event);
}

static void power_tick(stategrid_context_t *instance)
{
#ifndef WIN32
    uint32_t addr = 0;
    int ret = 0;
    uint16_t metertype = 0;
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);

    if (ctx->ac_power_status == AC_POWER_ON)
    {
        if (ac_power_off_monitor())
        {
            device_power_close(instance);
            PowerOff_Save_data(instance);

            ringbuffer_clear(ctx->meter_events);
            push_meter_event(instance, MSG_POWEROFF);
            ctx->ac_power_status = AC_POWER_OFF;
            publish_power_event(instance, 0);
        }
    }
    else if (ac_power_on_monitor())
    {
        addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ND_METER_TYPE_PARAM_ID);
        ret = nd_get_index(instance, addr, (uint8_t*)&metertype, 2);
        if(ret < 0)
        {
            metertype = (uint16_t)(init_Meter_Type[0] |
                                   ((uint16_t)init_Meter_Type[1] << 8));
        }
        nd_meter_status_metertype_set(instance, metertype);

        device_power_open(instance, NULL);
        ringbuffer_clear(ctx->meter_events);
        push_meter_event(instance, MSG_POWERON);
        ctx->ac_power_status = AC_POWER_ON;
        publish_power_event(instance, 1);
    }
#else
    (void)instance;
#endif
}

static void second_tick(stategrid_context_t *instance)
{
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);

    if (fifoBuf_getUsed(&ctx->pipe_rtc) > 0)
    {
        uint8_t byte = fifoBuf_getByte(&ctx->pipe_rtc);
#if WIN32
        if (byte == 1)
#else
        if (byte == IRQ_RTC)
#endif
        {
            if (ctx->ac_power_status == AC_POWER_ON)
            {
                push_meter_event(instance, MSG_SECOND);
            }
            else
            {
                push_lowpower_event(instance, MSG_SECOND);
            }
            publish_event(instance, ND_EVENT_TICK_SECOND);
        }
    }
}

static void key_tick(stategrid_context_t *instance)
{
    uint8_t keydly = 0;
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);

    if (fifoBuf_getUsed(&ctx->pipe_key) > 0)
    {
        uint8_t byte = fifoBuf_getByte(&ctx->pipe_key);
#if WIN32
        if (byte == 2)
#else
        if (byte == IRQ_PMU)
#endif
        {
            if (ctx->ac_power_status == AC_POWER_ON)
            {
                push_meter_event(instance, MSG_PAGE_KEY_UP);
            }
            else
            {
                for (keydly = 0; keydly < keydlymax; keydly++)
                {
                    if (nd_board_key_released())
                    {
                        return;
                    }
                }
                if (keydly >= keydlymax)
                {
                    push_lowpower_event(instance, MSG_PAGE_KEY_UP);
                }
            }
        }
    }
}

static void io_poll(stategrid_context_t *instance)
{
    size_t i;

    dispatch_645_rs4851(instance);
    for (i = 0; i < dl698_component_count; i++)
    {
        nd_dl698_poll(dl698_components[i]);
    }
}

static void event_tick(stategrid_context_t *instance)
{
#ifndef WIN32
    mcu_clear_watchdog();
    irq_transform_to_instance(instance);
#endif
    power_tick(instance);
    second_tick(instance);
    key_tick(instance);
}

static void meter_base_tick(stategrid_context_t *instance)
{
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);

    if (fifoBuf_getUsed(ctx->meter_events) > 0)
    {
        uint8_t ctrl = fifoBuf_getByte(ctx->meter_events);
        int ret = app_meter_base_event(instance, ctrl);

#ifdef ND_FIRMWARE_SIM
        nd_sim_filter("app_meter_base", 0x00, ret);
#endif
    }
}

static void resource_tick(stategrid_context_t *instance)
{
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);
    uint16_t record_id;

    if (fifoBuf_getUsed(ctx->records) >= sizeof(record_id))
    {
        fifoBuf_getData(ctx->records, &record_id, sizeof(record_id));
        sync_data_event(instance, record_id);
#ifdef ND_FIRMWARE_SIM
        nd_sim_filter("app_resource_manager", 0x01, 0);
#endif
    }
}

static void lowpower_tick(stategrid_context_t *instance)
{
    stategrid_context_t *ctx = ((stategrid_context_t *)instance);

    if (fifoBuf_getUsed(ctx->lowpower_events) > 0)
    {
        uint8_t ctrl = fifoBuf_getByte(ctx->lowpower_events);
        int ret = app_meter_lowpower_event(instance, ctrl);

#ifdef ND_FIRMWARE_SIM
        nd_sim_filter("pl_lowpower_wakeup", 0x01, ret);
#endif
    }
}

/* Waits for the PLC transmitter to drain before the report is cleared. The
 * bounds are the kernel loop's: 0x4FFFF watchdog-cleared polls, then a short
 * busy delay so the last byte leaves the shift register. */
static void report_poll(stategrid_context_t *instance)
{
    (void)instance;
    if (nd_fr698_report_pending())
    {
        uint32_t tick_report = 0x4FFFF;
        uint16_t dly = 0;
        while (tick_report)
        {
            mcu_clear_watchdog();
            if (fifoBuf_getUsed(link_plc.out) == 0)
            {
                tick_report = 1;
                dly = 10000;
                while (dly)
                {
                    dly--;
                }
            }
            tick_report--;
        }
        nd_fr698_report_clear();
    }
}

static void sleep_tick(stategrid_context_t *instance)
{
    if (((stategrid_context_t *)instance)->ac_power_status != AC_POWER_ON)
    {
        cpu_sleep_manage(instance, NULL);
    }
}

int baremetal_instance_entry_main(stategrid_context_t * instance, int argc, char** argv)
{
    stategrid_context_t *ctx = instance;

    stategrid_context_init(ctx);

    resource_tool_bind(ctx);


    csstack_base_init();

    //初始化时间
    ctx->ms = 0;
    /*struct tm time =
    {
        .tm_year = 2021 - 1900,  
        .tm_mon = 3,                     
        .tm_mday = 30,
        .tm_hour = 23,
        .tm_min = 59,
        .tm_sec = 57
    };
    memcpy(&ctx->time, &time, sizeof(struct tm));*/
    read_mcu_rtc(0x00, &ctx->time);
    ///////////////////////////////申请一个cache的BLOCK = 02/////////////////////////////////////////////
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_cache, (2108)))
    {
        return -1;
    }
    

    ctx->reset_flag = 0x5A; //复位标志
    ctx->need_data_recovery = 0x5A;

    uint8_t* buf = nd_malloc(instance, control_block_cache, 4);
    fifoBuf_init(&ctx->pipe_rtc, buf, 4);

    buf = nd_malloc(instance, control_block_cache, 8);
    fifoBuf_init(&ctx->pipe_key, buf, 8);

    ctx->apdu_cache = nd_malloc(instance, control_block_cache, cache_max);
        
    /////////////////////////////////////申请一个698应用的BLOCK = 03/////////////////////////////////////////////////
    if (NULL == add_block(instance, RES_DEV_RAM, control_block_698_name, 16 * 1024 + 640))//16
    {
        return -1;
    }

    link_4851.id = STG_PID_RX_698;
    link_plc.id = STG_PID_PLC_RX_698;
    link_ir.id = STG_PID_IR_RX_698;
    communication_channel_init(instance, &link_4851, UART_RS4851_CHL);
    rx645_view.id = STG_PID_RX_645;
    rx645_view.ctx = &ctx_645_server;
    rx645_view.in_queue = link_4851.in;
    rx645_view.out_queue = link_4851.out;
    communication_channel_init(instance, &link_plc, UART_MODULE_CHL);
    communication_channel_init(instance, &link_ir, UART_IR_CHL);
    ctx->plc_698_ctx = link_plc.ctx;

    ////////////////////////////申请一个mbase应用的BLOCK = 04/////////////////////////////////////////////
    mbase_cb_do_not_operation = (struct mbase_control_block_t*)add_block(instance, RES_DEV_RAM, mbase_control_block_name, 3 * 1024);
    if (NULL == mbase_cb_do_not_operation) 
    {
        return -1;
    }
    stategrid_context_set_mbase(ctx, mbase_cb_do_not_operation);
    //mbase分三个区
    //mbase_block_static
    size_t mbase_block_static_size = nd_mbase_offset_static_section_end() - nd_mbase_offset_static_section_start() + 1;
    uint32_t static_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_static, mbase_block_static_size, 0x00);
    memset((uint8_t*)static_block, 0x00, mbase_block_static_size);
    //mbase_block_cache
    size_t mbase_block_cache_size = nd_mbase_offset_cache_section_end() -  nd_mbase_offset_cache_section_start() + 1;
    uint32_t cache_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_cache, mbase_block_cache_size, mbase_block_static_size);
    memset((uint8_t*)cache_block, 0x00, mbase_block_cache_size);
    //mbase_block_persistent
    size_t mbase_block_persistent_size = nd_mbase_offset_persistent_section_end() - nd_mbase_offset_persistent_section_start() + 1;
    uint32_t persistent_block = (uint32_t)add_inside_block(instance, RES_DEV_RAM, mbase_control_block_name, mbase_block_persistent, mbase_block_persistent_size, mbase_block_static_size + mbase_block_cache_size);

    //注册EPROM  
    add_device(instance, RES_DEV_EEPROM, 0x00);
    //注册FLASH
    add_device(instance, RES_DEV_FLASH, BOARD_FLASH_SECTOR);


    //注册参数区1 及其备份区  
    register_parameter(instance, RES_DEV_EEPROM, mbase_block_parameter_1, 1 * 1024, (resource_supply_fn)supply_parameter_1_info);
    register_parameter(instance, RES_DEV_EEPROM, mbase_block_parameter_1_bp, 1 * 1024, (resource_supply_fn)supply_parameter_1_info);

    //注册事件的管理区/参数区
    register_parameter(instance, RES_DEV_EEPROM, mbase_block_parameter, 15 * 1024, (resource_supply_fn)supply_parameter_info);


    //申请一块参数保存区的EPROM区域 及备份区
    add_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom, mbase_block_static_size);//主区
    add_block(instance, RES_DEV_EEPROM, mbase_block_static_eprom_bp, mbase_block_static_size);//备份
    
    //申请一块掉电保存的EPROM区域
    add_block(instance, RES_DEV_EEPROM, mbase_block_persistent_eprom, mbase_block_persistent_size);
    

    //注册事件
    register_log(instance, RES_DEV_EEPROM, mbase_block_event, RES_DEV_EEPROM, mbase_block_parameter, 26 * 1024, (resource_supply_fn)supply_event_info);

    //注册冻结1-flash
    //必须为flash 首个块，和升级文件缓存 共用
    register_log(instance, RES_DEV_FLASH, mbase_block_event_flash1, RES_DEV_EEPROM, mbase_block_parameter, (MinuteFreeze_SECTION_SIZE) * 4 * 1024, (resource_supply_fn)supply_event_flash_1_info);

    //注册冻结2-flash
    register_log(instance, RES_DEV_FLASH, mbase_block_event_flash2, RES_DEV_EEPROM, mbase_block_parameter, (HourFreeze_SECTION_SIZE + DayFreeze_SECTION_SIZE + MonthFreeze_SECTION_SIZE) * 4 * 1024, (resource_supply_fn)supply_event_flash_2_info);
    //申请一块-flash
    add_block(instance, RES_DEV_FLASH, public_block_flash, 4096*2);
    //注册一个698的MAP
    register_map(instance, res_map_698, (resource_map_fn)resource_map_698);
#ifdef WIN32
    init_timer(instance);
#endif
    uint32_t addr = 0;  
    get_block_frist_addr(instance, RES_DEV_RAM, mbase_block_static, &addr);//RAM对应的地址
    uint32_t res = get_res(RES_DEV_EEPROM, mbase_block_static_eprom);
    nd_get_block(instance, res, (uint8_t*)addr, mbase_block_static_size); //将EPROM中的数据恢复到RAM;  

    //复位次数
    uint32_t count = 0;
    addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter << 16) + ResetCount);
    nd_get_index(instance, addr, (uint8_t*)&count, sizeof(count));
    count++;
    nd_set_index(instance, addr, (uint8_t*)&count, sizeof(count));   

    ctx->ac_power_status = AC_POWER_OFF; // 初始化为停电模式
    
    
    for (;;)
    {
#ifdef ND_FIRMWARE_SIM
        if (nd_sim_should_stop(instance))
        {
            break;
        }
#endif
        io_poll(instance);
        event_tick(instance);
        meter_base_tick(instance);
        resource_tick(instance);
        lowpower_tick(instance);
        report_poll(instance);
        sleep_tick(instance);
#ifdef ND_FIRMWARE_SIM
        nd_sim_after_run(instance);
#endif
    }
    return 0;
}




int main(int argc, char** argv)
{
    stategrid_context_t * instance = NULL;
    
#ifndef WIN32
    mcu_init(NULL);
    
    ndv_timer_init();
    
    instance = stategrid_context_create(RES_DEV_RAM, (uint32_t)&nd_memory, 24 * 1024, 2 * 1024, resource_block_name);
    if (instance == NULL)
    {
        return -1;
    }
    dl698_parameter_bind(instance);
    stategrid_platform_bind(instance);
    stategrid_record_storage_bind(instance);
    stategrid_backlight_bind();
    stategrid_relay_status_bind();
    stategrid_metering_input_bind();
    stategrid_runtime_bind();
    stategrid_timer_bind();

    //初始化硬件
    dev_init(instance, AC_POWER_OFF);

    //初始化固件
    irq_system_init();
#endif // !WIN32

    baremetal_instance_entry_main(instance, argc, argv);
}

