#ifndef ND_TARGET_STATEGRID_DISPLAY_CONTROL_ADAPTER_H
#define ND_TARGET_STATEGRID_DISPLAY_CONTROL_ADAPTER_H

#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/app/message.h"
struct message_t;

int backlight_pro(stategrid_context_t *instance, void *task,
                  struct message_t *mesg);
void check_RtcBat_volt(stategrid_context_t *instance);

#endif
