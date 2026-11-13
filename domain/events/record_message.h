#ifndef ND_EVENTS_RECORD_MESSAGE_H
#define ND_EVENTS_RECORD_MESSAGE_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;

void nd_record_message_send(struct instance_t *instance,
                            struct pipeline_t *pipeline, uint16_t record_id,
                            uint8_t *payload);

#endif
