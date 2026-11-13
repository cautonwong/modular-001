#include "components/core/bus/nd_bus.h"

#include <stddef.h>

typedef struct {
    nd_event_topic_t topic;
    nd_event_handler_t handler;
    void *ctx;
} nd_bus_subscription_t;

static nd_bus_subscription_t subscriptions[ND_BUS_MAX_SUBSCRIBERS];
static size_t subscription_count;

void nd_bus_init(void)
{
    nd_bus_reset();
}

void nd_bus_reset(void)
{
    size_t i;

    for (i = 0; i < ND_BUS_MAX_SUBSCRIBERS; i++) {
        subscriptions[i].topic = ND_EVENT_TOPIC_COUNT;
        subscriptions[i].handler = NULL;
        subscriptions[i].ctx = NULL;
    }
    subscription_count = 0;
}

int nd_bus_subscribe(nd_event_topic_t topic, nd_event_handler_t handler,
                     void *ctx)
{
    if ((handler == NULL) || (topic >= ND_EVENT_TOPIC_COUNT)
        || (subscription_count >= ND_BUS_MAX_SUBSCRIBERS)) {
        return -1;
    }

    subscriptions[subscription_count].topic = topic;
    subscriptions[subscription_count].handler = handler;
    subscriptions[subscription_count].ctx = ctx;
    subscription_count++;
    return 0;
}

int nd_bus_publish(const nd_event_t *event)
{
    size_t i;
    size_t snapshot;
    int handled = 0;

    if ((event == NULL) || (event->topic >= ND_EVENT_TOPIC_COUNT)) {
        return -1;
    }

    snapshot = subscription_count;
    for (i = 0; i < snapshot; i++) {
        if ((subscriptions[i].topic != event->topic)
            || (subscriptions[i].handler == NULL)) {
            continue;
        }
        subscriptions[i].handler(subscriptions[i].ctx, event);
        handled++;
    }
    return handled;
}
