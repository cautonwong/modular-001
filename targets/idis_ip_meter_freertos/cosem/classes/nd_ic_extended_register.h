#ifndef ND_IC_EXTENDED_REGISTER_H
#define ND_IC_EXTENDED_REGISTER_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/calendar.h"
#include "domain/common/store.h"

#define ND_IDIS_MAX_DEMAND 30

typedef struct {
    uint32_t value;
    uint8_t status;
    nd_datetime_t capture_time;
} nd_idis_extended_state_t;

int nd_ic_extended_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out);
/* group 1..4 or 9/10, rate 0..4; 1 when the stored maximum changed. */
int nd_ic_extended_action(void *ctx, uint8_t client_sap,
                          const nd_obis_t *ln, uint8_t method_id,
                          nd_axdr_reader_t *in, nd_axdr_writer_t *out);
int nd_idis_extended_update(nd_idis_extended_state_t *states, uint8_t group,
                            uint8_t rate, uint32_t value, uint8_t status,
                            const nd_datetime_t *capture_time);
int nd_ic_extended_save(const nd_idis_extended_state_t *states,
                        nd_store_t *store, uint32_t offset);
int nd_ic_extended_load(nd_idis_extended_state_t *states, nd_store_t *store,
                        uint32_t offset);

#endif
