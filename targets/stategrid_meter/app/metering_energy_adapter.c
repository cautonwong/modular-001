#include "targets/stategrid_meter/app/metering_energy_adapter.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

#include <string.h>

#include "application/ports/resource_access.h"
#include "targets/stategrid_meter/app/stategrid_context.h"

#include "domain/common/store.h"
#include "domain/metering/metering.h"
#include "domain/metering/pulse_lane.h"
#include "infra/memory/arena.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/app/storage_device.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/legacy_energy_layout.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"
#include "targets/stategrid_meter/persistence/legacy_energy_store.h"

_Static_assert(sizeof(legacy_energy_record_t) == STG_ENERGY_SNAPSHOT_BLOB_BYTES,
               "snapshot blob must be the legacy 172-byte record");

/* shortcut: one energy book per firmware. The target owns instance state now,
 * so this can move into stategrid_context_t when RAM is re-budgeted. */
static uint8_t state_buffer[512];
static nd_arena_t state_arena;
static metering_state_t *energy;

struct nd_energy_ledger
{
    struct mbase_control_block_t *mbase_cb;
    stategrid_context_t *instance;
    nd_pulse_store_t *store;
    const nd_store_t *legacy_store;
    nd_store_t parameter_store;
    uint32_t legacy_offset;
    uint32_t legacy_flag_offset;
    uint32_t ec2_energy_index;
    uint32_t ec2_month_index;
    uint8_t restore_pending;
    /* Pulses counted while the base is not restored. They were never appended
     * durably (the snapshot store has no append), so they live only here and
     * a power cut during the window loses them. */
    uint32_t pending[2][ND_TARIFF_SLOTS];
};

static struct nd_energy_ledger ledger;

static int legacy_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    return nd_storage_read(ledger.instance, (uint8_t)(uintptr_t)ctx, offset,
                           buffer, (uint16_t)length);
}

/* The parameter manager serves each EC2 item by its composite resource index,
 * which the ledger carries in the offset so the store contract stays generic. */
static int parameter_read(void *ctx, uint32_t index, uint8_t *buffer,
                          size_t length)
{
    const nd_resource_port_t *port = ctx;

    return nd_get_index_port(port, index, buffer, (uint16_t)length) < 0
           ? -1 : 0;
}

static const nd_store_t legacy_store = {
    .ctx = (void *)(uintptr_t)RES_DEV_EEPROM,
    .read = legacy_read,
};

static struct mbase_control_block_t *control_block(stategrid_context_t *instance)
{
    return instance->mbase;
}

nd_energy_ledger_t *metering_energy_ledger_from_instance(
    stategrid_context_t *instance)
{
    if (instance != NULL) {
        ledger.mbase_cb = control_block(instance);
        ledger.instance = instance;
        ledger.store = instance->energy_store;
        ledger.legacy_offset = instance->legacy_energy_offset;
        ledger.legacy_flag_offset = instance->legacy_energy_flag_offset;
        ledger.ec2_energy_index = instance->legacy_energy_ec2_energy_index;
        ledger.ec2_month_index = instance->legacy_energy_ec2_month_index;
        ledger.parameter_store.ctx = &instance->resource;
        ledger.parameter_store.read = parameter_read;
    }
    ledger.legacy_store = &legacy_store;
    return &ledger;
}

nd_energy_ledger_t *metering_energy_ledger_from_mbase(
    struct mbase_control_block_t *mbase_cb)
{
    ledger.mbase_cb = mbase_cb;
    return &ledger;
}

static void note_fault(nd_energy_ledger_t *handle)
{
    if (handle->instance != NULL) {
        handle->instance->storage_fault = 1;
    }
}

static void configure(metering_state_t *state,
                      struct mbase_control_block_t *mbase_cb)
{
    metering_config_t config;

    config.meter_const = nd_mbase_meter_const(mbase_cb);
    config.cal_word = nd_mbase_active_energy_cal_word(mbase_cb);
    config.save_step = nd_mbase_EC_SaveDiff(mbase_cb);
    metering_configure(state, &config);
}

static void import_arrays(metering_state_t *state,
                          struct mbase_control_block_t *mbase_cb)
{
    nd_metering_snapshot_t snapshot;
    uint8_t slot;

    for (slot = 0; slot < ND_TARIFF_SLOTS; slot++) {
        snapshot.forward.slots[slot].integer = nd_mbase_active_p(mbase_cb)[slot];
        snapshot.forward.slots[slot].fraction = nd_mbase_active_p_dec(mbase_cb)[slot];
        snapshot.reverse.slots[slot].integer = nd_mbase_active_n(mbase_cb)[slot];
        snapshot.reverse.slots[slot].fraction = nd_mbase_active_n_dec(mbase_cb)[slot];
    }
    snapshot.month.integer = nd_mbase_MonthEnegery(mbase_cb);
    snapshot.month.fraction = nd_mbase_MonthEnegery_dec(mbase_cb);
    metering_import(state, &snapshot);
}

static void export_arrays(metering_state_t *state,
                          struct mbase_control_block_t *mbase_cb)
{
    nd_metering_snapshot_t snapshot;
    uint8_t slot;

    metering_export(state, &snapshot);
    for (slot = 0; slot < ND_TARIFF_SLOTS; slot++) {
        nd_mbase_active_p(mbase_cb)[slot] = snapshot.forward.slots[slot].integer;
        nd_mbase_active_p_dec(mbase_cb)[slot] = snapshot.forward.slots[slot].fraction;
        nd_mbase_active_n(mbase_cb)[slot] = snapshot.reverse.slots[slot].integer;
        nd_mbase_active_n_dec(mbase_cb)[slot] = snapshot.reverse.slots[slot].fraction;
    }
    nd_mbase_MonthEnegery(mbase_cb) = snapshot.month.integer;
    nd_mbase_MonthEnegery_dec(mbase_cb) = snapshot.month.fraction;
}

/* Keep the legacy RAM view and its checksums consistent with the book so the
 * legacy recovery path does not pull stale EEPROM values back in. */
static void sync_legacy_view(metering_state_t *state,
                             struct mbase_control_block_t *mbase_cb)
{
    export_arrays(state, mbase_cb);
    nd_mbase_Energy_chksum(mbase_cb) =
        do_checksum((uint8_t *)nd_mbase_active_p(mbase_cb),
                    (uint16_t)legacy_energy_checksum_span());
    nd_mbase_UseEnergy_chksum(mbase_cb) =
        do_checksum((uint8_t *)&nd_mbase_MonthEnegery_dec(mbase_cb),
                    (uint16_t)legacy_energy_use_checksum_span());
}

static int restore_book(nd_energy_ledger_t *handle, metering_state_t *state)
{
    legacy_energy_sources_t sources;

    if (handle->store == NULL) {
        import_arrays(state, handle->mbase_cb);
        return 0;
    }
    sources.eeprom = handle->legacy_store;
    sources.poweroff_offset = handle->legacy_offset;
    sources.flag_offset = handle->legacy_flag_offset;
    sources.backup =
        handle->instance != NULL ? &handle->parameter_store : NULL;
    sources.backup_energy = handle->ec2_energy_index;
    sources.backup_month = handle->ec2_month_index;
    return legacy_energy_restore(handle->store, &sources, state);
}

static void record_pending(nd_energy_ledger_t *handle,
                           nd_energy_direction_t direction, uint8_t tariff,
                           uint32_t pulses)
{
    uint8_t side = direction == ND_ENERGY_REVERSE ? 1u : 0u;

    handle->pending[side][nd_energy_pulse_tariff(tariff)] += pulses;
}

static int persist(nd_energy_ledger_t *handle, metering_state_t *state);

/* Pulses counted while the base was not restored go on top of the recovered
 * book, never on top of the pre-recovery RAM book. */
static int apply_pending(nd_energy_ledger_t *handle, metering_state_t *state)
{
    int applied = 0;

    for (uint8_t side = 0; side < 2; side++) {
        for (uint8_t tariff = 1; tariff < ND_TARIFF_SLOTS; tariff++) {
            uint32_t pulses = handle->pending[side][tariff];

            if (pulses == 0u) {
                continue;
            }
            metering_add_pulses(state,
                                side != 0 ? ND_ENERGY_REVERSE
                                          : ND_ENERGY_FORWARD,
                                tariff, pulses);
            handle->pending[side][tariff] = 0u;
            applied = 1;
        }
    }
    return applied;
}

static metering_state_t *energy_state(nd_energy_ledger_t *handle)
{
    struct mbase_control_block_t *mbase_cb = handle->mbase_cb;
    int restored;

    if (mbase_cb == NULL) {
        return NULL;
    }
    if (energy == NULL) {
        metering_config_t config;

        config.meter_const = nd_mbase_meter_const(mbase_cb);
        config.cal_word = nd_mbase_active_energy_cal_word(mbase_cb);
        config.save_step = nd_mbase_EC_SaveDiff(mbase_cb);
        nd_arena_init(&state_arena, state_buffer, sizeof(state_buffer));
        energy = metering_create(nd_arena_allocator(&state_arena), &config);
        if (energy == NULL) {
            return NULL;
        }
    } else if (!handle->restore_pending) {
        return energy;
    }

    /* A failed base read leaves the book at zero; retry on every later call
     * so a transient EEPROM error cannot pin the meter at zero. A degraded
     * restore recovered the legacy mirror but may have regressed the total,
     * so it completes and only raises the storage fault. */
    restored = restore_book(handle, energy);
    if (restored < 0) {
        handle->restore_pending = 1;
        note_fault(handle);
        return energy;
    }
    if (restored > 0) {
        note_fault(handle);
    }
    handle->restore_pending = 0;
    if (apply_pending(handle, energy)) {
        /* The window crossed at least one save threshold; commit the merged
         * book so those pulses are durable before the next window. */
        sync_legacy_view(energy, mbase_cb);
        if (persist(handle, energy) != 0) {
            note_fault(handle);
        }
    } else {
        sync_legacy_view(energy, mbase_cb);
    }
    return energy;
}

static int persist(nd_energy_ledger_t *handle, metering_state_t *state)
{
    if (handle->store == NULL) {
        return 0;
    }
    if (handle->restore_pending) {
        /* Never commit a book whose base was not loaded: it would replace the
         * real total with a newer zero generation. */
        return -1;
    }
    return legacy_energy_persist(handle->store, state);
}

nd_energy_update_t metering_energy_add(nd_energy_ledger_t *ledger_handle,
                                       nd_energy_direction_t direction,
                                       uint32_t pulses)
{
    nd_energy_update_t update = {0};
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return update;
    }
    state = energy_state(ledger_handle);

    if (state == NULL) {
        return update;
    }

    if (ledger_handle->restore_pending) {
        /* The book is unknown, so it must not reach the legacy view or a
         * save threshold. Keep the pulses for the recovered book. */
        record_pending(ledger_handle, direction, nd_mbase_TOUNo(mbase_cb),
                       pulses);
        return update;
    }
    configure(state, mbase_cb);
    update = metering_add_pulses(state, direction, nd_mbase_TOUNo(mbase_cb),
                                 pulses);
    if (update.energy_changed || update.month_changed) {
        export_arrays(state, mbase_cb);
    }
    if (update.energy_save_due || update.month_save_due) {
        if (persist(ledger_handle, state) != 0) {
            note_fault(ledger_handle);
        }
    }
    return update;
}

int metering_energy_clear(nd_energy_ledger_t *ledger_handle)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return -1;
    }
    state = energy_state(ledger_handle);

    if (state == NULL || ledger_handle->restore_pending) {
        return -1;
    }
    configure(state, mbase_cb);
    metering_clear(state);
    sync_legacy_view(state, mbase_cb);
    if (persist(ledger_handle, state) != 0) {
        note_fault(ledger_handle);
        return -1;
    }
    return 0;
}

int metering_energy_clear_month(nd_energy_ledger_t *ledger_handle)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return -1;
    }
    state = energy_state(ledger_handle);

    if (state == NULL || ledger_handle->restore_pending) {
        return -1;
    }
    configure(state, mbase_cb);
    metering_clear_month(state);
    sync_legacy_view(state, mbase_cb);
    if (persist(ledger_handle, state) != 0) {
        note_fault(ledger_handle);
        return -1;
    }
    return 0;
}

int metering_energy_set_total(nd_energy_ledger_t *ledger_handle,
                              nd_energy_direction_t direction,
                              uint32_t integer)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return -1;
    }
    state = energy_state(ledger_handle);

    if (state == NULL || ledger_handle->restore_pending) {
        return -1;
    }
    configure(state, mbase_cb);
    metering_set_total(state, direction, integer);
    sync_legacy_view(state, mbase_cb);
    if (persist(ledger_handle, state) != 0) {
        note_fault(ledger_handle);
        return -1;
    }
    return 0;
}

int64_t metering_energy_read(nd_energy_ledger_t *ledger_handle,
                             nd_energy_direction_t direction, uint8_t tariff,
                             nd_energy_resolution_t resolution)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return 0;
    }
    state = energy_state(ledger_handle);

    if (state == NULL) {
        return 0;
    }
    configure(state, mbase_cb);
    return metering_read(state, direction, tariff, resolution);
}

int64_t metering_energy_read_combined(nd_energy_ledger_t *ledger_handle,
                                      uint8_t tariff,
                                      nd_energy_resolution_t resolution)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return 0;
    }
    state = energy_state(ledger_handle);

    if (state == NULL) {
        return 0;
    }
    configure(state, mbase_cb);
    return metering_read_combined(state, tariff, resolution);
}

int metering_energy_commit(stategrid_context_t *instance)
{
    nd_energy_ledger_t *handle;
    metering_state_t *state;

    if (instance == NULL) {
        return -1;
    }
    handle = metering_energy_ledger_from_instance(instance);
    if (handle->mbase_cb == NULL) {
        return -1;
    }
    state = energy_state(handle);
    if (state == NULL || handle->restore_pending) {
        return -1;
    }
    configure(state, handle->mbase_cb);
    sync_legacy_view(state, handle->mbase_cb);
    if (persist(handle, state) != 0) {
        note_fault(handle);
        return -1;
    }
    return 0;
}

int metering_energy_restore_pending(stategrid_context_t *instance)
{
    if (instance == NULL || instance->mbase == NULL) {
        return 0;
    }
    return metering_energy_ledger_from_instance(instance)->restore_pending;
}

void metering_energy_storage_fault_reported(stategrid_context_t *instance)
{
    if (instance != NULL) {
        instance->storage_fault = 0;
    }
}

void metering_energy_adapter_reset(void)
{
    energy = NULL;
    memset(&ledger, 0, sizeof(ledger));
}
