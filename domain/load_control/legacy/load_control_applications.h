#ifndef ND_DOMAIN_LOAD_CONTROL_LEGACY_APPLICATIONS_H
#define ND_DOMAIN_LOAD_CONTROL_LEGACY_APPLICATIONS_H

struct instance_t;
struct pipeline_t;

void do_relay(struct instance_t *instance, struct pipeline_t *pipeline);
void relay_led_ctl(struct instance_t *instance, struct pipeline_t *pipeline);

#endif
