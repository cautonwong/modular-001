#include "domain/metering/metering.h"

struct metering_state {
    nd_energy_book_t energy;
};

metering_state_t *metering_create(nd_allocator_t *allocator,
                                  const metering_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }

    metering_state_t *state = nd_alloc(allocator, sizeof(*state));
    if (state == NULL) {
        return NULL;
    }

    nd_energy_book_init(&state->energy, config->meter_const, config->cal_word,
                        config->save_step);
    return state;
}

void metering_destroy(metering_state_t *state, nd_allocator_t *allocator)
{
    nd_dealloc(allocator, state, sizeof(*state));
}

void metering_configure(metering_state_t *state, const metering_config_t *config)
{
    if (config == NULL) {
        return;
    }
    state->energy.meter_const = config->meter_const;
    state->energy.cal_word = config->cal_word;
    state->energy.save_step = config->save_step;
}

nd_energy_update_t metering_add_pulses(metering_state_t *state,
                                       nd_energy_direction_t direction,
                                       uint8_t tariff, uint32_t pulses)
{
    return nd_energy_add_pulses(&state->energy, direction, tariff, pulses);
}

int64_t metering_read(const metering_state_t *state,
                      nd_energy_direction_t direction, uint8_t tariff,
                      nd_energy_resolution_t resolution)
{
    return nd_energy_read(&state->energy, direction, tariff, resolution);
}

int64_t metering_read_combined(const metering_state_t *state, uint8_t tariff,
                               nd_energy_resolution_t resolution)
{
    return nd_energy_read_combined(&state->energy, tariff, resolution);
}

void metering_clear(metering_state_t *state)
{
    nd_energy_clear(&state->energy);
}

void metering_set_total(metering_state_t *state, nd_energy_direction_t direction,
                        uint32_t integer)
{
    nd_energy_set_total(&state->energy, direction, integer);
}

void metering_export(const metering_state_t *state,
                     nd_metering_snapshot_t *snapshot)
{
    snapshot->forward = state->energy.forward;
    snapshot->reverse = state->energy.reverse;
    snapshot->month = state->energy.month;
}

void metering_import(metering_state_t *state,
                     const nd_metering_snapshot_t *snapshot)
{
    state->energy.forward = snapshot->forward;
    state->energy.reverse = snapshot->reverse;
    state->energy.month = snapshot->month;
}
