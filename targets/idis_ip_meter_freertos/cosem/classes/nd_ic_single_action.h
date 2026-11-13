#ifndef ND_IC_SINGLE_ACTION_H
#define ND_IC_SINGLE_ACTION_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/calendar.h"
#include "domain/common/store.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_script_table.h"

#define ND_SINGLE_ACTION_MAX 10
#define ND_SINGLE_ACTION_TIMES_MAX 8

typedef struct {
    uint8_t time[4];
    uint8_t time_len;
    uint8_t date[5];
    uint8_t date_len;
} nd_single_action_time_t;

typedef struct {
    nd_obis_t ln;
    uint8_t script_ln[6];
    uint16_t selector;
    uint8_t type; /* 1..5 per Blue Book */
    uint8_t time_count;
    nd_single_action_time_t times[ND_SINGLE_ACTION_TIMES_MAX];
    uint8_t executed_valid;
    uint16_t executed_year;
    uint8_t executed_month;
    uint8_t executed_day;
    uint8_t executed_hour;
    uint8_t executed_minute;
    uint8_t executed_second;
    uint8_t executed_index;
} nd_single_action_state_t;

typedef struct {
    nd_single_action_state_t items[ND_SINGLE_ACTION_MAX];
    uint8_t count;
} nd_single_action_t;

void nd_idis_single_action_init(nd_single_action_t *state);

/* Installs the product default: capture billing on day 1 at 00:00:00 when
 * the billing single-action schedule has no script or execution time. */
void nd_idis_single_action_apply_defaults(nd_single_action_t *state);

int nd_ic_single_action_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                            uint8_t attribute_id,
                            const nd_axdr_reader_t *selector,
                            nd_axdr_writer_t *out);
int nd_ic_single_action_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                            uint8_t attribute_id, nd_axdr_reader_t *in);

/* Runs scripts whose execution time matches the current second, once. */
int nd_idis_single_action_on_second(nd_single_action_t *state,
                                    const nd_cosem_port_t *port,
                                    const nd_script_state_t *scripts,
                                    uint8_t client_sap,
                                    const nd_datetime_t *now);

int nd_idis_single_action_save(const nd_single_action_t *state,
                               nd_store_t *store, uint32_t offset);
int nd_idis_single_action_load(nd_single_action_t *state, nd_store_t *store,
                               uint32_t offset);

#endif
