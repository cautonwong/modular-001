#include "nidavellir.h"
extern int fr_rx_645_entry(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_decode_645_11(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_head(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_pack_645_tail(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
extern int fr_enable_tx(struct instance_t *instance,struct pipeline_t *pipeline,struct message_t *mesg);
struct fifo_t pipe_log;
int fr_log(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t *mesg)
{
    struct log_object_t log;
    struct fifo_t *fifo = (struct fifo_t *)pipeline->in_queue;
    uint8_t n = 0;

    while(fifoBuf_getUsed(fifo) >= sizeof(struct log_object_t))
    {
        n = fifoBuf_getData(fifo,&log,sizeof(struct log_object_t));
        assert(n == sizeof(struct log_object_t));
        printf("%04d-%02d-%02d %02d:%02d:%02d===%d-0x%02x-0x%04x\n", 
            instance->time.tm_year + 1900,
            instance->time.tm_mon + 1,
            instance->time.tm_mday,
            instance->time.tm_hour,
            instance->time.tm_min,
            instance->time.tm_sec,
            log.subject, log.verb, log.object);
        //print_message("[==========] %d-%d-0x%x-0x%x\n",log.when,log.subject,log.verb,log.object);
    }

    return 0;
}

int send_log(uint8_t s,uint8_t v,uint32_t o)
{
    return 0;
    struct log_object_t log;
    log.object = o;
    log.subject = s;
    log.verb = v;
    log.when = clock()/CLOCKS_PER_SEC;
    fifoBuf_putData(&pipe_log,&log,sizeof(struct log_object_t));
}

struct filter_t fr_logs[] =
{
    {0x00,fr_log,1,13},
};

struct pipeline_t pl_log = 
{
    10,
    "pl_log",
    NULL,
    &pipe_log,
    NULL,
    sizeof(fr_logs)/sizeof(fr_logs[0]),
    fr_logs,
};
