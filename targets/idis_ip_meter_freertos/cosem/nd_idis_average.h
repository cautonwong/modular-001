#ifndef ND_IDIS_AVERAGE_H
#define ND_IDIS_AVERAGE_H

#include "domain/metering/measurement_average.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_measurement_port.h"

#define ND_IDIS_AVERAGE_CHANNELS 6

typedef struct {
    nd_measurement_average_t channels[ND_IDIS_AVERAGE_CHANNELS];
    const nd_measurement_port_t *measurement;
} nd_idis_average_state_t;

void nd_idis_average_init(nd_idis_average_state_t *state,
                          const nd_measurement_port_t *measurement);
int nd_idis_average_on_second(nd_idis_average_state_t *state);
int nd_idis_average_get(const nd_idis_average_state_t *state, uint8_t channel,
                        nd_measurement_value_t *out);
void nd_idis_average_reset(nd_idis_average_state_t *state);

#endif
