#include "domain/freeze/sequence.h"

uint32_t nd_freeze_sequence_rollback(uint32_t current, uint16_t rolled_back,
                                     uint8_t has_records)
{
    if (!has_records || current < rolled_back) {
        return 0;
    }
    return current - rolled_back;
}

uint32_t nd_freeze_sequence_recover(uint32_t current, uint32_t last_record)
{
    if (last_record == 0xFFFFFFFFu) {
        return current;
    }
    return current < last_record + 1 ? last_record + 1 : current;
}

uint8_t nd_freeze_rollback_required(uint32_t before_seconds,
                                    uint32_t after_seconds)
{
    return (uint8_t)(after_seconds < before_seconds);
}
