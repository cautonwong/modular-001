#include "nidavellir.h"
#include "components/core/bus/nd_bus.h"
#include "infra/boards/stategrid_single_phase/meter_config.h"
#include "infra/boards/stategrid_single_phase/meter_status.h"
extern const uint8_t init_Meter_Type[];
#ifndef WIN32
#include "cpu_surport.h"
#endif // !WIN32

#ifdef WIN32
#define AC_POWER_ON 0xAA
#define AC_POWER_OFF 0x55
#endif 

#include "infra/boards/stategrid_single_phase/board_io.h"
#ifdef ND_FIRMWARE_SIM
void nd_sim_filter(struct instance_t* instance, struct pipeline_t* pipeline, const struct filter_t* filter, int ret);
void nd_sim_message(struct instance_t* instance, struct message_t* mesg);
#endif
extern int PowerOff_Save_data(struct instance_t* instance,  struct pipeline_t* pipeline, struct message_t* mesg);
extern int clear_mesg(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
char* get_pipeline_name(struct instance_t* instance, pid_t pipeline_id)
{
    struct recipe_t* recipe = instance->recipe;
    struct pipeline_t* pipeline;
    uint8_t pipeline_n = recipe->pipeline_n;

    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];

        if (pipeline->id == pipeline_id)
        {
            return pipeline->name;
        }
    }

    return "kernel";
}

struct fifo_t* get_pipeline_stdin(struct instance_t* instance, pid_t pipeline_id)
{
    struct recipe_t* recipe = instance->recipe;
    struct pipeline_t* pipeline;
    uint8_t pipeline_n = recipe->pipeline_n;

    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];

        if (pipeline->id == pipeline_id)
        {
            return pipeline->in_queue;
        }
    }

    return NULL;
}
void* get_pipeline_ctx(struct instance_t* instance, pid_t pipeline_id)
{
    const struct recipe_t* recipe = instance->recipe;
    struct pipeline_t* pipeline;
    uint8_t pipeline_n = recipe->pipeline_n;

    if(recipe)
    {
        for (size_t i = 0; i < pipeline_n; i++)
        {
            pipeline = recipe->pipelines[i];
#if WIN32_U_TEST
            return pipeline->ctx;
#endif
            if (pipeline->id == pipeline_id)
            {
                return pipeline->ctx;
            }
        }
    }

    return NULL;
}


struct instance_t *create_nidavellir(struct recipe_t *recipe)
{
    return NULL;
}


static void publish_event(struct instance_t *instance, nd_event_topic_t topic)
{
    nd_event_t event = { 0 };

    event.topic = topic;
    event.timestamp_ms = instance->ms;
    nd_bus_publish(&event);
}


static void publish_power_event(struct instance_t *instance, uint8_t on)
{
    nd_event_t event = { 0 };

    event.topic = ND_EVENT_POWER_CHANGE;
    event.timestamp_ms = instance->ms;
    event.data.power.on = on;
    nd_bus_publish(&event);
}

int fr_sw_rtc(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    if (fifoBuf_getUsed(&instance->pipe_rtc) > 0)
    {
        uint8_t byte = fifoBuf_getByte(&instance->pipe_rtc);
#if WIN32
        if (byte == 1)
#else
        if (byte == IRQ_RTC)
#endif // WIN32
        {
            mesg->from = kernel;
            if(instance->ac_power_statue == AC_POWER_ON)
            {
                mesg->to = pid_meter_base;
            }
            else
            {
                mesg->to = pid_lowpower_wakeup;
            }
            mesg->ctrl = MSG_SECOND;
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg);
            publish_event(instance, ND_EVENT_TICK_SECOND);
        }
    }
    return 0;
}

int fr_sw_key(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint8_t keydly=0;
    if (fifoBuf_getUsed(&instance->pipe_key) > 0)
    {
        uint8_t byte = fifoBuf_getByte(&instance->pipe_key);
#if WIN32
        if (byte == 2)
#else
        if (byte == IRQ_PMU)
#endif // WIN32
        {
            mesg->from = kernel;
            if(instance->ac_power_statue == AC_POWER_ON)
            {
                mesg->to = pid_meter_base;
                mesg->ctrl = MSG_PAGE_KEY_UP;
            }
            else
            {
                for(keydly=0; keydly<keydlymax; keydly++)
                {
                    if(nd_board_key_released())    //消抖，按键抬起的时候不进入唤醒
                    {
                        keydly = 0;
                        return 0;
                    }
                }
                
                if(keydly >= keydlymax)
                {
                    mesg->to = pid_lowpower_wakeup;
                    mesg->ctrl = MSG_PAGE_KEY_UP;
                }
            }
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg);
        }
    }

    return 0;
}




int fr_sw_ac_power(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
#ifndef WIN32
    uint32_t addr = 0;
    int ret = 0;
    uint16_t metertype = 0;

    if (instance->ac_power_statue == AC_POWER_ON)
    {
        if(true == ac_power_off_monitor())
        {
            device_power_close(instance);//关闭大功率外设
            PowerOff_Save_data(instance, pipeline, mesg); //数据保存

            mesg->from = kernel;
            mesg->to = pid_meter_base;
            mesg->ctrl = MSG_POWEROFF;
            mesg->id = 0x0000001;
            mesg->len = 0;
            clear_mesg(instance, NULL, mesg);
            send_mesg(instance, NULL, mesg);
            //mesg->from = kernel;
            //mesg->to = pid_resource_manager;
            //mesg->ctrl = MSG_POWEROFF;
            //mesg->id = 0x0000001;
            //mesg->len = 0;
            //send_mesg(instance, NULL, mesg);
            instance->ac_power_statue = AC_POWER_OFF;
            publish_power_event(instance, 0);

        }
    }
    else
    {
        if(true == ac_power_on_monitor())
        {
            //获取电表类型
            addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + ND_METER_TYPE_PARAM_ID);
            ret = nd_get_index(instance, addr, (uint8_t*)&metertype, 2);
            if(ret < 0) //校验错误
            {
                metertype = (uint16_t)(init_Meter_Type[0] |
                                       ((uint16_t)init_Meter_Type[1] << 8));
                //addr = ((RES_DEV_EEPROM << 24) + (mbase_block_parameter_1 << 16) + );
                //nd_set_index(instance, s_index, (uint8_t*)&metertype, 2);
            }
            nd_meter_status_metertype_set(instance, metertype);

            device_power_open(instance, pipeline);
            mesg->from = kernel;
            mesg->to = pid_meter_base;
            mesg->ctrl = MSG_POWERON;
            mesg->id = 0x0000001;
            mesg->len = 0;
            clear_mesg(instance, NULL, mesg);
            send_mesg(instance, NULL, mesg);
            //mesg->from = kernel;
            //mesg->to = pid_resource_manager;
            //mesg->ctrl = MSG_POWERON;
            //mesg->id = 0x0000001;
            //mesg->len = 0;
            //send_mesg(instance, NULL, mesg);
            instance->ac_power_statue = AC_POWER_ON;
            publish_power_event(instance, 1);
            
        }
    }
#endif // !WIN32
    return 0;
}


int nidavellir_run(struct instance_t *instance)
{
    struct message_t mesg = {0};
    const struct recipe_t *recipe = instance->recipe;
    const struct filter_t *filter; 
//    pid_t pid = 0;
    int ret = 0;
    size_t n = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline;

//    extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
  //  fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);

    fr_sw_ac_power(instance,NULL,&mesg); //挪到RTC前面
    
    fr_sw_rtc(instance,NULL,&mesg);
    
    fr_sw_key(instance,NULL,&mesg);


    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        if ((pipeline->filters == NULL) || (pipeline->filter_n == 0))
        {
            continue;
        }
        while(fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t ))//waitmesg
        {
            filter = pipeline->filters;
            n = pipeline->filter_n;
            do
            {
                ret = filter->on_work(instance, pipeline, &mesg);
#ifdef ND_FIRMWARE_SIM
                nd_sim_filter(instance, pipeline, filter, ret);
#endif
                if (ret > 1)
                {
                    filter = id_to_filter(pipeline->filters, n, ret);
                }
                else if (ret == 1)
                {
                    filter = id_to_filter(pipeline->filters, n, filter->ok_id);
                }
                else if (ret == -1)
                {
                    filter = id_to_filter(pipeline->filters, n, filter->err_id);
                }
                else if (ret < -1)
                {
                    filter = id_to_filter(pipeline->filters, n, -ret);
                }
                else if (ret == 0)
                {
                    filter = NULL;
                    
                }
            } while (filter != NULL);
        }
    }
    return 1;
}

struct pipeline_t* get_pipeline(struct instance_t* instance, pid_t id)
{
    const struct recipe_t* recipe = instance->recipe;
    struct pipeline_t* pipeline;

    for (size_t i = 0; i < recipe->pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        if(id == pipeline->id)
        {
            return pipeline;
        }
    }
    return NULL;
}

int fr_uart_timeout(struct instance_t* instance, struct pipeline_t* pipeline)
{
    int ret = 0;
    uint8_t index = 0xff;
    
    if(pipeline->id == pid_rx_698){
        index = 0;
    }else if(pipeline->id == pid_ir_rx_698){
        index = 2;
    }else if(pipeline->id == pid_plc_rx_698){
        index = 1;
    }else{
        return 1;
    }
    
    if (fifoBuf_getUsed(&instance->pipe_uart[index]) > 0){
        uint8_t byte = fifoBuf_getByte(&instance->pipe_uart[index]);
        if(IRQ_UART_RX_TM_OUT == byte){
            ret = 1;
        }
    }
       
    return ret;
}


int fr_uart_empty(struct instance_t* instance, uint8_t chl)
{
    uint8_t channel = 0xff;
    uint8_t rx_used = 0xff, tx_used = 0xff;
    struct fifo_t* fifo = NULL;
    const struct recipe_t *recipe = instance->recipe;
    struct pipeline_t* pipeline = NULL;
    for (size_t i = 0; i < recipe->pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        switch(pipeline->id)
        {
        case pid_ir_rx_698: 
            channel = chl_ir_698;
            break;
        case pid_rx_698: 
            channel = chl_RS485_1_698;
            break;
        case pid_plc_rx_698: 
            channel = chl_PLC_698;
            break;
        case pid_4852_rx_698: 
            channel = chl_RS485_2_698;
            break;
        default:
            break;
        }
        if(channel == chl){
            fifo = (struct fifo_t*)pipeline->in_queue;
            rx_used = fifoBuf_getUsed(fifo);
            fifo = (struct fifo_t*)pipeline->out_queue;
            tx_used = fifoBuf_getUsed(fifo);
            //if((rx_used == 0)&&(tx_used == 0))
            //    return  chl_empty;
            if(tx_used == 0)
                return  chl_tx_empty;//;
            //if(rx_used==0)
            //    return  chl_rx_empty;//;
            return  chl_no_empty;
        }
    }
    return -1;
}

extern uint16_t report_flag;
int nidavellir_run_baremetal(struct instance_t* instance)
{
    struct message_t mesg;
    const struct recipe_t* recipe = instance->recipe;
    const struct filter_t* filter;
    size_t n = 0;
    int ret = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline = NULL;
    uint32_t tick_report = 0;

#ifndef WIN32
    mcu_clear_watchdog();
#endif

#ifdef NIDAVELLIR_SYSTEM_TEST
    extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);
#endif
    //接收时钟更新信号的处理
#ifndef WIN32
    irq_transform_to_instance(instance);
#endif // !WIN32

    fr_sw_ac_power(instance,NULL,&mesg);
    
    fr_sw_rtc(instance, NULL, &mesg);
    
    fr_sw_key(instance,NULL,&mesg);
    

    //处理事件标志组  事件驱动
    //handle_event_flags()

    //handle message loop
    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];

        if ((pipeline->filters == NULL) || (pipeline->filter_n == 0))
        {
            continue;
        }

        if(fr_uart_timeout(instance, pipeline) == 0){
            continue;               
        }
        
        if (fifoBuf_getUsed(pipeline->in_queue) >= 12)
        {
            filter = pipeline->filters;
            n = pipeline->filter_n;
            do
            {
                ret = filter->on_work(instance, pipeline, &mesg);
#ifdef ND_FIRMWARE_SIM
                nd_sim_filter(instance, pipeline, filter, ret);
#endif
                if (ret > 1)
                {
                    filter = id_to_filter(pipeline->filters, n, ret);
                }
                else if (ret == 1)
                {
                    filter = id_to_filter(pipeline->filters, n, filter->ok_id);
                }
                else if (ret == -1)
                {
                    filter = id_to_filter(pipeline->filters, n, filter->err_id);
                }
                else if (ret < -1)
                {
                    filter = id_to_filter(pipeline->filters, n, -ret);
                }
                else if (ret == 0)
                {
                    filter = NULL;
                }
            } while (filter != NULL);
        }
    }

    if(report_flag == 0xa55a)
    {
        uint16_t dly=0;
        tick_report = 0x4FFFF;
        while (tick_report)
        {
          mcu_clear_watchdog();
          if(fr_uart_empty(instance, 2) == chl_tx_empty)
          {
                tick_report = 1;
                dly = 10000;
                while(dly)  //延时，等待最后一个字节16发送完毕
                {
                    dly--;
                }
          }
          tick_report--;
        }
        report_flag = 0;
    }
    //处理持久化事务  如掉电保存和记录保存等  每圈保存一次
    //sync_data()
    if(instance->ac_power_statue != AC_POWER_ON)
    {
        cpu_sleep_manage(instance, pipeline);
    }
    
    return 1;
}

int baremetal_instance_entry(struct instance_t *instance)
{     
    log_info("meter go.\n");
    //log_warn("meter go.\n");
    //log_err("meter go.\n");
    //uint8_t buf[8] = {0x55,0xAF,0xEE,0x67};
    //log_hex(buf, sizeof(buf));    

    while (1)
    {
        nidavellir_run_baremetal(instance);
    }
}

int nidavellir_loop(struct instance_t *instance)
{
    while(1)
    {
        nidavellir_run_baremetal(instance);
    }
}

int baremetal_loop(struct instance_t* instance)
{
    while (1)
    {
        nidavellir_run(instance);
    }
}

int clear_mesg(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg)
{
    struct  fifo_t* fifo = get_pipeline_stdin(instance,mesg->to);
    if(fifo){
        ringbuffer_clear(fifo);
    }
    return 0;
}


int send_mesg(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg)
{
    struct  fifo_t* fifo = get_pipeline_stdin(instance,mesg->to);
    if (pipeline == NULL){
        mesg->from = kernel;
    }else{
        mesg->from = pipeline->id;
    }   
    mesg->magic = MSG_MAGIC;
    mesg->check = nd_get_crc16((uint8_t*)mesg, sizeof(struct message_t) - 2);
#ifdef ND_FIRMWARE_SIM
    nd_sim_message(instance, mesg);
#endif
#if 0
    log_info(": Free=%d.from=%d.to=%d.ctrl=%d.\n", fifoBuf_getFree(fifo), mesg->from, mesg->to, mesg->ctrl);
#endif

    fifoBuf_putData(fifo, mesg, sizeof(struct message_t));
    if ((mesg->len > 0)&&(mesg->payload)){
#ifdef WIN32
        log_hex(payload, mesg->len);
#endif
        fifoBuf_putData(fifo, mesg->payload, mesg->len);
        return 1;
    }
    return 0;
}


int recv_mesg_v2(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t *payload, uint16_t max_len)
{
    uint16_t len = 0;
    uint16_t check = 0;
    int ret = MESG_V2_OK;
    struct fifo_t *buf = pipeline->in_queue;
    uint8_t *p = buf->buf_ptr + buf->rd;
    uint16_t rd = buf->rd;
    uint16_t wr = buf->wr;
    if (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t)){
        len = fifoBuf_getData(pipeline->in_queue, mesg, sizeof(struct message_t));
        ret = MESG_V2_ERR_LEN;
        if(len == sizeof(struct message_t)){
            ret = MESG_V2_ERR_MAGIC;
            if (MSG_MAGIC == mesg->magic){
                check = nd_get_crc16((uint8_t*)mesg, sizeof(struct message_t) - 2);
                ret = MESG_V2_ERR_CHECK;
                if (check == mesg->check){
                    ret = MESG_V2_OK;
                    if ((mesg->len > 0) && (mesg->payload) && (payload)){
                        ret = MESG_V2_HEAD_PAYLOAD_EXT;                        
                        if(mesg->len <= max_len){
                            fifoBuf_getData(pipeline->in_queue, payload, mesg->len); 
                            ret = MESG_V2_HEAD_PAYLOAD;
                        }
                    }else{
                        ret = MESG_V2_HEAD;
                    }
                }
            }
        }
#if 0
        if(ret < 0){
          //while(1)
          {
            log_info(": ret=%d.\n", ret);
            log_info(": rd=%d.\n", rd);
            log_info(": wr=%d.\n", wr);
            log_info(": buf_size=%d.\n", buf->buf_size);
            log_hex(p, 32);
            mcu_delay_us(50000);
            mcu_delay_us(50000);
            v85xxp_clear_watchdog();
            mcu_delay_us(50000);
            mcu_delay_us(50000);
            v85xxp_clear_watchdog();
          }
        }
#endif
    }
    //if(ret < MESG_V2_OK){
        //uint8_t data = 0;
        //fifoBuf_getData(pipeline->in_queue, &data, 1);
    //}
    
    return ret;
}
