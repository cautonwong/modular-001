#ifndef ND_TARGET_HOST_COMPOSITION_H
#define ND_TARGET_HOST_COMPOSITION_H

#include <stdint.h>

#include "application/metering/application.h"
#include "domain/common/allocator.h"
#include "domain/common/parameter_store.h"
#include "domain/common/store.h"
#include "domain/load_control/load_control.h"
#include "domain/metering/demand.h"
#include "domain/metering/energy.h"
#include "domain/metering/metering.h"

typedef struct {
    nd_store_t *energy_store;
    uint32_t energy_block_offset;
    nd_parameter_store_t *parameters;
    metering_config_t metering;
    nd_demand_config_t demand;
} host_config_t;

typedef meter_tick_t host_tick_t;

typedef struct {
    uint8_t fee;
    uint8_t day_table;
    uint8_t relay_connected;
    uint32_t demand;
    int64_t energy_0_01_kwh;
} host_status_t;

typedef struct host_composition host_composition_t;

host_composition_t *host_composition_create(nd_allocator_t *allocator,
                                            const host_config_t *config);

void host_composition_destroy(host_composition_t *composition,
                              nd_allocator_t *allocator);

int host_composition_boot(host_composition_t *composition);

void host_composition_set_load_command(host_composition_t *composition,
                                       uint8_t command);

void host_composition_second(host_composition_t *composition,
                             const host_tick_t *tick);

int host_composition_save(host_composition_t *composition);

void host_composition_status(const host_composition_t *composition,
                             host_status_t *status);

#endif
