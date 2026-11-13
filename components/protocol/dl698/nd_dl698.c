#include "components/protocol/dl698/nd_dl698.h"
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


#include <string.h>

#include "components/core/bus/nd_bus.h"
#include "application/adapters/dl698/fr_698.h"
#include "application/adapters/dl698/fr_698_context.h"
#include "application/ports/fifo_channel.h"
#include "application/ports/fifo.h"

#define ND_DL698_INSTANCES 3

struct nd_dl698 {
    nd_dl698_config_t config;
    struct fifo_t *rx_fifo;
    struct fifo_t *tx_fifo;
    int used;
};

static nd_dl698_t instances[ND_DL698_INSTANCES];

nd_dl698_t *nd_dl698_create(const nd_dl698_config_t *config)
{
    size_t i;

    if ((config == NULL) || (config->instance == NULL)
        || (config->context == NULL) || (config->input == NULL)
        || (config->output == NULL)) {
        return NULL;
    }

    for (i = 0; i < ND_DL698_INSTANCES; i++) {
        if (!instances[i].used) {
            nd_dl698_t *self = &instances[i];
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

int nd_dl698_send(nd_dl698_t *self, nd_dl698_msg_t *mesg)
{
    struct pipeline_t pipeline = { 0 };

    if ((self == NULL) || !self->used || (mesg == NULL)) {
        return -1;
    }

    pipeline.id = self->config.pid;
    pipeline.ctx = self->config.context;
    pipeline.out_queue = self->tx_fifo;
    fr_tx_698_entry(self->config.instance, &pipeline, mesg);
    return 0;
}

void nd_dl698_destroy(nd_dl698_t *self)
{
    if (self != NULL) {
        memset(self, 0, sizeof(*self));
    }
}

int nd_dl698_poll(nd_dl698_t *self)
{
    struct pipeline_t pipeline = { 0 };
    nd_dl698_msg_t mesg = { 0 };

    if ((self == NULL) || !self->used) {
        return -1;
    }

    pipeline.id = self->config.pid;
    pipeline.ctx = self->config.context;
    pipeline.in_queue = self->rx_fifo;
    pipeline.out_queue = self->tx_fifo;

    if ((fr_uart_timeout(self->config.instance, &pipeline) != 0)
        && (fifoBuf_getUsed(self->rx_fifo) >= 12)) {
        self->config.context->fr_err = 0;
        self->config.context->fr_err = fr_rx_698_entry(self->config.instance,
                                                       &pipeline, &mesg);
        if (self->config.context->fr_err > 0) {
            nd_event_t event = { 0 };

            event.topic = ND_EVENT_DL698_FRAME;
            event.timestamp_ms = self->config.now_ms != NULL
                ? self->config.now_ms(self->config.instance) : 0;
            event.data.frame.channel = self->config.context->channel;
            event.data.frame.length =
                (uint16_t)(self->config.context->min_len + 2);
            nd_bus_publish(&event);

            self->config.context->fr_err = fr_gatekeeper_698_server(
                self->config.instance, &pipeline, &mesg);
        }
    }

    return 1;
}
