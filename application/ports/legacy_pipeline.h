#ifndef ND_APPLICATION_PORTS_LEGACY_PIPELINE_H
#define ND_APPLICATION_PORTS_LEGACY_PIPELINE_H

#include <stddef.h>
#include <stdint.h>

struct instance_t;
struct message_t;
struct filter_t;

/* Layout of the legacy kernel pipeline. It matches
 * nidavellir/include/kernel/pipeline.h without including the kernel
 * header from the protocol components. */
struct pipeline_t {
    uint8_t id;
    char *name;
    void *ctx;
    void *in_queue;
    void *out_queue;
    int (*on_work)(struct instance_t *instance, struct pipeline_t *pipeline,
                   struct message_t *mesg);
    size_t filter_n;
    const struct filter_t *filters;
};

#endif
