#ifndef ND_DOMAIN_EVENTS_LEGACY_APPLICATIONS_H
#define ND_DOMAIN_EVENTS_LEGACY_APPLICATIONS_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct message_t;

int event_pro(struct instance_t *instance, struct pipeline_t *pipeline,
              struct message_t *mesg);
int event_PowerOn_pro(struct instance_t *instance, struct pipeline_t *pipeline,
                      struct message_t *mesg);
int event_LowPower_pro(struct instance_t *instance,
                       struct pipeline_t *pipeline, struct message_t *mesg);
void send_event_mesg_pro(struct instance_t *instance,
                         struct pipeline_t *task, uint16_t RecordId,
                         uint8_t *payload, int mode);
uint8_t get_activeflag(struct instance_t *instance, uint16_t RecordId);

#endif
