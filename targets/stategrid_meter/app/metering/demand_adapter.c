#include "targets/stategrid_meter/app/metering/demand_adapter.h"

#include <string.h>

#include "application/metering/demand_registry.h"

#define ND_DEMAND_COUNT 2u
#define ND_DEMAND_PERSIST_VERSION 1u

typedef struct {
    uint8_t version;
    uint8_t reserved;
    uint16_t crc;
    uint32_t max_value[ND_DEMAND_COUNT];
    uint32_t max_fee_value[ND_DEMAND_COUNT];
    uint32_t max_time[ND_DEMAND_COUNT];
} nd_demand_persist_t;

/* The only free space in the frozen persistent section, persistent_blank_space[4*9]. */
_Static_assert(sizeof(nd_demand_persist_t) <= 36u,
               "demand maxima must fit the persistent blank space");

struct nd_demand_adapter {
    nd_allocator_t *allocator;
    nd_demand_registry_t *registry;
    nd_demand_view_t views[ND_DEMAND_COUNT];
    uint32_t now;
    uint32_t last_second;
    uint32_t last_minute;
    uint8_t time_valid;
};

/* CRC-16/X-25, the same convention as the 698 frame check. */
static uint16_t demand_crc16(const uint8_t *data, size_t size)
{
    uint16_t crc = 0xFFFFu;

    while (size-- != 0u) {
        crc ^= *data++;
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            crc = (crc & 1u) != 0u ? (uint16_t)((crc >> 1) ^ 0x8408u)
                                   : (uint16_t)(crc >> 1);
        }
    }
    return (uint16_t)(crc ^ 0xFFFFu);
}

static int view_index(uint32_t oad)
{
    if (oad == ND_DEMAND_OAD_INTERVAL) {
        return 0;
    }
    if (oad == ND_DEMAND_OAD_SLIDING) {
        return 1;
    }
    return -1;
}

static void reset_window(nd_demand_adapter_t *adapter, uint32_t now)
{
    nd_demand_registry_reset(adapter->registry);
    for (uint8_t index = 0u; index < ND_DEMAND_COUNT; index++) {
        adapter->views[index].value = 0;
        adapter->views[index].fee_value = 0;
        adapter->views[index].start_time = now;
    }
}

static void on_update(void *ctx, uint32_t oad, nd_demand_update_t update,
                      uint32_t value, uint32_t fee_value)
{
    nd_demand_adapter_t *adapter = ctx;
    int index = view_index(oad);
    nd_demand_view_t *view;

    if (index < 0) {
        return;
    }
    view = &adapter->views[index];
    if (update == ND_DEMAND_BOTH || update == ND_DEMAND_VALUE) {
        view->value = value;
        if (value > view->max_value) {
            view->max_value = value;
            view->max_time = adapter->now;
        }
    }
    if (update == ND_DEMAND_BOTH || update == ND_DEMAND_FEE) {
        view->fee_value = fee_value;
        if (fee_value > view->max_fee_value) {
            view->max_fee_value = fee_value;
        }
    }
    view->start_time = adapter->now;
}

nd_demand_adapter_t *nd_demand_adapter_create(nd_allocator_t *allocator,
                                              const int32_t *power)
{
    nd_demand_entry_config_t entries[ND_DEMAND_COUNT];
    nd_demand_adapter_t *adapter;

    if (allocator == NULL || power == NULL) {
        return NULL;
    }
    adapter = nd_alloc(allocator, sizeof(*adapter));
    if (adapter == NULL) {
        return NULL;
    }
    adapter->allocator = allocator;

    entries[0].power = power;
    entries[0].output_oad = ND_DEMAND_OAD_INTERVAL;
    entries[0].config.max_slide = 1;
    entries[0].config.sliding = 5;
    entries[0].config.slide_num = 1;
    entries[0].config.mode = 0;

    entries[1].power = power;
    entries[1].output_oad = ND_DEMAND_OAD_SLIDING;
    entries[1].config.max_slide = 5;
    entries[1].config.sliding = 1;
    entries[1].config.slide_num = 5;
    entries[1].config.mode = 1;

    adapter->registry =
        nd_demand_registry_create(allocator, entries, ND_DEMAND_COUNT);
    if (adapter->registry == NULL) {
        nd_dealloc(allocator, adapter, sizeof(*adapter));
        return NULL;
    }
    return adapter;
}

void nd_demand_adapter_destroy(nd_demand_adapter_t *adapter,
                               nd_allocator_t *allocator)
{
    if (adapter == NULL) {
        return;
    }
    nd_demand_registry_destroy(adapter->registry, allocator);
    nd_dealloc(allocator, adapter, sizeof(*adapter));
}

void nd_demand_adapter_on_second(nd_demand_adapter_t *adapter, uint32_t now)
{
    uint32_t minute;

    if (adapter == NULL) {
        return;
    }
    if (adapter->time_valid && now == adapter->last_second) {
        return;
    }
    if (adapter->time_valid && now != adapter->last_second + 1u) {
        reset_window(adapter, now);
        adapter->last_minute = now / 60u;
    } else if (!adapter->time_valid) {
        reset_window(adapter, now);
        adapter->last_minute = now / 60u;
        adapter->time_valid = 1;
    }
    adapter->last_second = now;
    adapter->now = now;

    minute = now / 60u;
    if (minute != adapter->last_minute) {
        adapter->last_minute = minute;
        nd_demand_registry_on_minute(adapter->registry, on_update, adapter);
    }
    nd_demand_registry_on_second(adapter->registry);
}

int nd_demand_adapter_restore(nd_demand_adapter_t *adapter,
                              const uint8_t *blank, size_t size, uint32_t now)
{
    nd_demand_persist_t persisted;
    uint16_t crc;
    int valid = 0;

    if (adapter == NULL) {
        return 0;
    }
    memset(&persisted, 0, sizeof(persisted));
    if (blank != NULL && size >= sizeof(persisted)) {
        memcpy(&persisted, blank, sizeof(persisted));
        crc = persisted.crc;
        persisted.crc = 0;
        valid = persisted.version == ND_DEMAND_PERSIST_VERSION
                && demand_crc16((const uint8_t *)&persisted,
                                sizeof(persisted)) == crc;
    }

    reset_window(adapter, now);
    for (uint8_t index = 0u; index < ND_DEMAND_COUNT; index++) {
        adapter->views[index].max_value =
            valid ? persisted.max_value[index] : 0;
        adapter->views[index].max_fee_value =
            valid ? persisted.max_fee_value[index] : 0;
        adapter->views[index].max_time = valid ? persisted.max_time[index] : 0;
    }
    adapter->time_valid = 1;
    adapter->last_second = now;
    adapter->last_minute = now / 60u;
    adapter->now = now;
    return valid;
}

int nd_demand_adapter_save(const nd_demand_adapter_t *adapter,
                           uint8_t *blank, size_t size)
{
    nd_demand_persist_t persisted;

    if (adapter == NULL || blank == NULL || size < sizeof(persisted)) {
        return -1;
    }
    memset(&persisted, 0, sizeof(persisted));
    persisted.version = ND_DEMAND_PERSIST_VERSION;
    for (uint8_t index = 0u; index < ND_DEMAND_COUNT; index++) {
        persisted.max_value[index] = adapter->views[index].max_value;
        persisted.max_fee_value[index] = adapter->views[index].max_fee_value;
        persisted.max_time[index] = adapter->views[index].max_time;
    }
    persisted.crc =
        demand_crc16((const uint8_t *)&persisted, sizeof(persisted));
    memcpy(blank, &persisted, sizeof(persisted));
    return 0;
}

void nd_demand_adapter_clear(nd_demand_adapter_t *adapter, uint32_t now)
{
    if (adapter == NULL) {
        return;
    }
    reset_window(adapter, now);
    for (uint8_t index = 0u; index < ND_DEMAND_COUNT; index++) {
        adapter->views[index].max_value = 0;
        adapter->views[index].max_fee_value = 0;
        adapter->views[index].max_time = 0;
    }
}

const nd_demand_view_t *nd_demand_adapter_view(const nd_demand_adapter_t *adapter,
                                               uint32_t oad)
{
    int index;

    if (adapter == NULL) {
        return NULL;
    }
    index = view_index(oad);
    return index < 0 ? NULL : &adapter->views[index];
}
