#ifndef ND_IC_REGISTER_H
#define ND_IC_REGISTER_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/events/power_failure.h"
#include "domain/metering/energy_set.h"
#include "targets/idis_ip_meter_freertos/cosem/classes/nd_ic_clock.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_average.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_pq.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_measurement_port.h"

typedef struct {
    const nd_measurement_port_t *measurement;
    nd_energy_set_t *energy;
    const nd_idis_average_state_t *average;
    nd_idis_clock_state_t *clock;
    nd_pq_state_t *pq;
    nd_power_failure_state_t *power;
    uint32_t measurement_period_3;
    uint8_t phases; /* 0 = all configured phases, 1 = single-phase */
} nd_idis_register_state_t;

int nd_ic_pq_stat_matches(const nd_obis_t *ln);

int nd_ic_register_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id,
                       const nd_axdr_reader_t *selector,
                       nd_axdr_writer_t *out);
int nd_ic_register_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_register_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t method_id, nd_axdr_reader_t *in,
                          nd_axdr_writer_t *out);

#endif
