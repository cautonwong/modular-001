#include "components/protocol/dl645/nd_dl645.h"

#include <string.h>

#include "components/core/bus/nd_bus.h"
#include "application/adapters/dl645/fr_645_context.h"
#include "application/ports/fifo_channel.h"
#include "application/ports/fifo.h"

#define ND_DL645_INSTANCES 3

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



struct nd_dl645 {
    nd_dl645_config_t config;
    struct fifo_t *rx_fifo;
    struct fifo_t *tx_fifo;
    int used;
};

static nd_dl645_t instances[ND_DL645_INSTANCES];

nd_dl645_t *nd_dl645_create(const nd_dl645_config_t *config)
{
    size_t i;

    if ((config == NULL) || (config->instance == NULL)
        || (config->context == NULL) || (config->input == NULL)
        || (config->output == NULL)) {
        return NULL;
    }

    for (i = 0; i < ND_DL645_INSTANCES; i++) {
        if (!instances[i].used) {
            nd_dl645_t *self = &instances[i];
            struct fifo_t *rx_fifo = nd_fifo_channel_fifo(config->input);
            struct fifo_t *tx_fifo = nd_fifo_channel_fifo(config->output);

            if ((rx_fifo == NULL) || (tx_fifo == NULL)) {
                return NULL;
            }

            memset(self, 0, sizeof(*self));
            self->config = *config;
            self->rx_fifo = rx_fifo;
            self->tx_fifo = tx_fifo;
            self->used = 1;
            return self;
        }
    }
    return NULL;
}

void nd_dl645_destroy(nd_dl645_t *self)
{
    if (self != NULL) {
        memset(self, 0, sizeof(*self));
    }
}

int nd_dl645_poll(nd_dl645_t *self)
{
    struct pipeline_t pipeline = { 0 };

    if ((self == NULL) || !self->used) {
        return -1;
    }

    pipeline.id = self->config.pid;
    pipeline.ctx = self->config.context;
    pipeline.in_queue = self->rx_fifo;
    pipeline.out_queue = self->tx_fifo;

    if ((fr_uart_timeout(self->config.instance, &pipeline) != 0)
        && (fifoBuf_getUsed(self->rx_fifo) > 0)) {
        nd_dl645_msg_t mesg = { 0 };
        int result = fr_rx_645_entry(self->config.instance, &pipeline, &mesg);

        if (result > 0) {
            nd_event_t event = { 0 };

            fr_gatekeeper_645_server(self->config.instance, &pipeline, &mesg);

            event.topic = ND_EVENT_DL645_FRAME;
            event.timestamp_ms = (self->config.now_ms != NULL)
                ? self->config.now_ms(self->config.instance) : 0;
            event.data.frame.channel = self->config.context->channel;
            event.data.frame.length = self->config.context->min_len;
            nd_bus_publish(&event);
        }
    }
    return 1;
}
