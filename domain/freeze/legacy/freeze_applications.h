#ifndef ND_DOMAIN_FREEZE_LEGACY_APPLICATIONS_H
#define ND_DOMAIN_FREEZE_LEGACY_APPLICATIONS_H

#include <stdint.h>

struct instance_t;
struct pipeline_t;
struct message_t;

int freeze_pro(struct instance_t *instance, struct pipeline_t *task,
               struct message_t *mesg);
int freeze_power_on_pro(struct instance_t *instance, struct pipeline_t *task,
                        struct message_t *mesg);
int cal_record_size_sectors(struct instance_t *instance, uint8_t *capture_ptr,
                            uint8_t *depth_ptr, uint32_t oad);
int freez_size_sectors_init(struct instance_t *instance, uint32_t oad);
int record_rollback_after_set_time(struct instance_t *instance,
                                   uint8_t finish_check);

#endif
