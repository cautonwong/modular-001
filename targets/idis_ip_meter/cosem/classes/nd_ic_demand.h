#ifndef ND_IC_DEMAND_H
#define ND_IC_DEMAND_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"
#include "domain/metering/demand_history.h"

#define ND_IDIS_DEMAND_MAX 6

typedef struct {
    uint32_t value; /* last average captured by next_period */
    uint8_t status;
    nd_datetime_t capture_time;
} nd_idis_average_power_t;

typedef struct {
    uint64_t sum;
    uint32_t samples;
} nd_idis_demand_acc_t;

typedef struct {
    nd_demand_snapshot_t current;
    nd_demand_history_t history;
} nd_idis_demand_state_t;

/* Adds one power sample to a group's period accumulator. */
void nd_idis_demand_accumulate(nd_idis_demand_acc_t *acc, int32_t power);

int nd_ic_demand_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                     uint8_t attribute_id,
                     const nd_axdr_reader_t *selector,
                     nd_axdr_writer_t *out);
int nd_ic_demand_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                        uint8_t method_id, nd_axdr_reader_t *in,
                        nd_axdr_writer_t *out);
int nd_ic_demand_save(const nd_idis_demand_state_t *states, nd_store_t *store,
                      uint32_t offset);
int nd_ic_demand_load(nd_idis_demand_state_t *states, nd_store_t *store,
                      uint32_t offset);

/* Last average demand register (class 4, 1-0:G.5.0.255). */
int nd_ic_demand_last_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id,
                          const nd_axdr_reader_t *selector,
                          nd_axdr_writer_t *out);
int nd_ic_demand_last_action(void *ctx, uint8_t client_sap,
                             const nd_obis_t *ln, uint8_t method_id,
                             nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
