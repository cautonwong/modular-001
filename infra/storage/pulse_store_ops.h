#ifndef ND_INFRA_STORAGE_PULSE_STORE_OPS_H
#define ND_INFRA_STORAGE_PULSE_STORE_OPS_H

#include <stddef.h>
#include <stdint.h>

#include "domain/metering/pulse_store.h"

/* The only definition of the consumer's opaque handle. Every binding embeds
 * one as its first member and fills the table. */
typedef struct {
    nd_pulse_status_t (*append)(void *ctx, uint8_t lane);
    nd_pulse_status_t (*replay)(void *ctx, nd_pulse_replay_fn sink,
                                void *sink_ctx);
    nd_pulse_status_t (*base)(void *ctx, uint8_t *buffer, size_t capacity,
                              size_t *length);
    nd_pulse_status_t (*commit)(void *ctx, const uint8_t *blob, size_t length);
} nd_pulse_store_ops_t;

struct nd_pulse_store {
    const nd_pulse_store_ops_t *ops;
    void *ctx;
};

#endif
