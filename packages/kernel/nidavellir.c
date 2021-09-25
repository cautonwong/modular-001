#include "nidavellir.h"

struct matrix_t
{
    struct fifo_t *pipe_rx;
    struct fifo_t *pipe_tx;
    struct fifo_t *pipe_tx_req;
    struct context_645_t *ctx_rx;
    struct pipeline_t *pl_rx;
    struct pipeline_t *pl_tx;
    struct fifo_t *pipe_out;
};

struct matrix_t matrices[] = 
{
    {
        &pipe_msta_rx_from_meter,
        &pipe_msta_tx_to_meter,
        &pipe_msta_tx_to_meter_req,
        &ctx_msta_rx_from_meter,
        &pl_msta_rx_from_meter,
        &pl_msta_tx_to_meter,
        &pipe_msta_tx_to_meter,//to-be-deleted
    },
    {
        &pipe_msta_rx_from_bt,
        &pipe_msta_tx_to_bt,
        &pipe_msta_tx_to_bt_req,
        &ctx_msta_rx_from_bt,
        &pl_msta_rx_from_bt,
        &pl_msta_tx_to_bt,
        &pipe_msta_rx_from_bt,//to-be-deleted
    },
    {
        &pipe_msta_rx_from_sta,
        &pipe_msta_tx_to_sta,
        &pipe_msta_tx_to_sta_req,
        &ctx_msta_rx_from_sta,
        &pl_msta_rx_from_sta,
        &pl_msta_tx_to_sta,
        &pipe_msta_rx_from_bt,//to-be-deleted
    },
    {
        &pipe_msta_tx_to_bt,
        &pipe_msta_rx_from_bt,
        &pipe_bt_tx_to_msta_req,
        &ctx_bt_rx_from_msta,
        &pl_bt_rx_from_msta,
        &pl_bt_tx_to_msta,
        &pipe_msta_rx_from_bt,
    },
    {
        &pipe_msta_tx_to_sta,
        &pipe_msta_rx_from_sta,
        &pipe_sta_tx_to_msta_req,
        &ctx_sta_rx_from_msta,
        &pl_sta_rx_from_msta,
        &pl_sta_tx_to_msta,
        &pipe_msta_rx_from_sta,
    },
    {
        &pipe_msta_tx_to_meter,
        &pipe_msta_rx_from_meter,
        &pipe_meter_tx_to_msta_req,
        &ctx_meter_rx_from_msta,
        &pl_meter_rx_from_msta,
        &pl_meter_tx_to_msta,
        &pipe_msta_rx_from_meter,
    },
};

int init_ipc_matrix()//just for test
{
    #define qsize  300
    struct matrix_t *matrix;
    size_t i = 0;
    for (i = 0; i < sizeof(matrices)/sizeof(matrices[0]); i++)
    {
        matrix = &matrices[i];
        if(i < 3)
        {
            fifoBuf_init(matrix->pipe_rx,malloc(qsize),qsize);
            fifoBuf_init(matrix->pipe_tx,malloc(qsize),qsize);
        }
        fifoBuf_init(matrix->pipe_tx_req, malloc(qsize), qsize);
        memset(matrix->ctx_rx, 0, sizeof(struct context_645_t));
        
        matrix->ctx_rx->min_len = 12;
        matrix->ctx_rx->pipeline = matrix->pl_rx;
        matrix->pl_rx->in_queue = matrix->pipe_rx;
        matrix->pl_rx->out_queue = matrix->pipe_tx;
        matrix->pl_tx->in_queue = matrix->pipe_tx_req;
        matrix->pl_tx->out_queue = matrix->pipe_tx;
        matrix->pl_rx->ctx = matrix->ctx_rx;
        matrix->pl_tx->ctx = matrix->ctx_rx;
        matrix->pl_rx->id = 2*i;
        matrix->pl_tx->id = 2*i+1;
        print_message("[==========] %d----%s\n",matrix->pl_rx->id,matrix->pl_rx->name);
        print_message("[==========] %d----%s\n",matrix->pl_tx->id,matrix->pl_tx->name);
    }
    memset(&ctx_msta_proxy_to_bt, 0, sizeof(struct context_645_t));
    memset(&ctx_proxy, 0, sizeof(struct context_645_t));
    memset(&msta_context, 0, sizeof(struct rtc_context_t));
    fifoBuf_init(&pipe_msta,malloc(200),200);
    pl_msta.id = 2 * i;
    pl_kernel.id = 2 * i + 1;

    fifoBuf_init(&stdin_698, malloc(2000), 2000);
    fifoBuf_init(&stdout_698, malloc(2000), 2000);
    fifoBuf_init(&pipe_tx_698_req, malloc(2000), 2000);
    extern struct fifo_t dlms_pdu;
    fifoBuf_init(&dlms_pdu, malloc(2048), 2048);

    return 0;
}
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
struct instance_t *create_nidavellir(struct recipe_t *recipe)
{
}

int fr_sw_rtc(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    if (fifoBuf_getUsed(&instance->pipe_rtc) > 0)
    {
        uint8_t byte = fifoBuf_getByte(&instance->pipe_rtc);//秒定时器函数或秒中断里往此fifo里put一个1
        if (byte == 1)//收到来自定时器或中断的秒信号
        {
            //发送时钟更新信号给pl_msta 进行软时钟更新  定时抄表  费率电能统计  超时和重发
            mesg->from = kernel;
            mesg->to = pid_msta;
            mesg->ctrl = 0x01;
            mesg->id = 0x0000001;
            mesg->len = 4;
            send_mesg(instance, NULL, mesg, NULL);
        }
    }

    return 0;
}

int nidavellir_run(struct instance_t *instance)
{
    struct message_t mesg;
    struct log_item_t log_item;
    struct recipe_t *recipe = instance->recipe;
    struct filter_t *filter;    
    int ret = 0;
    size_t n = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline;
    struct cache_t* cache = instance->cache2;

    //中断函数(定时器  串口中断)触发
    extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);
    
    fr_sw_rtc(instance,NULL,&mesg);//在这里接收来自定时器或中断的时钟更新信号

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
}

int nidavellir_run_baremetal(struct instance_t* instance)
{
    struct message_t mesg;
    struct recipe_t* recipe = instance->recipe;
    struct filter_t* filter;
    size_t n = 0;
    uint8_t pipeline_n = recipe->pipeline_n;
    struct pipeline_t* pipeline;
    struct cache_t* cache = instance->cache2;

    //中断函数(定时器  串口中断)触发
    extern int fr_rx_data_from_serial(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    fr_rx_data_from_serial(instance, &pl_rx_from_serial, &mesg);

    fr_sw_rtc(instance, NULL, &mesg);//在这里接收来自定时器或中断的时钟更新信号

    //处理延迟任务 deffered_task

    //处理消息循环
    for (size_t i = 0; i < pipeline_n; i++)
    {
        pipeline = recipe->pipelines[i];
        cache->used = 0;
        while (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
        {
            filter = pipeline->filters;
            n = pipeline->filter_n;
            //pipeline->entry();
            filter->on_work(instance, pipeline, &mesg);
        }
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
        nidavellir_run(instance);
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

int nidavellir_n_loop(struct instance_t **instances,size_t n)
{
    while(1)
    {
        for (size_t i = 0; i < n; i++)
        {
            nidavellir_run(instances[i]);
        }
    }
}


int nidavellir_loop_n_with_rtc_running(struct instance_t* instance, int n)
{
    struct message_t mesg;

    for (size_t i = 0; i < n; i++)
    {
        if(msta_context.CCO != 1)
        { 
            mesg.from = kernel;
            mesg.to = pid_msta;
            mesg.ctrl = 0x01;
            mesg.id = 0x0000001;
            mesg.len = 4;
            send_mesg(instance, NULL, &mesg, NULL);
        }
        nidavellir_run(instance);
        nidavellir_run(instance);
    }
}

int nidavellir_loop_n(struct instance_t *instance,int n)
{
    for (size_t i = 0; i < n; i++)
    {
        nidavellir_run(instance);
    }
}

//从from开始连续运行n seconds
int nidavellir_run_for_n_seconds_from_time(struct instance_t* instance, int n,struct tm *from)
{
    if(from != NULL)
        memcpy(&instance->time, from, sizeof(struct tm));
    
    uint32_t ts = rtc_mktime(&instance->time);
    uint32_t ts_expected = ts + n;
    clock_t start = clock();
    struct tm tm_expected;
    rtc_localtime(ts + n, &tm_expected);
    struct message_t mesg;

    while (ts <= ts_expected)
    {
        mesg.from = kernel;
        mesg.to = pid_msta;
        mesg.ctrl = 0x01;
        mesg.id = 0x0000001;
        mesg.len = 4;
        ts += 1;
        send_mesg(instance, &pl_kernel, &mesg, NULL);
        nidavellir_run(instance);
    }

    return 0;
}

int send_mesg(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg, uint8_t* payload)
{
#ifdef VERBOSE
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("                   {%s,send,%s,{0x%02x,%d,0x%04x", get_pipeline_name(instance, pipeline->id), get_pipeline_name(instance, mesg->to), mesg->ctrl, mesg->len,mesg->id);
#endif
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
    if (mesg->len > 4)//payload exist
    {
#ifdef VERBOSE
        printf(",");
        for (size_t i = 0; i < mesg->len-4; i++)
        {
            printf("%02x", payload[i]);
        }
#endif
        fifoBuf_putData(fifo, payload, mesg->len - 4);
    }
#ifdef VERBOSE
    printf("}}\n");
#endif
    return 0;
}

int recv_mesg(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg, uint8_t* payload)
{
    uint16_t r = 0;
    if (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
    {
        r = fifoBuf_getData(pipeline->in_queue, mesg, sizeof(struct message_t));
#ifdef VERBOSE
        printf("                   {%s,recv,%s,{0x%02x,%d,0x%04x", get_pipeline_name(instance, pipeline->id), get_pipeline_name(instance, mesg->from), mesg->ctrl, mesg->len, mesg->id);
#endif
        if (mesg->len > 4)
        {
            fifoBuf_getData(pipeline->in_queue, payload, mesg->len - 4);
#ifdef VERBOSE
            printf(",");
            for (size_t i = 0; i < mesg->len - 4; i++)
            {
                printf("%02x", payload[i]);
            }
#endif
        }
#ifdef VERBOSE
        printf("}}\n");
#endif
        return r;
    }

    return 0;
}

int recv_mesg_header(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    uint16_t r = 0;
    if (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
    {
        r = fifoBuf_getData(pipeline->in_queue, mesg, sizeof(struct message_t));
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        //printf("                   {%s,recv,%s,{0x%02x,%d,0x%04x", get_pipeline_name(instance, pipeline->id), get_pipeline_name(instance, mesg->from), mesg->ctrl, mesg->len, mesg->id);
        //printf("}}\n");
        return r;
    }

    return 0;
}

int message_filter(void* control_block, uint8_t* b)
{
    uint8_t byte = *b;
    int r = 0;

    return -1;
}

int fr_pack_message_head(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    
    return 0;
}

int fr_pack_message_content(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 0;
}

int fr_pack_message_tail(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    return 0;
}