#ifndef ND_APPLICATION_ADAPTERS_DL645_MESSAGE_H
#define ND_APPLICATION_ADAPTERS_DL645_MESSAGE_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct context_645_t;

/*
 * Protocol-local message value for 645. The layout matches the legacy kernel
 * message header because the master path copies it into instance->cache and
 * places payload bytes after it.
 */
#pragma pack(1)
typedef struct nd_dl645_msg {
    uint32_t magic;
    uint8_t from;
    uint8_t to;
    uint8_t ctrl;
    uint8_t qos;
    uint32_t id;
    uint16_t len;
    uint8_t oad_n;
    uint8_t x;
    uint8_t *payload;
    uint16_t check;
} nd_dl645_msg_t;
#pragma pack()

typedef int (*nd_dl645_send_fn)(struct instance_t *instance,
                                struct pipeline_t *pipeline,
                                nd_dl645_msg_t *mesg);

#endif
