#ifndef ND_APPLICATION_METERING_APPLICATION_H
#define ND_APPLICATION_METERING_APPLICATION_H

#include <stdint.h>

#include "domain/common/calendar.h"
#include "domain/load_control/load_control.h"
#include "domain/metering/demand.h"
#include "domain/metering/energy.h"
#include "domain/metering/metering.h"
#include "domain/tariff/tariff.h"

typedef struct {
    nd_energy_direction_t direction;
    uint32_t pulses;
    int32_t power;
    nd_datetime_t time;
    nd_load_inputs_t load;
} meter_tick_t;

typedef struct {
    uint8_t fee;
    uint8_t day_table;
    uint8_t relay_connected;
} meter_status_t;

typedef struct {
    metering_state_t *metering;
    nd_demand_t *demand;
    load_control_state_t *load;
    const nd_tariff_table_t *tariff;
    meter_status_t status;
} meter_application_t;

void meter_application_init(meter_application_t *application,
                            metering_state_t *metering, nd_demand_t *demand,
                            load_control_state_t *load,
                            const nd_tariff_table_t *tariff);

void meter_application_on_second(meter_application_t *application,
                                 const meter_tick_t *tick);

const meter_status_t *meter_application_status(
    const meter_application_t *application);

#endif
