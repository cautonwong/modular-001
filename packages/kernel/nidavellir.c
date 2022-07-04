#include "nidavellir.h"
#ifndef WIN32
#include "cpu_surport.h"
#endif // !WIN32

#ifdef WIN32
#define AC_POWER_ON 0xAA
#define AC_POWER_OFF 0x55
#endif 

#include "pin_function_def.h"

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

    return NULL;
}


struct instance_t *create_nidavellir(struct recipe_t *recipe)
{
    return NULL;
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
                    if(DISPLAY_KEY_GET_DIN() == 1)    //消抖，按键抬起的时候不进入唤醒
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

    if (instance->ac_power_statue == AC_POWER_ON)
    {
        if(true == ac_power_off_monitor())
        {
            mesg->from = kernel;
            mesg->to = pid_meter_base;
            mesg->ctrl = MSG_POWEROFF;
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg);
            mesg->from = kernel;
            mesg->to = pid_resource_manager;
            mesg->ctrl = MSG_POWEROFF;
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg);
            instance->ac_power_statue = AC_POWER_OFF;
            device_power_close1(instance);//待调整
        }
    }
    else
    {
        if(true == ac_power_on_monitor())
        {
            device_power_open(instance);
            mesg->from = kernel;
            mesg->to = pid_meter_base;
            mesg->ctrl = MSG_POWERON;
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg);
            mesg->from = kernel;
            mesg->to = pid_resource_manager;
            mesg->ctrl = MSG_POWERON;
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg);
            instance->ac_power_statue = AC_POWER_ON;
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
    
    fr_sw_rtc(instance,NULL,&mesg);
    
    fr_sw_key(instance,NULL,&mesg);
    
    fr_sw_ac_power(instance,NULL,&mesg);

    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        while(fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t ))//waitmesg
        {
            filter = pipeline->filters;
            n = pipeline->filter_n;
            do
            {
                ret = filter->on_work(instance, pipeline, &mesg);
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

int fr_uart_timeout(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
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

int nidavellir_run_baremetal(struct instance_t* instance)
{
    struct message_t mesg;
    const struct recipe_t* recipe = instance->recipe;
    const struct filter_t* filter;
    size_t n = 0;
    int ret = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline = NULL;

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

    fr_sw_rtc(instance, NULL, &mesg);
    
    fr_sw_key(instance,NULL,&mesg);
    
    fr_sw_ac_power(instance,NULL,&mesg);
   

    //处理事件标志组  事件驱动
    //handle_event_flags()

    //handle message loop
    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];

        if(fr_uart_timeout(instance, pipeline, &mesg) == 0){
            continue;               
        }
        
        if (fifoBuf_getUsed(pipeline->in_queue) >= 12)
        {
            filter = pipeline->filters;
            n = pipeline->filter_n;
            do
            {
                ret = filter->on_work(instance, pipeline, &mesg);
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

    //处理持久化事务  如掉电保存和记录保存等  每圈保存一次
    //sync_data()
    if(instance->ac_power_statue != AC_POWER_ON)
    {
        if(instance->ac_power_off_sec_cnt >= 5)//待调整
        {
            cpu_io_lowpower_cfg_chk(instance);
            
            cpu_sleep_manage(instance, pipeline);
        }


    }
    
    return 1;
}

int baremetal_instance_entry(struct instance_t *instance)
{     
    log_info("meter go.\n");
    log_warn("meter go.\n");
    log_err("meter go.\n");
    uint8_t buf[8] = {0x55,0xAF,0xEE,0x67};
    log_hex(buf, sizeof(buf));    

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
    }
    return ret;
}
