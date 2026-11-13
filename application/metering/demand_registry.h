#ifndef ND_APPLICATION_METERING_DEMAND_REGISTRY_H
#define ND_APPLICATION_METERING_DEMAND_REGISTRY_H

#include <stdint.h>

#include "domain/common/allocator.h"
#include "domain/metering/demand.h"

typedef struct {
    const int32_t *power;
    uint32_t output_oad;
    nd_demand_config_t config;
} nd_demand_entry_config_t;

typedef void (*nd_demand_update_cb_t)(void *ctx, uint32_t output_oad,
                                      nd_demand_update_t update,
                                      uint32_t value, uint32_t fee_value);

typedef struct nd_demand_registry nd_demand_registry_t;

nd_demand_registry_t *nd_demand_registry_create(
    nd_allocator_t *allocator, const nd_demand_entry_config_t *entries,
    uint8_t count);

void nd_demand_registry_destroy(nd_demand_registry_t *registry,
                                nd_allocator_t *allocator);

void nd_demand_registry_on_second(nd_demand_registry_t *registry);

void nd_demand_registry_on_minute(nd_demand_registry_t *registry,
                                  nd_demand_update_cb_t callback, void *ctx);

#endif
