#include "targets/stategrid_meter/persistence/legacy_energy_store.h"

#include <string.h>

#include "domain/metering/pulse_lane.h"
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

static void legacy_energy_replay(void *ctx, uint8_t lane, uint32_t count)
{
    metering_state_t *state = ctx;

    metering_add_pulses(state, nd_energy_pulse_lane_direction(lane),
                        nd_energy_pulse_lane_tariff(lane), count);
}

/* The old firmware set this word before writing the power-off record and
 * cleared it after the next boot consumed it. */
#define LEGACY_POWEROFF_FLAG 0x5AA5u

static int read_poweroff_record(const legacy_energy_sources_t *sources,
                                legacy_energy_record_t *record)
{
    return nd_store_read(sources->eeprom, sources->poweroff_offset,
                         (uint8_t *)record, sizeof(*record));
}

/* EC2 stores the energy item through Energy_chksum and the month item from
 * MonthEnegery_dec; together they cover the same 172 bytes as EC1 but are
 * separate parameter items with their own checksums. The parameter manager
 * reports a bad item checksum the same way as a bus error, so retry once. A
 * persistent month failure still counts as an absent half; a persistent energy
 * failure is a storage fault, not an absent item (adopt_legacy_energy). */
static int read_backup_energy(const legacy_energy_sources_t *sources,
                              legacy_energy_record_t *record)
{
    if (nd_store_read(sources->backup, sources->backup_energy,
                      (uint8_t *)record,
                      offsetof(legacy_energy_record_t, month_energy_dec))
        == 0) {
        return 0;
    }
    return nd_store_read(sources->backup, sources->backup_energy,
                         (uint8_t *)record,
                         offsetof(legacy_energy_record_t, month_energy_dec));
}

static int read_backup_month(const legacy_energy_sources_t *sources,
                             legacy_energy_record_t *record)
{
    size_t energy_bytes =
        offsetof(legacy_energy_record_t, month_energy_dec);

    if (nd_store_read(sources->backup, sources->backup_month,
                      (uint8_t *)record + energy_bytes,
                      sizeof(*record) - energy_bytes)
        == 0) {
        return 0;
    }
    return nd_store_read(sources->backup, sources->backup_month,
                         (uint8_t *)record + energy_bytes,
                         sizeof(*record) - energy_bytes);
}

typedef struct {
    const legacy_energy_record_t *record;
    uint8_t energy_read;
    uint8_t month_read;
} legacy_source_t;

/* Mirror Check_EC_recover: the power-off record is authoritative only while
 * the flag is set; once a boot consumed it, EC2 is the newer backup. Valid
 * and torn halves are adopted independently, so a torn half of one source
 * does not discard the other half. A persistently unreadable EC2 energy half
 * is a fault, not an absent half: on first boot it must not fall back to EC1,
 * and on a torn snapshot the EC1 fallback is recovered but reported through
 * *degraded because the total may regress to the last power-off value. */
static int adopt_legacy_energy(const legacy_energy_sources_t *sources,
                               nd_metering_snapshot_t *snapshot, int torn,
                               int *degraded)
{
    legacy_energy_record_t poweroff;
    legacy_energy_record_t backup;
    legacy_source_t order[2];
    nd_metering_snapshot_t part;
    uint16_t flag = 0;
    int poweroff_read;
    int backup_energy_read = -1;
    int backup_month_read = -1;
    int energy = 0;
    int month = 0;

    *degraded = 0;

    if (nd_store_read(sources->eeprom, sources->flag_offset,
                      (uint8_t *)&flag, sizeof(flag)) != 0) {
        /* Without the flag the source order is unknown; adopting zero could
         * regress the total, so surface the fault and retry later. */
        return -1;
    }
    poweroff_read = read_poweroff_record(sources, &poweroff);
    if (sources->backup != NULL) {
        /* The parameter manager reports a bad item checksum the same way as a
         * bus error, so an unreadable EC2 item means "no valid backup". */
        backup_energy_read = read_backup_energy(sources, &backup);
        backup_month_read = read_backup_month(sources, &backup);
    }

    order[0] = (legacy_source_t){ &poweroff, (uint8_t)(poweroff_read == 0),
                                  (uint8_t)(poweroff_read == 0) };
    order[1] = (legacy_source_t){ &backup, (uint8_t)(backup_energy_read == 0),
                                  (uint8_t)(backup_month_read == 0) };
    if (flag != LEGACY_POWEROFF_FLAG) {
        legacy_source_t tmp = order[0];

        order[0] = order[1];
        order[1] = tmp;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    for (uint8_t index = 0; index < 2; index++) {
        const legacy_source_t *source = &order[index];

        if (!energy && source->energy_read
            && legacy_energy_valid_energy(source->record)) {
            legacy_energy_decode(source->record, &part);
            snapshot->forward = part.forward;
            snapshot->reverse = part.reverse;
            energy = 1;
        }
        if (!month && source->month_read
            && legacy_energy_valid_month(source->record)) {
            legacy_energy_decode(source->record, &part);
            snapshot->month = part.month;
            month = 1;
        }
    }

    /* Once the power-off flag is clear, EC2 is the newer energy source. On
     * first boot a persistent read fault must not fall back to a valid older
     * EC1 half: that would silently regress the total. A torn snapshot has no
     * newer source left, so the legacy mirror is recovered, but the caller
     * learns that the value may be the last power-off total. If EC1 cannot
     * supply energy either, both sources are unusable and the zero start
     * applies. */
    if (sources->backup != NULL && backup_energy_read != 0
        && flag != LEGACY_POWEROFF_FLAG && energy) {
        if (!torn) {
            return -1;
        }
        *degraded = 1;
    }
    /* An unreadable EC1 leaves the half only it could supply unknown; that is
     * a bus fault, not a zero meter. Both EC2 halves still recover alone. */
    if (poweroff_read != 0 && (!energy || !month)) {
        return -1;
    }
    return 0;
}

static int persist_snapshot(nd_pulse_store_t *store,
                            const nd_metering_snapshot_t *snapshot)
{
    legacy_energy_record_t record;

    memset(&record, 0, sizeof(record));
    legacy_energy_encode(snapshot, &record);
    legacy_energy_seal(&record);
    return nd_pulse_store_commit(store, (const uint8_t *)&record,
                                 sizeof(record))
                   == ND_PULSE_OK
               ? 0
               : -1;
}

int legacy_energy_restore(nd_pulse_store_t *store,
                          const legacy_energy_sources_t *sources,
                          metering_state_t *state)
{
    legacy_energy_record_t record;
    nd_metering_snapshot_t snapshot;
    nd_pulse_status_t status;
    size_t length = 0;
    int degraded = 0;

    if (store == NULL || sources == NULL || state == NULL) {
        return -1;
    }
    status = nd_pulse_store_base(store, (uint8_t *)&record, sizeof(record),
                                 &length);
    if (status == ND_PULSE_OK) {
        if (length != sizeof(record) || !legacy_energy_valid(&record)) {
            return -1;
        }
        legacy_energy_decode(&record, &snapshot);
    } else if (status == ND_PULSE_EMPTY || status == ND_PULSE_ERROR) {
        /* EMPTY is the first boot after the upgrade. ERROR is a completed scan
         * of a region whose slots are all invalid: a known torn state an
         * explicit commit may replace. IO_ERROR is a device read fault; the
         * region is unknown, so refuse the adoption and retry later. Otherwise
         * an error that clears before the commit would write the legacy value
         * as the next generation over a real base. */
        if (adopt_legacy_energy(sources, &snapshot,
                                status == ND_PULSE_ERROR, &degraded) != 0) {
            return -1;
        }
        if (persist_snapshot(store, &snapshot) != 0) {
            return -1;
        }
    } else {
        /* FULL: the base blob does not fit the caller's buffer. IO_ERROR: a
         * device read fault, retryable without adopting. */
        return -1;
    }
    metering_import(state, &snapshot);
    status = nd_pulse_store_replay(store, legacy_energy_replay, state);
    if (status != ND_PULSE_OK && status != ND_PULSE_EMPTY) {
        return -1;
    }
    return degraded ? 1 : 0;
}

int legacy_energy_persist(nd_pulse_store_t *store, const metering_state_t *state)
{
    nd_metering_snapshot_t snapshot;

    if (store == NULL || state == NULL) {
        return -1;
    }
    metering_export(state, &snapshot);
    return persist_snapshot(store, &snapshot);
}
