#ifndef ND_APPLICATION_PORTS_PIPELINE_VIEW_H
#define ND_APPLICATION_PORTS_PIPELINE_VIEW_H

#include <stdint.h>

/* The channel fields the protocol adapters read for one dispatch pass. The
 * target owns the context and FIFOs; the view only carries them. */
typedef struct nd_pipeline_view_t
{
    uint8_t id;
    void *ctx;
    void *in_queue;
    void *out_queue;
} nd_pipeline_view_t;

#endif
