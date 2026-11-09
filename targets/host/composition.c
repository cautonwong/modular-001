#include "targets/host/composition.h"

#include "targets/stategrid_meter/persistence/legacy_energy_store.h"
#include "targets/stategrid_meter/persistence/legacy_tariff_parser.h"

struct host_composition {
    nd_store_t *energy_store;
    nd_parameter_store_t *parameters;
    uint32_t energy_block_offset;
    metering_state_t *metering;
    nd_demand_t *demand;
    load_control_state_t *load;
    nd_tariff_table_t tariff;
    uint8_t fee;
    uint8_t day_table;
    uint8_t relay_connected;
};

host_composition_t *host_composition_create(nd_allocator_t *allocator,
                                            const host_config_t *config)
{
    if (config == NULL || config->energy_store == NULL || config->parameters == NULL) {
        return NULL;
    }

    host_composition_t *composition = nd_alloc(allocator, sizeof(*composition));
    if (composition == NULL) {
        return NULL;
    }

    composition->metering = metering_create(allocator, &config->metering);
    if (composition->metering == NULL) {
        nd_dealloc(allocator, composition, sizeof(*composition));
        return NULL;
    }

    composition->demand = nd_demand_create(allocator, &config->demand);
    if (composition->demand == NULL) {
        metering_destroy(composition->metering, allocator);
        nd_dealloc(allocator, composition, sizeof(*composition));
        return NULL;
    }

    composition->load = load_control_create(allocator);
    if (composition->load == NULL) {
        nd_demand_destroy(composition->demand, allocator);
        metering_destroy(composition->metering, allocator);
        nd_dealloc(allocator, composition, sizeof(*composition));
        return NULL;
    }

    composition->energy_store = config->energy_store;
    composition->parameters = config->parameters;
    composition->energy_block_offset = config->energy_block_offset;
    composition->relay_connected = 1;
    nd_tariff_table_init(&composition->tariff);
    return composition;
}

void host_composition_destroy(host_composition_t *composition,
                              nd_allocator_t *allocator)
{
    if (composition == NULL) {
        return;
    }
    load_control_destroy(composition->load, allocator);
    nd_demand_destroy(composition->demand, allocator);
    metering_destroy(composition->metering, allocator);
    nd_dealloc(allocator, composition, sizeof(*composition));
}

int host_composition_boot(host_composition_t *composition)
{
    if (legacy_energy_load(composition->energy_store, composition->energy_block_offset,
                           composition->metering) != 0) {
        return -1;
    }
    if (legacy_tariff_load(composition->parameters, &composition->tariff) != 0) {
        return -1;
    }
    return 0;
}

void host_composition_set_load_command(host_composition_t *composition,
                                       uint8_t command)
{
    load_control_set_command(composition->load, command);
}

void host_composition_second(host_composition_t *composition,
                             const host_tick_t *tick)
{
    composition->day_table =
        nd_tariff_select_day_table(&composition->tariff, &tick->time);
    composition->fee = nd_tariff_select_fee(&composition->tariff,
                                            composition->day_table,
                                            tick->time.hour, tick->time.minute);

    if (tick->pulses != 0) {
        metering_add_pulses(composition->metering, tick->direction,
                            composition->fee, tick->pulses);
    }

    nd_demand_on_second(composition->demand, tick->power);
    if (tick->time.second == 0) {
        nd_demand_on_minute(composition->demand);
    }

    nd_load_actions_t actions;
    load_control_second(composition->load, &tick->load, &actions);
    if (actions.connect) {
        composition->relay_connected = 1;
    }
    if (actions.disconnect) {
        composition->relay_connected = 0;
    }
}

int host_composition_save(host_composition_t *composition)
{
    return legacy_energy_save(composition->energy_store,
                              composition->energy_block_offset,
                              composition->metering);
}

void host_composition_status(const host_composition_t *composition,
                             host_status_t *status)
{
    status->fee = composition->fee;
    status->day_table = composition->day_table;
    status->relay_connected = composition->relay_connected;
    status->demand = nd_demand_value(composition->demand);
    status->energy_0_01_kwh = metering_read(composition->metering,
                                            ND_ENERGY_FORWARD, 0,
                                            ND_ENERGY_RES_0_01_KWH);
}
