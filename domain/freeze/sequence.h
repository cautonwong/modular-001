#ifndef ND_DOMAIN_FREEZE_SEQUENCE_H
#define ND_DOMAIN_FREEZE_SEQUENCE_H

#include <stdint.h>

/* Rolls the sequence back after the records of a reverted time window are
 * removed. An empty log restarts at zero. */
uint32_t nd_freeze_sequence_rollback(uint32_t current, uint16_t rolled_back,
                                     uint8_t has_records);

/* Takes the larger of the persisted counter and the last stored record, so a
 * reset cannot append over records that are still in storage. */
uint32_t nd_freeze_sequence_recover(uint32_t current, uint32_t last_record);

/* True when the clock moved backwards and stored records must be rolled back. */
uint8_t nd_freeze_rollback_required(uint32_t before_seconds,
                                    uint32_t after_seconds);

#endif
