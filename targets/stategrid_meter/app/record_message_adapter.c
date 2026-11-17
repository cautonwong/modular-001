#include "domain/events/record_message.h"

#include "targets/stategrid_meter/app/app_entry.h"

/* The record manager runs later in the same loop pass, after the meter-base
 * tick, so the write order matches the old pipeline recipe. */
void nd_record_message_send(void *instance, uint16_t record_id)
{
    stategrid_record_enqueue((stategrid_context_t *)instance, record_id);
}
