#ifndef ND_CORE_BUS_H
#define ND_CORE_BUS_H

#include <stdint.h>

#ifndef ND_BUS_MAX_SUBSCRIBERS
#define ND_BUS_MAX_SUBSCRIBERS 32
#endif

typedef enum {
    ND_EVENT_TICK_SECOND = 0,
    ND_EVENT_TICK_MINUTE,
    ND_EVENT_POWER_CHANGE,
    ND_EVENT_KEY,
    ND_EVENT_ALARM,
    ND_EVENT_DL698_FRAME,
    ND_EVENT_DL645_FRAME,
    ND_EVENT_TOPIC_COUNT
} nd_event_topic_t;

typedef struct {
    nd_event_topic_t topic;
    uint32_t timestamp_ms;
    union {
        struct {
            uint8_t key;
            uint8_t state;
        } key;
        struct {
            uint8_t on;
        } power;
        struct {
            uint32_t mask;
        } alarm;
        struct {
            uint8_t channel;
            uint16_t length;
        } frame;
    } data;
} nd_event_t;

typedef void (*nd_event_handler_t)(void *ctx, const nd_event_t *event);

/*
 * The bus is a fixed-size system service. Subscriptions are registered during
 * startup and are not removed at runtime. Publish is synchronous and must run
 * in task context; an ISR posts to a queue owned by the platform layer.
 */
void nd_bus_init(void);
void nd_bus_reset(void);

int nd_bus_subscribe(nd_event_topic_t topic, nd_event_handler_t handler,
                     void *ctx);

/*
 * Returns the number of subscribers that handled the event, or -1 when the
 * event is invalid.
 */
int nd_bus_publish(const nd_event_t *event);

#endif
