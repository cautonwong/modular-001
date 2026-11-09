#include "targets/stategrid_meter/persistence/legacy_energy_store.h"

#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"

static uint32_t energy_offset(uint32_t block_offset)
{
    return block_offset + ND_LEGACY_ENERGY_OFFSET_IN_BLOCK;
}

int legacy_energy_load(const nd_store_t *store, uint32_t block_offset,
                       metering_state_t *state)
{
    legacy_energy_record_t record;
    int result = nd_store_read(store, energy_offset(block_offset),
                               (uint8_t *)&record, sizeof(record));
    if (result != 0) {
        return result;
    }

    nd_metering_snapshot_t snapshot;
    legacy_energy_decode(&record, &snapshot);
    metering_import(state, &snapshot);
    return 0;
}

int legacy_energy_save(const nd_store_t *store, uint32_t block_offset,
                       const metering_state_t *state)
{
    legacy_energy_record_t record;
    int result = nd_store_read(store, energy_offset(block_offset),
                               (uint8_t *)&record, sizeof(record));
    if (result != 0) {
        return result;
    }

    nd_metering_snapshot_t snapshot;
    metering_export(state, &snapshot);
    legacy_energy_encode(&snapshot, &record);
    return nd_store_write(store, energy_offset(block_offset),
                          (const uint8_t *)&record, sizeof(record));
}
