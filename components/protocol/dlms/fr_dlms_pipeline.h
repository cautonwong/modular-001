#ifndef ND_ADAPTER_DLMS_PIPELINE_H
#define ND_ADAPTER_DLMS_PIPELINE_H

#include <stddef.h>
#include <stdint.h>
/* Local dispatch view. DLMS only reads the context and queue pointers; the
   trailing fields stay opaque. */
struct nd_dlms_pipeline_t {
    uint8_t id;
    char *name;
    void *ctx;
    void *in_queue;
    void *out_queue;
    int (*on_work)(void *instance, struct nd_dlms_pipeline_t *pipeline,
                   void *mesg);
    size_t filter_n;
    const void *filters;
};

#endif
