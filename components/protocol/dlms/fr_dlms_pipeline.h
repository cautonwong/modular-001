#ifndef ND_ADAPTER_DLMS_PIPELINE_H
#define ND_ADAPTER_DLMS_PIPELINE_H

#include <stddef.h>
#include <stdint.h>

struct instance_t;
struct message_t;

/* Prefix layout shared with the legacy kernel pipeline. DLMS only reads the
   context and queue pointers; the trailing kernel fields stay opaque. */
struct pipeline_t {
    uint8_t id;
    char *name;
    void *ctx;
    void *in_queue;
    void *out_queue;
    int (*on_work)(struct instance_t *instance, struct pipeline_t *pipeline,
                   struct message_t *mesg);
    size_t filter_n;
    const void *filters;
};

#endif
