#include "application/metering/demand_registry.h"

#include <stddef.h>

struct nd_demand_entry {
    nd_demand_t *demand;
    const int32_t *power;
    uint32_t output_oad;
};

struct nd_demand_registry {
    uint8_t count;
    struct nd_demand_entry entries[];
};

static size_t registry_size(uint8_t count)
{
    return sizeof(struct nd_demand_registry)
           + (size_t)count * sizeof(struct nd_demand_entry);
}

nd_demand_registry_t *nd_demand_registry_create(
    nd_allocator_t *allocator, const nd_demand_entry_config_t *entries,
    uint8_t count)
{
    if (allocator == NULL || entries == NULL || count == 0) {
        return NULL;
    }

    nd_demand_registry_t *registry = nd_alloc(allocator, registry_size(count));
    if (registry == NULL) {
        return NULL;
    }
    registry->count = count;

    for (uint8_t index = 0; index < count; index++) {
        registry->entries[index].power = entries[index].power;
        registry->entries[index].output_oad = entries[index].output_oad;
        registry->entries[index].demand =
            nd_demand_create(allocator, &entries[index].config);
        if (registry->entries[index].demand == NULL) {
            for (uint8_t created = 0; created < index; created++) {
                nd_demand_destroy(registry->entries[created].demand, allocator);
            }
            nd_dealloc(allocator, registry, registry_size(count));
            return NULL;
        }
    }
    return registry;
}

void nd_demand_registry_destroy(nd_demand_registry_t *registry,
                                nd_allocator_t *allocator)
{
    if (registry == NULL) {
        return;
    }
    for (uint8_t index = 0; index < registry->count; index++) {
        nd_demand_destroy(registry->entries[index].demand, allocator);
    }
    nd_dealloc(allocator, registry, registry_size(registry->count));
}

void nd_demand_registry_on_second(nd_demand_registry_t *registry)
{
    if (registry == NULL) {
        return;
    }
    for (uint8_t index = 0; index < registry->count; index++) {
        const int32_t *power = registry->entries[index].power;
        if (power != NULL) {
            nd_demand_on_second(registry->entries[index].demand, *power);
        }
    }
}

void nd_demand_registry_on_minute(nd_demand_registry_t *registry,
                                  nd_demand_update_cb_t callback, void *ctx)
{
    if (registry == NULL) {
        return;
    }
    for (uint8_t index = 0; index < registry->count; index++) {
        const struct nd_demand_entry *entry = &registry->entries[index];
        nd_demand_update_t update = nd_demand_on_minute(entry->demand);

        if (update != ND_DEMAND_NONE && callback != NULL) {
            callback(ctx, entry->output_oad, update,
                     nd_demand_value(entry->demand),
                     nd_demand_fee_value(entry->demand));
        }
    }
}
