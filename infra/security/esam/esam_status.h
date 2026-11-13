#ifndef ND_ESAM_STATUS_H
#define ND_ESAM_STATUS_H

#include <stdint.h>

struct instance_t;

/*
 * Security-facing esam state that the legacy control block owns. The target
 * adapter maps these calls onto the block and its persistent slice.
 */
void nd_esam_status_chip_abnormal_set(struct instance_t *instance,
                                      uint8_t abnormal);
void nd_esam_status_key_state_set(struct instance_t *instance, uint8_t state);
void nd_esam_status_auth_on_set(struct instance_t *instance, uint8_t on);
int nd_esam_status_meter_no_update(struct instance_t *instance,
                                   const uint8_t *meter_no);
int nd_esam_status_auth_pending(struct instance_t *instance);

#endif
