#include "nidavellir.h"

int start_tasklet(struct instance_t* instance, int tid)
{
    struct tasklet_t* tasklet = &instance->tasklet_pool.tasklets[tid];
    memcpy(&tasklet->start, &instance->time, sizeof(struct tm));
    bit_set(tid, instance->tasklet_pool.bits);
    return 0;
}

int add_tasklet(struct instance_t* instance, uint8_t interval, uint8_t dcount, 
    int (*deffered_work)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg))
{
    struct tasklet_t* tasklet;
    for (int i = 0; i < tasklet_setsize * 8; i++)
    {
        if (!bit_isset(i, instance->tasklet_pool.bits))
        {
            tasklet = &instance->tasklet_pool.tasklets[i];
            memcpy(&tasklet->start, &instance->time, sizeof(struct tm));
            tasklet->count = 0;
            tasklet->dcount = dcount;
            tasklet->interval = interval;
            tasklet->deffered_work = deffered_work;
            return i;
        }
    }

    return -1;
}

int stop_tasklet(struct instance_t* instance, int tid)
{
//    struct tasklet_t* tasklet = &instance->tasklet_pool.tasklets[tid];
    bit_clr(tid, instance->tasklet_pool.bits);
    return 0;
}

int stop_all_tasklets(struct instance_t* instance, int tid)
{
//    struct tasklet_t* tasklet = &instance->tasklet_pool.tasklets[tid];
    bit_clr(tid, instance->tasklet_pool.bits);
    memset(instance->tasklet_pool.bits, 0x00,tasklet_setsize);
    return 0;
}

int set_tasklet_start_time(struct instance_t* instance, int tid,struct tm *time)
{
    struct tasklet_t* tasklet = &instance->tasklet_pool.tasklets[tid];
    bit_clr(tid, instance->tasklet_pool.bits);
    memcpy(&tasklet->start, time, sizeof(struct tm));
    return 0;
}

int set_tasklet_deffered_work(struct instance_t* instance, int tid, 
    int (*deffered_work)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg))
{
    struct tasklet_t* tasklet = &instance->tasklet_pool.tasklets[tid];
    bit_clr(tid, instance->tasklet_pool.bits);
    tasklet->deffered_work = deffered_work;
    return 0;
}

int event_flags_get(struct instance_t* instance, int event_flag_id)
{
    assert(event_flag_id < instance->event_group_size*8);
    return bit_isset(event_flag_id, instance->event_flags);
}

int event_flags_set(struct instance_t* instance, int event_flag_id)
{
    assert(event_flag_id < instance->event_group_size * 8);
    bit_set(event_flag_id, instance->event_flags);
    return 0;
}

int event_flags_clr(struct instance_t* instance, int event_flag_id)
{
    assert(event_flag_id < instance->event_group_size * 8);
    bit_clr(event_flag_id, instance->event_flags);
    return 0;
}