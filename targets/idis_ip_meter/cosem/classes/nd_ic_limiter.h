#ifndef ND_IC_LIMITER_H
#define ND_IC_LIMITER_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"

typedef struct {
    uint8_t script[6];
    uint16_t selector;
} nd_limiter_action_t;

typedef struct {
    nd_obis_t ln;
    uint16_t target_class;
    uint8_t target_ln[6];
    uint8_t target_attr;
    uint32_t threshold_active;
    uint32_t threshold_normal;
    uint32_t threshold_emergency;
    uint32_t min_over_duration;
    uint32_t min_under_duration;
    uint16_t emergency_profile_id;
    uint32_t emergency_duration;
    uint8_t emergency_active;
    nd_limiter_action_t up;
    nd_limiter_action_t down;
    uint32_t over_timer;
    uint32_t under_timer;
    uint8_t above;
    uint8_t above_valid;
} nd_limiter_state_t;

void nd_idis_limiter_init(nd_limiter_state_t *state);

int nd_ic_limiter_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                      uint8_t attribute_id, const nd_axdr_reader_t *selector,
                      nd_axdr_writer_t *out);
int nd_ic_limiter_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                      uint8_t attribute_id, nd_axdr_reader_t *in);

int nd_idis_limiter_on_second(nd_limiter_state_t *state,
                              const nd_cosem_port_t *port,
                              const nd_script_state_t *scripts,
                              uint8_t client_sap);

int nd_idis_limiter_save(const nd_limiter_state_t *state, nd_store_t *store,
                         uint32_t offset);
int nd_idis_limiter_load(nd_limiter_state_t *state, nd_store_t *store,
                         uint32_t offset);

#endif
