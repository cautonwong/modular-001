#include "application/metering/application.h"

void meter_application_init(meter_application_t *application,
                            metering_state_t *metering, nd_demand_t *demand,
                            load_control_state_t *load,
                            const nd_tariff_table_t *tariff)
{
    application->metering = metering;
    application->demand = demand;
    application->load = load;
    application->tariff = tariff;
    application->status.fee = 0;
    application->status.day_table = 0;
    application->status.relay_connected = 1;
}

void meter_application_on_second(meter_application_t *application,
                                 const meter_tick_t *tick)
{
    application->status.day_table =
        nd_tariff_select_day_table(application->tariff, &tick->time);
    application->status.fee = nd_tariff_select_fee(application->tariff,
                                                   application->status.day_table,
                                                   tick->time.hour,
                                                   tick->time.minute);

    if (tick->pulses != 0) {
        metering_add_pulses(application->metering, tick->direction,
                            application->status.fee, tick->pulses);
    }

    nd_demand_on_second(application->demand, tick->power);
    if (tick->time.second == 0) {
        nd_demand_on_minute(application->demand);
    }

    nd_load_actions_t actions;
    load_control_second(application->load, &tick->load, &actions);
    if (actions.connect) {
        application->status.relay_connected = 1;
    }
    if (actions.disconnect) {
        application->status.relay_connected = 0;
    }
}

const meter_status_t *meter_application_status(
    const meter_application_t *application)
{
    return &application->status;
}
