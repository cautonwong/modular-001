#ifndef ND_TARGET_STATEGRID_FREEZE_APPLICATIONS_H
#define ND_TARGET_STATEGRID_FREEZE_APPLICATIONS_H

#include <stdint.h>


int freeze_pro(void *instance, void *task,
               void *mesg);
int freeze_power_on_pro(void *instance, void *task,
                        void *mesg);
int cal_record_size_sectors(void *instance, uint8_t *capture_ptr,
                            uint8_t *depth_ptr, uint32_t oad);
int freez_size_sectors_init(void *instance, uint32_t oad);
int record_rollback_after_set_time(void *instance,
                                   uint8_t finish_check);

#endif
