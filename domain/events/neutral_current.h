#ifndef ND_DOMAIN_EVENTS_NEUTRAL_CURRENT_H
#define ND_DOMAIN_EVENTS_NEUTRAL_CURRENT_H

#include <stdint.h>

#include "domain/events/duration.h"

typedef struct {
    uint32_t minimum_current; /* below this on both lines the event is disabled */
    uint32_t rate_threshold;  /* difference as a ratio of 10000 */
    uint8_t debounce_seconds;
} nd_neutral_current_rule_t;

typedef struct {
    nd_event_duration_t debounce;
    uint32_t accumulated_seconds;
} nd_neutral_current_state_t;

nd_event_transition_t nd_neutral_current_step(nd_neutral_current_state_t *state,
                                              const nd_neutral_current_rule_t *rule,
                                              int32_t phase_current,
                                              int32_t neutral_current);

#endif
