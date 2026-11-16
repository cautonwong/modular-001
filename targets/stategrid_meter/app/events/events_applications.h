#ifndef ND_TARGET_STATEGRID_EVENTS_APPLICATIONS_H
#define ND_TARGET_STATEGRID_EVENTS_APPLICATIONS_H

#include <stdint.h>


int event_pro(void *instance, void *pipeline,
              void *mesg);
int event_PowerOn_pro(void *instance, void *pipeline,
                      void *mesg);
int event_LowPower_pro(void *instance,
                       void *pipeline, void *mesg);
void send_event_mesg_pro(void *instance,
                         void *task, uint16_t RecordId,
                         uint8_t *payload, int mode);
uint8_t get_activeflag(void *instance, uint16_t RecordId);
void mac_err_cnt_add(void *instance);

#endif
