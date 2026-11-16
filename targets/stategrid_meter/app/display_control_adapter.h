#ifndef ND_TARGET_STATEGRID_DISPLAY_CONTROL_ADAPTER_H
#define ND_TARGET_STATEGRID_DISPLAY_CONTROL_ADAPTER_H

struct instance_t;
struct pipeline_t;
struct message_t;

int backlight_pro(struct instance_t *instance, struct pipeline_t *task,
                  struct message_t *mesg);
void check_RtcBat_volt(struct instance_t *instance);

#endif
