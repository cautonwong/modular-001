#include "domain/events/power_event.h"

uint8_t nd_event_close_uses_poweron(uint32_t oad)
{
    return (uint8_t)((oad & 0xFFu) == 0xAAu);
}

void nd_event_power_resume(nd_event_power_resume_state_t *state)
{
    state->power_off_acc += state->power_off_low;
    state->cover_open_acc += state->cover_open_low;
    state->power_abnormal_acc += state->power_abnormal_low;
    state->power_off_low = 0;
    state->cover_open_low = 0;
    state->power_abnormal_low = 0;
}
