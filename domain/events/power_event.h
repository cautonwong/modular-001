#ifndef ND_DOMAIN_EVENTS_POWER_EVENT_H
#define ND_DOMAIN_EVENTS_POWER_EVENT_H

#include <stdint.h>

typedef struct {
    uint32_t power_off_acc;
    uint32_t power_off_low;
    uint32_t cover_open_acc;
    uint32_t cover_open_low;
    uint32_t power_abnormal_acc;
    uint32_t power_abnormal_low;
} nd_event_power_resume_state_t;

/* Folds the low-power accumulators into the event duration and clears them. */
void nd_event_power_resume(nd_event_power_resume_state_t *state);

#endif
