#include "domain/metering/demand.h"

#include <string.h>

struct nd_demand {
    nd_demand_config_t config;
    uint64_t cumulative_seconds;
    uint32_t cumulative_minute;
    uint32_t seconds;
    uint32_t value;
    uint32_t fee_value;
    uint8_t minute_count;
    uint8_t slide_index;
    uint8_t slide_count;
    uint8_t fee_count;
    uint32_t slide[];
};

static uint32_t magnitude(int32_t value)
{
    return value < 0 ? (uint32_t)(-(value + 1)) + 1u : (uint32_t)value;
}

static size_t demand_size(uint32_t max_slide)
{
    return sizeof(struct nd_demand) + (size_t)max_slide * sizeof(uint32_t);
}

nd_demand_t *nd_demand_create(nd_allocator_t *allocator,
                              const nd_demand_config_t *config)
{
    if (config == NULL || config->max_slide == 0 || config->sliding == 0) {
        return NULL;
    }
    if (config->slide_num == 0 || config->slide_num > config->max_slide) {
        return NULL;
    }

    nd_demand_t *demand = nd_alloc(allocator, demand_size(config->max_slide));
    if (demand == NULL) {
        return NULL;
    }
    demand->config = *config;
    return demand;
}

void nd_demand_destroy(nd_demand_t *demand, nd_allocator_t *allocator)
{
    if (demand == NULL) {
        return;
    }
    nd_dealloc(allocator, demand, demand_size(demand->config.max_slide));
}

void nd_demand_on_second(nd_demand_t *demand, int32_t power)
{
    demand->cumulative_seconds += magnitude(power);
    demand->seconds++;
}

static uint32_t recent_average(const nd_demand_t *demand, uint8_t count)
{
    uint32_t sum = 0;
    uint8_t index = demand->slide_index;

    for (uint8_t taken = 0; taken < count; taken++) {
        index = (uint8_t)((index + demand->config.max_slide - 1) % demand->config.max_slide);
        sum += demand->slide[index];
    }
    return sum / demand->config.slide_num;
}

nd_demand_update_t nd_demand_on_minute(nd_demand_t *demand)
{
    if (demand->seconds != 0) {
        demand->cumulative_minute += (uint32_t)(demand->cumulative_seconds / demand->seconds);
        demand->cumulative_seconds = 0;
        demand->seconds = 0;
    }

    demand->minute_count++;
    if (demand->minute_count < demand->config.sliding) {
        return ND_DEMAND_NONE;
    }

    demand->slide[demand->slide_index] = demand->cumulative_minute / demand->config.sliding;
    demand->minute_count = 0;
    demand->cumulative_minute = 0;
    demand->slide_index = (uint8_t)((demand->slide_index + 1) % demand->config.max_slide);

    if (demand->slide_count < demand->config.slide_num) {
        demand->slide_count++;
    }
    if (demand->fee_count < demand->config.slide_num) {
        demand->fee_count++;
    }

    nd_demand_update_t update = ND_DEMAND_BOTH;
    if (demand->config.mode == 1) {
        update = ND_DEMAND_NONE;
        if (demand->slide_count == demand->config.slide_num
            && demand->fee_count == demand->config.slide_num) {
            update = ND_DEMAND_BOTH;
        } else if (demand->slide_count == demand->config.slide_num) {
            update = ND_DEMAND_VALUE;
        } else if (demand->fee_count == demand->config.slide_num) {
            update = ND_DEMAND_FEE;
        }
    }

    if (update == ND_DEMAND_BOTH || update == ND_DEMAND_VALUE) {
        demand->value = recent_average(demand, demand->slide_count);
    }
    if (update == ND_DEMAND_BOTH || update == ND_DEMAND_FEE) {
        demand->fee_value = recent_average(demand, demand->fee_count);
    }
    return update;
}

uint32_t nd_demand_value(const nd_demand_t *demand)
{
    return demand->value;
}

uint32_t nd_demand_fee_value(const nd_demand_t *demand)
{
    return demand->fee_value;
}

int nd_demand_snapshot(const nd_demand_t *demand, nd_demand_snapshot_t *out)
{
    if (demand == NULL || out == NULL) {
        return -1;
    }
    out->current_value = demand->value;
    out->current_fee_value = demand->fee_value;
    out->period_seconds = (uint32_t)demand->config.sliding * 60u;
    out->periods = demand->config.slide_num;
    return 0;
}

int nd_demand_reset(nd_demand_t *demand)
{
    if (demand == NULL) {
        return -1;
    }
    demand->cumulative_seconds = 0;
    demand->cumulative_minute = 0;
    demand->seconds = 0;
    demand->value = 0;
    demand->fee_value = 0;
    demand->minute_count = 0;
    demand->slide_index = 0;
    demand->slide_count = 0;
    demand->fee_count = 0;
    memset(demand->slide, 0,
           (size_t)demand->config.max_slide * sizeof(demand->slide[0]));
    return 0;
}

int nd_demand_next_period(nd_demand_t *demand)
{
    if (demand == NULL) {
        return -1;
    }
    return (int)nd_demand_on_minute(demand);
}
