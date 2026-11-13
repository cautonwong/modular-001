#include "targets/idis_ip_meter_freertos/cosem/nd_idis_average.h"

#include <string.h>

static void channel_quantity(uint8_t channel, nd_measure_quantity_t *quantity,
                             uint8_t *phase)
{
    if (channel < 3) {
        *quantity = ND_MEASURE_VOLTAGE;
        *phase = (uint8_t)(channel + 1);
    } else {
        *quantity = ND_MEASURE_CURRENT;
        *phase = (uint8_t)(channel - 2);
    }
}

void nd_idis_average_init(nd_idis_average_state_t *state,
                          const nd_measurement_port_t *measurement)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->measurement = measurement;
    for (uint8_t index = 0; index < ND_IDIS_AVERAGE_CHANNELS; index++) {
        nd_measurement_average_init(&state->channels[index]);
    }
}

int nd_idis_average_on_second(nd_idis_average_state_t *state)
{
    if (state == NULL || state->measurement == NULL
        || state->measurement->read == NULL) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_AVERAGE_CHANNELS; index++) {
        nd_measure_quantity_t quantity;
        uint8_t phase;
        channel_quantity(index, &quantity, &phase);
        nd_measurement_value_t value;
        if (state->measurement->read(state->measurement->ctx, quantity, phase,
                                     &value) != 0
            || nd_measurement_average_add(&state->channels[index], &value) != 0) {
            return -1;
        }
    }
    return 0;
}

int nd_idis_average_get(const nd_idis_average_state_t *state, uint8_t channel,
                        nd_measurement_value_t *out)
{
    if (state == NULL || out == NULL || channel >= ND_IDIS_AVERAGE_CHANNELS) {
        return -1;
    }
    return nd_measurement_average_value(&state->channels[channel], out);
}

void nd_idis_average_reset(nd_idis_average_state_t *state)
{
    if (state == NULL) {
        return;
    }
    for (uint8_t index = 0; index < ND_IDIS_AVERAGE_CHANNELS; index++) {
        nd_measurement_average_reset(&state->channels[index]);
    }
}
