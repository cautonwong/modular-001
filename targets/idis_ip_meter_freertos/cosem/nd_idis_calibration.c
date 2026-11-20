#include "targets/idis_ip_meter_freertos/cosem/nd_idis_calibration.h"

#include <stddef.h>

static int record_valid(const nd_idis_calibration_record_t *record)
{
    return record->magic == ND_IDIS_CALIBRATION_MAGIC && record->voltage > 0
           && record->current > 0 && record->power > 0;
}

static int read_record(const nd_store_t *store, uint32_t offset,
                       nd_idis_calibration_record_t *record)
{
    return nd_store_read(store, offset, (uint8_t *)record, sizeof(*record)) == 0;
}

nd_idis_calibration_status_t nd_idis_calibration_load(
    nd_idis_calibration_record_t *record, const nd_store_t *store)
{
    if (record == NULL || store == NULL) {
        return ND_IDIS_CALIBRATION_FAULT;
    }

    nd_idis_calibration_record_t current = {0};
    int current_read =
        read_record(store, ND_IDIS_CALIBRATION_OFFSET, &current);
    if (current_read && record_valid(&current)) {
        *record = current;
        return ND_IDIS_CALIBRATION_LOADED;
    }

    nd_idis_calibration_record_t legacy = {0};
    int legacy_read =
        read_record(store, ND_IDIS_CALIBRATION_LEGACY_OFFSET, &legacy);
    if (legacy_read && record_valid(&legacy)) {
        *record = legacy;
        /* A failed rewrite is retried on the next boot; the record is still
         * usable from the legacy slot. */
        (void)nd_store_write(store, ND_IDIS_CALIBRATION_OFFSET,
                             (const uint8_t *)&legacy, sizeof(legacy));
        return ND_IDIS_CALIBRATION_ADOPTED;
    }

    if ((current_read && current.magic == ND_IDIS_CALIBRATION_MAGIC)
        || (legacy_read && legacy.magic == ND_IDIS_CALIBRATION_MAGIC)) {
        return ND_IDIS_CALIBRATION_FAULT;
    }
    if (nd_idis_registry_covers(store, ND_IDIS_REGISTRY_EEPROM_OFFSET,
                                ND_IDIS_CALIBRATION_LEGACY_OFFSET)) {
        return ND_IDIS_CALIBRATION_LOST;
    }
    return ND_IDIS_CALIBRATION_NONE;
}

uint16_t nd_idis_calibration_fault_event(nd_idis_calibration_status_t status)
{
    switch (status) {
    case ND_IDIS_CALIBRATION_FAULT:
    case ND_IDIS_CALIBRATION_LOST:
        return ND_IDIS_CALIBRATION_FAULT_EVENT;
    case ND_IDIS_CALIBRATION_NONE:
    case ND_IDIS_CALIBRATION_LOADED:
    case ND_IDIS_CALIBRATION_ADOPTED:
        break;
    }
    return 0;
}
