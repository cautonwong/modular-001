#include "support/pulse_harness.h"

#include <string.h>

#include "domain/metering/pulse_lane.h"

static nd_energy_channel_id_t channel_of(nd_energy_direction_t direction)
{
    return direction == ND_ENERGY_REVERSE ? ND_ENERGY_CHANNEL_ACTIVE_EXPORT
                                          : ND_ENERGY_CHANNEL_ACTIVE_IMPORT;
}

static const nd_energy_channel_t *channel_read(const nd_energy_set_t *set,
                                               nd_energy_direction_t direction)
{
    return direction == ND_ENERGY_REVERSE ? &set->active_export
                                          : &set->active_import;
}

static uint32_t slot_pulses(const nd_energy_channel_t *channel, uint8_t tariff)
{
    return channel->slots[tariff].integer * 100u
         + channel->slots[tariff].fraction;
}

static void apply(void *ctx, uint8_t lane, uint32_t count)
{
    nd_energy_set_t *set = ctx;

    (void)nd_energy_set_add_pulses(
        set, channel_of(nd_energy_pulse_lane_direction(lane)),
        nd_energy_pulse_lane_tariff(lane), count);
}

void pulse_set_init(nd_energy_set_t *set)
{
    nd_energy_set_init(set, 10000u);
}

int pulse_recover(nd_pulse_store_t *store, nd_energy_set_t *set)
{
    uint8_t blob[sizeof(nd_energy_set_t)];
    size_t length = 0;
    nd_pulse_status_t status;

    pulse_set_init(set);
    status = nd_pulse_store_base(store, blob, sizeof(blob), &length);
    if (status == ND_PULSE_OK) {
        if (length != sizeof(nd_energy_set_t)) {
            return -1;
        }
        memcpy(set, blob, sizeof(*set));
    } else if (status != ND_PULSE_EMPTY) {
        return -1;
    }
    status = nd_pulse_store_replay(store, apply, set);
    return status == ND_PULSE_OK || status == ND_PULSE_EMPTY ? 0 : -1;
}

void pulse_add(nd_energy_set_t *set, nd_energy_direction_t direction,
               uint8_t tariff)
{
    (void)nd_energy_set_add_pulses(set, channel_of(direction), tariff, 1u);
}

uint32_t pulse_total(const nd_energy_set_t *set)
{
    return slot_pulses(&set->active_import, ND_TARIFF_TOTAL)
         + slot_pulses(&set->active_export, ND_TARIFF_TOTAL);
}

uint32_t pulse_slot_count(const nd_energy_set_t *set,
                          nd_energy_direction_t direction, uint8_t tariff)
{
    return slot_pulses(channel_read(set, direction), tariff);
}
