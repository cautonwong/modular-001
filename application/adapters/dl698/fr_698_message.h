#ifndef ND_APPLICATION_ADAPTERS_DL698_MESSAGE_H
#define ND_APPLICATION_ADAPTERS_DL698_MESSAGE_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct context_698_t;

/*
 * Protocol-local message value. It replaces the kernel message_t inside the
 * 698 adapter; it carries only the fields the 698 state machine uses.
 */
typedef struct nd_dl698_msg {
    uint32_t id;
    uint16_t len;
    uint8_t oad_n;
    uint8_t ctrl;
    uint8_t from;
    uint8_t to;
    uint8_t qos;
    uint8_t *payload;
} nd_dl698_msg_t;

typedef int (*nd_dl698_send_fn)(struct instance_t *instance,
                                struct pipeline_t *pipeline,
                                nd_dl698_msg_t *mesg);

#endif
