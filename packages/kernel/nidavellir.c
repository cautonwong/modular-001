#include "nidavellir.h"
#ifndef WIN32
#include "cpu_surport.h"
#endif // !WIN32

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
            mesg->to = pid_meter_base;
            mesg->ctrl = MSG_SECOND;
            mesg->id = 0x0000001;
            mesg->len = 0;
            send_mesg(instance, NULL, mesg, NULL);
        }
    }

    return 0;
}

int nidavellir_run(struct instance_t *instance)
{
    struct message_t mesg;
    //struct log_item_t log_item;
    const struct recipe_t *recipe = instance->recipe;
    const struct filter_t *filter; 
    
    int ret = 0;
    size_t n = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline;
    struct cache_t* cache = instance->cache2;

//    extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
  //  fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);
    
    fr_sw_rtc(instance,NULL,&mesg);

    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        cache->used = 0;
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


int nidavellir_run_baremetal(struct instance_t* instance)
{
    struct message_t mesg;
    const struct recipe_t* recipe = instance->recipe;
    const struct filter_t* filter;
    size_t n = 0;
    int ret = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline;
    struct cache_t* cache = instance->cache2;
     
    mcu_clear_watchdog();
    
#ifdef NIDAVELLIR_SYSTEM_TEST
    extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);
#endif
    //接收时钟更新信号的处理
    irq_transform_to_instance(instance);
    
    fr_sw_rtc(instance, NULL, &mesg);

    //处理事件标志组  事件驱动
    //handle_event_flags()

    //handle message loop
    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        cache->used = 0;
        if (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
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
    return 1;
}

int baremetal_instance_entry(struct instance_t *instance)
{   
    while (1)
    {
        nidavellir_run_baremetal(instance);
    }
}



int nidavellir_loop(struct instance_t *instance)
{
    /*
    struct message_t mesg;
    mesg.ctrl = 0x13;
    mesg.id = 0x04000401;
    mesg.len = 4;
    mesg.to = msta_tx_to_meter;
    memset(msta_context.addr, 0x00, 6);
    send_mesg(instance, &pl_msta, &mesg, NULL);
    nidavellir_run(instance);
    nidavellir_run(instance);
    */
    while(1)
    {
        nidavellir_run_baremetal(instance);
    }
}

int baremetal_loop(struct instance_t* instance)
{
    /*
    struct message_t mesg;
    mesg.ctrl = 0x13;
    mesg.id = 0x04000401;
    mesg.len = 4;
    mesg.to = msta_tx_to_meter;
    memset(msta_context.addr, 0x00, 6);
    send_mesg(instance, &pl_msta, &mesg, NULL);
    nidavellir_run(instance);
    nidavellir_run(instance);
    */
    while (1)
    {
        nidavellir_run(instance);
    }
}


int send_mesg(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg, uint8_t* payload)
{
    struct  fifo_t* fifo = get_pipeline_stdin(instance,mesg->to);
    if (pipeline == NULL)
    {
        mesg->from = kernel;
    }
    else
    {
        mesg->from = pipeline->id;
    }
    fifoBuf_putData(fifo, mesg, sizeof(struct message_t));
    if (mesg->len > 0)//payload exist
    {
#ifdef WIN32
        log_hex(payload, mesg->len);
#endif
        fifoBuf_putData(fifo, payload, mesg->len);
    }
    return 0;
}

int recv_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* payload)
{
    uint16_t r = 0;
    if (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
    {
        r = fifoBuf_getData(pipeline->in_queue, mesg, sizeof(struct message_t));

        if (mesg->len > 4)
        {
            fifoBuf_getData(pipeline->in_queue, payload, mesg->len - 4);
#ifdef WIN32
            log_hex(payload, mesg->len);
#endif
        }
        return r;
    }

    return 0;
}

int recv_mesg_header(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint16_t r = 0;
    uint16_t check = 0;
    if (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
    {
        r = fifoBuf_getData(pipeline->in_queue, mesg, sizeof(struct message_t));
        return r;
        if (MSG_MAGIC == mesg->magic)
        {
            check = nd_get_crc16((uint8_t*)mesg, sizeof(struct message_t) - 2);
            if (check == mesg->check)
            {
#if WIN32
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                //printf("                   {%s,recv,%s,{0x%02x,%d,0x%04x", get_pipeline_name(instance, pipeline->id), get_pipeline_name(instance, mesg->from), mesg->ctrl, mesg->len, mesg->id);
                //printf("}}\n");
#endif
                return r;
            }
        }
    }

    return 0;
}
