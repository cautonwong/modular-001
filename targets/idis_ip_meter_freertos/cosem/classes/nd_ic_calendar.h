#ifndef ND_IC_CALENDAR_H
#define ND_IC_CALENDAR_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"
#include "domain/tariff/activity_calendar.h"

#define ND_IDIS_CAL_MAX_SEASONS 4
#define ND_IDIS_CAL_MAX_WEEKS 4
#define ND_IDIS_CAL_MAX_DAYS 8
#define ND_IDIS_CAL_MAX_ACTIONS 4

typedef struct {
    nd_calendar_name_t name;
    nd_calendar_season_t seasons[ND_IDIS_CAL_MAX_SEASONS];
    nd_calendar_week_t weeks[ND_IDIS_CAL_MAX_WEEKS];
    nd_calendar_day_t days[ND_IDIS_CAL_MAX_DAYS];
    nd_calendar_action_t actions[ND_IDIS_CAL_MAX_DAYS][ND_IDIS_CAL_MAX_ACTIONS];
    uint8_t season_count;
    uint8_t week_count;
    uint8_t day_count;
    uint8_t action_count[ND_IDIS_CAL_MAX_DAYS];
} nd_idis_calendar_bank_t;

#define ND_IDIS_SPECIAL_DAYS_MAX 16

typedef struct {
    nd_activity_calendar_t calendar;
    nd_idis_calendar_bank_t active;
    nd_idis_calendar_bank_t passive;
    nd_calendar_special_day_t special_days[ND_IDIS_SPECIAL_DAYS_MAX];
    uint16_t special_index[ND_IDIS_SPECIAL_DAYS_MAX];
    uint8_t special_day_count;
} nd_idis_calendar_state_t;

void nd_idis_calendar_init(nd_idis_calendar_state_t *state);

/* Installs a one-season/one-week/one-day tariff calendar that selects the
 * given script at 00:00:00 every day. */
void nd_idis_calendar_apply_defaults(nd_idis_calendar_state_t *state,
                                     const nd_obis_t *script_ln,
                                     uint16_t selector);
int nd_ic_calendar_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out);
int nd_ic_calendar_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_calendar_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t method_id, nd_axdr_reader_t *in,
                          nd_axdr_writer_t *out);
int nd_idis_calendar_tick(nd_idis_calendar_state_t *state,
                          const nd_datetime_t *now);
int nd_idis_calendar_save(const nd_idis_calendar_state_t *state,
                          nd_store_t *store, uint32_t offset);
int nd_idis_calendar_load(nd_idis_calendar_state_t *state, nd_store_t *store,
                          uint32_t offset);
int nd_ic_special_days_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                           uint8_t attribute_id,
                           const nd_axdr_reader_t *selector,
                           nd_axdr_writer_t *out);
int nd_ic_special_days_action(void *ctx, uint8_t client_sap,
                              const nd_obis_t *ln, uint8_t method_id,
                              nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
