#include "targets/idis_ip_meter_freertos/cosem/nd_idis_pq.h"

#include <string.h>

#define ND_IDIS_PQ_MAGIC 0x50513131u

typedef struct {
    uint32_t magic;
    nd_pq_config_t config;
} nd_idis_pq_record_t;

void nd_idis_pq_init(nd_pq_state_t *state, const nd_pq_config_t *config)
{
    nd_pq_init(state, config);
}

static uint16_t event_code(nd_event_kind_t kind, uint8_t phase)
{
    uint16_t offset = phase >= 1 && phase <= 3 ? (uint16_t)(phase - 1) : 0;
    switch (kind) {
    case ND_EVENT_VOLTAGE_SAG:
        return (uint16_t)(76 + offset); /* Undervoltage L1..L3 */
    case ND_EVENT_VOLTAGE_SWELL:
        return (uint16_t)(79 + offset); /* Overvoltage L1..L3 */
    case ND_EVENT_VOLTAGE_MISSING:
        return (uint16_t)(82 + offset); /* Missing voltage L1..L3 */
    case ND_EVENT_FREQUENCY_LOW:
        return 92;
    case ND_EVENT_FREQUENCY_HIGH:
        return 93;
    case ND_EVENT_VOLTAGE_UNBALANCE:
        return 90;
    default:
        return 0;
    }
}

int nd_idis_pq_on_second(nd_pq_state_t *state,
                         const nd_measurement_port_t *measurement,
                         nd_idis_event_state_t *events,
                         const nd_datetime_t *now)
{
    if (state == NULL || measurement == NULL || measurement->read == NULL
        || events == NULL || now == NULL) {
        return -1;
    }
    nd_measurement_value_t voltage[3];
    for (uint8_t phase = 0; phase < 3; phase++) {
        if (measurement->read(measurement->ctx, ND_MEASURE_VOLTAGE,
                              (uint8_t)(phase + 1), &voltage[phase]) != 0) {
            return -1;
        }
    }
    nd_measurement_value_t frequency;
    if (measurement->read(measurement->ctx, ND_MEASURE_FREQUENCY,
                          ND_MEASURE_PHASE_TOTAL, &frequency) != 0) {
        return -1;
    }
    nd_event_record_t records[5];
    uint8_t count = nd_pq_on_second(state, voltage, &frequency, now, records,
                                    5);
    for (uint8_t index = 0; index < count; index++) {
        uint16_t code = event_code(records[index].kind, records[index].source);
        if (code != 0) {
            (void)nd_idis_event_raise_kind(
                events, records[index].kind, code, records[index].source,
                records[index].severity, records[index].duration_seconds,
                records[index].value);
        }
    }
    return count;
}

int nd_idis_pq_save(const nd_pq_state_t *state, nd_store_t *store,
                    uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_pq_record_t record = {
        .magic = ND_IDIS_PQ_MAGIC,
        .config = state->config,
    };
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_pq_load(nd_pq_state_t *state, nd_store_t *store, uint32_t offset)
{
    if (state == NULL || store == NULL) {
        return -1;
    }
    nd_idis_pq_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_PQ_MAGIC) {
        return -1;
    }
    state->config = record.config;
    return 0;
}
