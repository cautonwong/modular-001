#ifndef ND_KERNEL_LOG_OBJECT_H
#define ND_KERNEL_LOG_OBJECT_H

#include <stdint.h>
#include <time.h>

#include "types.h"

struct message_t;

//日志对象
struct log_object_t
{
    uint8_t subject;
    uint8_t verb;
    uint32_t object;
    clock_t when;
};
extern int send_log(uint8_t s,uint8_t v,uint32_t o);

struct log_item_t
{
    pid_t pipeline_id;//正处于运行态的pipeline
    struct message_t *mesg;//正在运行的pipeline收到和发送的消息
};

#endif
