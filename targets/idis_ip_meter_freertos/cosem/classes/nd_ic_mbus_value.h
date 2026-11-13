#ifndef ND_IC_MBUS_VALUE_H
#define ND_IC_MBUS_VALUE_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/calendar.h"

#define ND_IDIS_MBUS_VALUES_PER_CHANNEL 8

/* Backing state of one M-Bus value object (Extended register, class 4). */
typedef struct {
    uint32_t value;
    uint8_t status;
    int8_t scaler;
    uint8_t unit;
    nd_datetime_t capture_time;
} nd_idis_mbus_value_t;

/* Returns 1 when ln is 0-X:24.2.Y.255 with X 1..4 and Y 1..8. */
int nd_ic_mbus_value_matches(const nd_obis_t *ln);

int nd_ic_mbus_value_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id,
                         const nd_axdr_reader_t *selector,
                         nd_axdr_writer_t *out);
int nd_ic_mbus_value_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                         uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_mbus_value_action(void *ctx, uint8_t client_sap,
                            const nd_obis_t *ln, uint8_t method_id,
                            nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
