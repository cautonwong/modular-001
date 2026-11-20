#ifndef ND_TARGET_STATEGRID_DEMAND_ADAPTER_H
#define ND_TARGET_STATEGRID_DEMAND_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/allocator.h"

/*
 * The two ACC-MET-005 demand objects on the active-power source:
 *   0x11000201 interval: 5-minute block, publishes every 5 minutes.
 *   0x11000202 sliding:  1-minute slides, 5-slide window, publishes after 5.
 *
 * The OADs and the GET layout are placeholders until the product OAD table
 * lands; the product must replace them. Maxima are kept on a strict increase,
 * so a tie keeps the earlier maximum time.
 */
#define ND_DEMAND_OI 0x1100u
#define ND_DEMAND_OAD_INTERVAL 0x11000201u
#define ND_DEMAND_OAD_SLIDING 0x11000202u

typedef struct {
    uint32_t value;
    uint32_t fee_value; /* mirrors value until demand becomes tariff-aware */
    uint32_t max_value;
    uint32_t max_fee_value;
    uint32_t max_time;
    uint32_t start_time;
} nd_demand_view_t;

typedef struct nd_demand_adapter nd_demand_adapter_t;

nd_demand_adapter_t *nd_demand_adapter_create(nd_allocator_t *allocator,
                                              const int32_t *power);

void nd_demand_adapter_destroy(nd_demand_adapter_t *adapter,
                               nd_allocator_t *allocator);

/*
 * One wall-clock second. A repeated second is ignored, a gap restarts the
 * window, and the minute boundary is detected once per minute here.
 */
void nd_demand_adapter_on_second(nd_demand_adapter_t *adapter, uint32_t now);

/* Loads the persisted maxima and starts a fresh window. Returns 1 when the
 * stored block was valid; a bad version or CRC loads zero maxima and returns
 * 0 so boot continues. */
int nd_demand_adapter_restore(nd_demand_adapter_t *adapter,
                              const uint8_t *blank, size_t size, uint32_t now);

/* Writes the versioned maxima block. Returns 0, or -1 when the space is too
 * small for the block. */
int nd_demand_adapter_save(const nd_demand_adapter_t *adapter,
                           uint8_t *blank, size_t size);

/* Total clear: current demand and maxima return to zero. */
void nd_demand_adapter_clear(nd_demand_adapter_t *adapter, uint32_t now);

const nd_demand_view_t *nd_demand_adapter_view(const nd_demand_adapter_t *adapter,
                                               uint32_t oad);

#endif
