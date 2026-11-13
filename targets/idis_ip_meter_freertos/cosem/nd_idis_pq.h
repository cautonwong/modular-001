#ifndef ND_IDIS_PQ_H
#define ND_IDIS_PQ_H

#include "domain/common/store.h"
#include "domain/events/pq.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_event.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_measurement_port.h"

void nd_idis_pq_init(nd_pq_state_t *state, const nd_pq_config_t *config);
int nd_idis_pq_on_second(nd_pq_state_t *state,
                         const nd_measurement_port_t *measurement,
                         nd_idis_event_state_t *events,
                         const nd_datetime_t *now);
int nd_idis_pq_save(const nd_pq_state_t *state, nd_store_t *store,
                    uint32_t offset);
int nd_idis_pq_load(nd_pq_state_t *state, nd_store_t *store, uint32_t offset);

#endif
