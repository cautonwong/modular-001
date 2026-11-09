#ifndef ND_DOMAIN_METERING_DEMAND_H
#define ND_DOMAIN_METERING_DEMAND_H

#include <stdint.h>

#include "domain/common/allocator.h"

/*
 * Sliding-window demand register. One instance owns one demand object.
 *
 * mode 0 updates demand and fee demand at every slide period.
 * mode 1 updates them only when the full demand period has elapsed.
 */
typedef struct {
    uint32_t max_slide;
    uint8_t sliding;
    uint8_t slide_num;
    uint8_t mode;
} nd_demand_config_t;

typedef struct nd_demand nd_demand_t;

typedef enum {
    ND_DEMAND_NONE = 0,
    ND_DEMAND_BOTH = 1,
    ND_DEMAND_VALUE = 2,
    ND_DEMAND_FEE = 3
} nd_demand_update_t;

nd_demand_t *nd_demand_create(nd_allocator_t *allocator,
                              const nd_demand_config_t *config);

void nd_demand_destroy(nd_demand_t *demand, nd_allocator_t *allocator);

void nd_demand_on_second(nd_demand_t *demand, int32_t power);

nd_demand_update_t nd_demand_on_minute(nd_demand_t *demand);

uint32_t nd_demand_value(const nd_demand_t *demand);

uint32_t nd_demand_fee_value(const nd_demand_t *demand);

#endif
