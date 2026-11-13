#ifndef ND_IC_REGISTER_MONITOR_H
#define ND_IC_REGISTER_MONITOR_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_script_table.h"

#define ND_IDIS_MONITOR_MAX 5
#define ND_IDIS_MONITOR_THRESHOLDS 4

typedef struct {
    uint8_t script[6];
    uint16_t selector;
} nd_monitor_action_t;

typedef struct {
    nd_obis_t ln;
    uint16_t target_class;
    uint8_t target_ln[6];
    uint8_t target_attr;
    uint8_t threshold_count;
    uint32_t thresholds[ND_IDIS_MONITOR_THRESHOLDS];
    nd_monitor_action_t up[ND_IDIS_MONITOR_THRESHOLDS];
    nd_monitor_action_t down[ND_IDIS_MONITOR_THRESHOLDS];
    uint32_t last_value;
    uint8_t last_valid;
    int8_t last_level;
} nd_monitor_state_t;

typedef struct {
    nd_monitor_state_t items[ND_IDIS_MONITOR_MAX];
    uint8_t count;
} nd_idis_monitors_t;

void nd_idis_monitors_init(nd_idis_monitors_t *state);

int nd_ic_register_monitor_get(void *ctx, uint8_t client_sap,
                               const nd_obis_t *ln, uint8_t attribute_id,
                               const nd_axdr_reader_t *selector,
                               nd_axdr_writer_t *out);
int nd_ic_register_monitor_set(void *ctx, uint8_t client_sap,
                               const nd_obis_t *ln, uint8_t attribute_id,
                               nd_axdr_reader_t *in);

/* Evaluates thresholds and runs crossing scripts; returns executions. */
int nd_idis_monitor_on_second(nd_idis_monitors_t *state,
                              const nd_cosem_port_t *port,
                              const nd_script_state_t *scripts,
                              uint8_t client_sap);

int nd_idis_monitor_save(const nd_idis_monitors_t *state, nd_store_t *store,
                         uint32_t offset);
int nd_idis_monitor_load(nd_idis_monitors_t *state, nd_store_t *store,
                         uint32_t offset);

#endif
