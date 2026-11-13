#include "targets/stategrid_meter/app/metering_energy_adapter.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

#include "nidavellir.h"

#include "domain/metering/metering.h"
#include "infra/memory/arena.h"
#include "kernel/legacy_control_block.h"

/* shortcut: one energy book per firmware. Move it into the target composition
 * root when the target owns instance state. */
static uint8_t state_buffer[512];
static nd_arena_t state_arena;
static metering_state_t *energy;

struct nd_energy_ledger
{
    struct mbase_control_block_t *mbase_cb;
};

static struct nd_energy_ledger ledger;

static struct mbase_control_block_t *control_block(struct instance_t *instance)
{
    return (struct mbase_control_block_t *)get_pipeline_ctx(instance,
                                                            pid_meter_base);
}

nd_energy_ledger_t *metering_energy_ledger_from_instance(
    struct instance_t *instance)
{
    ledger.mbase_cb = control_block(instance);
    return &ledger;
}

nd_energy_ledger_t *metering_energy_ledger_from_mbase(
    struct mbase_control_block_t *mbase_cb)
{
    ledger.mbase_cb = mbase_cb;
    return &ledger;
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

static metering_state_t *energy_state(struct mbase_control_block_t *mbase_cb)
{
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
        import_arrays(energy, mbase_cb);
    }
    return energy;
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
    state = energy_state(mbase_cb);

    if (state == NULL) {
        return update;
    }

    configure(state, mbase_cb);
    import_arrays(state, mbase_cb);
    update = metering_add_pulses(state, direction, nd_mbase_TOUNo(mbase_cb), pulses);
    export_arrays(state, mbase_cb);
    return update;
}

void metering_energy_clear(nd_energy_ledger_t *ledger_handle)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return;
    }
    state = energy_state(mbase_cb);

    if (state == NULL) {
        return;
    }
    configure(state, mbase_cb);
    import_arrays(state, mbase_cb);
    metering_clear(state);
    export_arrays(state, mbase_cb);
}

void metering_energy_set_total(nd_energy_ledger_t *ledger_handle,
                               nd_energy_direction_t direction,
                               uint32_t integer)
{
    struct mbase_control_block_t *mbase_cb = ledger_handle == NULL
        ? NULL : ledger_handle->mbase_cb;
    metering_state_t *state;

    if (mbase_cb == NULL) {
        return;
    }
    state = energy_state(mbase_cb);

    if (state == NULL) {
        return;
    }
    configure(state, mbase_cb);
    import_arrays(state, mbase_cb);
    metering_set_total(state, direction, integer);
    export_arrays(state, mbase_cb);
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
    state = energy_state(mbase_cb);

    if (state == NULL) {
        return 0;
    }
    configure(state, mbase_cb);
    import_arrays(state, mbase_cb);
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
    state = energy_state(mbase_cb);

    if (state == NULL) {
        return 0;
    }
    configure(state, mbase_cb);
    import_arrays(state, mbase_cb);
    return metering_read_combined(state, tariff, resolution);
}
