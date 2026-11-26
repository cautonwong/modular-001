#ifndef ND_TARGET_STATEGRID_STORAGE_DEVICE_H
#define ND_TARGET_STATEGRID_STORAGE_DEVICE_H

#include <stdint.h>

#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/app/resource_ids.h"

/* Resource-tool storage by RES_DEV_* name. The board owns the chips; this is
 * the only place a name maps to one. */
int nd_storage_read(stategrid_context_t *instance, uint8_t device_name,
                    uint32_t addr, uint8_t *buf, uint16_t len);
int nd_storage_write(stategrid_context_t *instance, uint8_t device_name,
                     uint32_t addr, const uint8_t *buf, uint16_t len);
int nd_storage_erase(stategrid_context_t *instance, uint8_t device_name,
                     uint32_t addr);
uint32_t nd_storage_size(stategrid_context_t *instance,
                         uint8_t device_name);

#endif
