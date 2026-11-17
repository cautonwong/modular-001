#ifndef ND_TARGET_STATEGRID_METER_APP_ENTRY_H
#define ND_TARGET_STATEGRID_METER_APP_ENTRY_H

#include <stdint.h>

#include "targets/stategrid_meter/app/stategrid_context.h"

int app_meter_base_event(stategrid_context_t *instance, uint8_t event);
int app_meter_lowpower_event(stategrid_context_t *instance, uint8_t event);
int PowerOff_Save_data(stategrid_context_t *instance);
void stategrid_record_enqueue(stategrid_context_t *instance, uint16_t record_id);
int baremetal_instance_entry_main(stategrid_context_t *instance, int argc,
                                  char **argv);

#endif
