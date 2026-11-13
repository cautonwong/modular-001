#ifndef ND_IC_DATA_H
#define ND_IC_DATA_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/events/power_failure.h"
#include "domain/common/store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event.h"

int nd_ic_data_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id,
                   const nd_axdr_reader_t *selector,
                   nd_axdr_writer_t *out);
int nd_ic_data_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_idis_power_save(const nd_power_failure_state_t *state,
                       nd_store_t *store, uint32_t offset);
int nd_idis_power_load(nd_power_failure_state_t *state, nd_store_t *store,
                       uint32_t offset);

#endif
