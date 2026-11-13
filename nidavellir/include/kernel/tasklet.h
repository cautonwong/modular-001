#ifndef ND_KERNEL_TASKLET_H
#define ND_KERNEL_TASKLET_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "types.h"

struct instance_t;
struct pipeline_t;
struct message_t;

struct period_object_t
{
    uint32_t cron;
    uint32_t id;
    uint8_t len;
    uint8_t ctrl;
};

struct tasklet_t
{
    int (*deffered_work)(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg);
    uint8_t interval;
    uint8_t dcount;
    pid_t pid;//自己所属的pipeline
    
    uint8_t count;
    struct tm start;
};

#define tasklet_setsize    1
#define bit_set(n, p)  ((p)[(n)>>3] |=  (1 << ((n) & 7)))
#define bit_clr(n, p)  ((p)[(n)>>3] &= ~(1 << ((n) & 7)))
#define bit_isset(n,p) ((p)[(n)>>3] &   (1 << ((n) & 7)))
#define tasklet_zero(p)    memset((void*)(p),0,sizeof(*(p)))
struct tasklet_pool_t
{
    uint8_t bits[tasklet_setsize];
    struct tasklet_t tasklets[tasklet_setsize * 8];
};

#endif
