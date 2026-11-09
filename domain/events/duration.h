#ifndef ND_DOMAIN_EVENTS_DURATION_H
#define ND_DOMAIN_EVENTS_DURATION_H

#include <stdint.h>

/*
 * Debounced event state. The caller samples a condition once per second and
 * passes it in. The counter ramps up while the condition holds and down while
 * it clears. A transition is reported once per activation and once per clear.
 */
typedef struct {
    uint8_t duration;
    uint8_t active;
} nd_event_duration_t;

typedef enum {
    ND_EVENT_NONE = 0,
    ND_EVENT_STARTED = 1,
    ND_EVENT_ENDED = -1
} nd_event_transition_t;

nd_event_transition_t nd_event_duration_step(nd_event_duration_t *state,
                                             uint8_t debounce_seconds,
                                             uint8_t condition);

#endif
