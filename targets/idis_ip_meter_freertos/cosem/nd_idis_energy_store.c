#include "targets/idis_ip_meter_freertos/cosem/nd_idis_energy_store.h"

#include <stddef.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"

#define ND_IDIS_ENERGY_MAGIC 0x454E4531u

typedef struct {
    uint32_t magic;
    nd_energy_set_t set;
} nd_idis_energy_record_t;

int nd_idis_energy_save_legacy(const nd_energy_set_t *set, nd_store_t *store,
                               uint32_t offset)
{
    if (set == NULL || store == NULL) {
        return -1;
    }
    nd_idis_energy_record_t record = {
        .magic = ND_IDIS_ENERGY_MAGIC,
        .set = *set,
    };
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_energy_load_legacy(nd_energy_set_t *set, nd_store_t *store,
                               uint32_t offset)
{
    if (set == NULL || store == NULL) {
        return -1;
    }
    nd_idis_energy_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_ENERGY_MAGIC) {
        return -1;
    }
    *set = record.set;
    return 0;
}

static nd_energy_channel_id_t channel_of(nd_energy_direction_t direction)
{
    return direction == ND_ENERGY_REVERSE ? ND_ENERGY_CHANNEL_ACTIVE_EXPORT
                                          : ND_ENERGY_CHANNEL_ACTIVE_IMPORT;
}

#ifndef ND_IDIS_ENERGY_LEGACY_ONLY
static void replay_pulses(void *ctx, uint8_t lane, uint32_t count)
{
    nd_energy_set_t *set = ctx;

    (void)nd_energy_set_add_pulses(
        set, channel_of(nd_energy_pulse_lane_direction(lane)),
        nd_energy_pulse_lane_tariff(lane), count);
}
#endif

static int book_matches_legacy(const nd_energy_set_t *set, nd_store_t *legacy,
                               uint32_t legacy_offset)
{
    nd_energy_set_t base;

    return nd_idis_energy_load_legacy(&base, legacy, legacy_offset) == 0
           && memcmp(set, &base, sizeof(base)) == 0;
}

int nd_idis_energy_save(const nd_energy_set_t *set, nd_pulse_store_t *store,
                        nd_store_t *legacy, uint32_t legacy_offset)
{
    if (set == NULL) {
        return -1;
    }
#ifdef ND_IDIS_ENERGY_LEGACY_ONLY
    (void)store;
    return book_matches_legacy(set, legacy, legacy_offset)
               ? 0
               : nd_idis_energy_save_legacy(set, legacy, legacy_offset);
#else
    if (store == NULL) {
        return book_matches_legacy(set, legacy, legacy_offset)
                   ? 0
                   : nd_idis_energy_save_legacy(set, legacy, legacy_offset);
    }
    /* Each accepted pulse is recorded in the journal (at granularity N up to
     * N-1 bits may still be in RAM), so a periodic save must not erase it.
     * Commit only when the journal cannot hold the book; that is
     * nd_idis_energy_commit. */
    return 0;
#endif
}

int nd_idis_energy_commit(const nd_energy_set_t *set, nd_pulse_store_t *store,
                          nd_store_t *legacy, uint32_t legacy_offset)
{
    if (set == NULL) {
        return -1;
    }
#ifdef ND_IDIS_ENERGY_LEGACY_ONLY
    (void)store;
    return book_matches_legacy(set, legacy, legacy_offset)
               ? 0
               : nd_idis_energy_save_legacy(set, legacy, legacy_offset);
#else
    if (store == NULL) {
        return book_matches_legacy(set, legacy, legacy_offset)
                   ? 0
                   : nd_idis_energy_save_legacy(set, legacy, legacy_offset);
    }
    return nd_pulse_store_commit(store, (const uint8_t *)set, sizeof(*set))
                   == ND_PULSE_OK
               ? 0
               : -1;
#endif
}

int nd_idis_energy_load(nd_energy_set_t *set, nd_pulse_store_t *store,
                        nd_store_t *legacy, uint32_t legacy_offset)
{
#ifndef ND_IDIS_ENERGY_LEGACY_ONLY
    nd_energy_set_t base;
    size_t length = 0;
    nd_pulse_status_t status;
#endif

    if (set == NULL) {
        return -1;
    }
#ifdef ND_IDIS_ENERGY_LEGACY_ONLY
    (void)store;
    return nd_idis_energy_load_legacy(set, legacy, legacy_offset);
#else
    if (store == NULL) {
        return nd_idis_energy_load_legacy(set, legacy, legacy_offset);
    }
    status = nd_pulse_store_base(store, (uint8_t *)&base, sizeof(base),
                                 &length);
    if (status == ND_PULSE_OK) {
        if (length != sizeof(base)) {
            return -1;
        }
        *set = base;
    } else if (status == ND_PULSE_EMPTY) {
        /* First boot or upgrade: adopt the legacy record, then make the
         * caller's initialized book the generation 1 base so appends work. */
        if (nd_idis_energy_load_legacy(&base, legacy, legacy_offset) == 0) {
            *set = base;
        }
        if (nd_pulse_store_commit(store, (const uint8_t *)set, sizeof(*set))
            != ND_PULSE_OK) {
            return -1;
        }
    } else {
        return -1;
    }
    status = nd_pulse_store_replay(store, replay_pulses, set);
    return status == ND_PULSE_OK || status == ND_PULSE_EMPTY ? 0 : -1;
#endif
}

#ifndef ND_IDIS_ENERGY_LEGACY_ONLY
int nd_idis_energy_adopt(nd_energy_set_t *set, nd_pulse_store_t *store,
                         nd_store_t *legacy, uint32_t legacy_offset)
{
    uint8_t check[sizeof(nd_energy_set_t)];
    size_t length = 0;
    nd_pulse_status_t status;

    if (set == NULL || store == NULL) {
        return -1;
    }
    status = nd_pulse_store_base(store, check, sizeof(check), &length);
    if (status == ND_PULSE_OK) {
        return 1;
    }
    if (status != ND_PULSE_EMPTY && status != ND_PULSE_ERROR) {
        return -1;
    }
    if (nd_idis_energy_load_legacy(set, legacy, legacy_offset) != 0) {
        return 1;
    }
    return nd_pulse_store_commit(store, (const uint8_t *)set, sizeof(*set))
                   == ND_PULSE_OK
               ? 0
               : -1;
}
#endif

int nd_idis_energy_pulse(nd_energy_set_t *set, nd_pulse_store_t *store,
                         nd_energy_direction_t direction, uint8_t tariff)
{
#ifndef ND_IDIS_ENERGY_LEGACY_ONLY
    int durable = 1;
#endif

    if (set == NULL) {
        return -1;
    }
#ifdef ND_IDIS_ENERGY_LEGACY_ONLY
    (void)store;
#else
    if (store != NULL) {
        uint8_t lane = nd_energy_pulse_lane(direction, tariff);
        nd_pulse_status_t status = nd_pulse_store_append(store, lane);

        if (status == ND_PULSE_FULL
            && nd_idis_energy_commit(set, store, NULL, 0) == 0) {
            status = nd_pulse_store_append(store, lane);
        }
        durable = status == ND_PULSE_OK;
    }
#endif
    (void)nd_energy_set_add_pulses(set, channel_of(direction), tariff, 1u);
#ifdef ND_IDIS_ENERGY_LEGACY_ONLY
    return 0;
#else
    return durable ? 0 : -1;
#endif
}

uint32_t nd_idis_energy_pulses(uint64_t *fraction, int64_t watts,
                               uint32_t meter_const)
{
    uint64_t magnitude;
    uint64_t limit;
    uint32_t pulses;

    if (fraction == NULL || meter_const == 0u) {
        return 0;
    }
    magnitude = watts < 0 ? (uint64_t)(-(watts + 1)) + 1u : (uint64_t)watts;
    if (magnitude > ND_IDIS_ENERGY_MAX_POWER_W) {
        return 0;
    }
    /* Micro-pulses: watts * meter_const / 3.6 = watts * meter_const * 5 / 18. */
    *fraction += magnitude * meter_const * 5u / 18u;
    pulses = (uint32_t)(*fraction / 1000000u);
    *fraction %= 1000000u;
    limit = (uint64_t)ND_IDIS_ENERGY_MAX_POWER_W * meter_const * 5u / 18u
                / 1000000u
            + 1u;
    return pulses > limit ? (uint32_t)limit : pulses;
}
