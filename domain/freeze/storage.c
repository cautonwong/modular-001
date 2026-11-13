#include "domain/freeze/storage.h"

uint32_t nd_freeze_scheme_units(uint8_t on_flash, uint32_t record_bytes,
                                uint32_t depth)
{
    if (record_bytes == 0 || depth == 0) {
        return 0;
    }
    if (!on_flash) {
        return record_bytes * depth + 1;
    }

    uint32_t per_sector = ND_FREEZE_SECTOR_BYTES / record_bytes;
    if (per_sector == 0) {
        per_sector = 1;
    }
    return 1 + (depth + per_sector - 1) / per_sector;
}
