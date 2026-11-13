#ifndef ND_IC_CLOCK_H
#define ND_IC_CLOCK_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/clock.h"
#include "domain/common/store.h"
#include "targets/idis_ip_meter/cosem/nd_clock_port.h"

typedef struct {
    const nd_clock_port_t *port;
    nd_clock_config_t config;
    nd_datetime_t preset_time;
    uint16_t measuring_period;
    uint16_t time_shift_limit; /* seconds, object 1-0:0.9.11.255, persisted */
} nd_idis_clock_state_t;

void nd_idis_clock_init(nd_idis_clock_state_t *state,
                        const nd_clock_port_t *port);
int nd_ic_clock_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, const nd_axdr_reader_t *selector,
                    nd_axdr_writer_t *out);
int nd_ic_clock_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_clock_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t method_id, nd_axdr_reader_t *in,
                       nd_axdr_writer_t *out);
int nd_idis_clock_save(const nd_idis_clock_state_t *state, nd_store_t *store,
                       uint32_t offset);
int nd_idis_clock_load(nd_idis_clock_state_t *state, nd_store_t *store,
                       uint32_t offset);

#endif
