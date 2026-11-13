#ifndef ND_IDIS_PROFILE_H
#define ND_IDIS_PROFILE_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/clock_value.h"
#include "targets/idis_ip_meter/cosem/nd_clock_port.h"
#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#define ND_IDIS_PROFILE_MAX_CAPTURES 12
#define ND_IDIS_PROFILE_RECORD_MAX 256

typedef struct {
    uint16_t class_id;
    nd_obis_t ln;
    uint8_t attribute_id;
    uint16_t data_index;
} nd_capture_object_t;

typedef struct {
    nd_profile_store_t store;
    nd_obis_t ln;
    const nd_clock_port_t *clock;
    const nd_cosem_port_t *port;
    nd_capture_object_t captures[ND_IDIS_PROFILE_MAX_CAPTURES];
    uint8_t capture_count;
    uint32_t capture_period; /* seconds, 0 = manual only */
    uint8_t compressed;
    uint8_t delta;
    uint8_t status; /* profile status captured as a normal column */
    void (*after_capture)(void *ctx);
    void *after_capture_ctx;
} nd_idis_profile_state_t;

int nd_idis_profile_init(nd_idis_profile_state_t *profile, nd_store_t *store,
                         uint32_t base_offset, uint16_t capacity,
                         uint16_t slot_size, const nd_obis_t *ln,
                         const nd_clock_port_t *clock,
                         const nd_capture_object_t *captures,
                         uint8_t capture_count, uint32_t capture_period,
                         uint8_t compressed, uint8_t delta);

void nd_idis_profile_set_status(nd_idis_profile_state_t *profile,
                                uint8_t status);
int nd_idis_profile_capture(nd_idis_profile_state_t *profile,
                            const nd_cosem_port_t *port, uint8_t client_sap,
                            const nd_clock_value_t *now);

int nd_idis_profile_on_second(nd_idis_profile_state_t *profile,
                              const nd_cosem_port_t *port, uint8_t client_sap,
                              const nd_clock_value_t *now);

#endif
