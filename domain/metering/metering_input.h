#ifndef ND_METERING_INPUT_H
#define ND_METERING_INPUT_H

#include <stdint.h>

typedef enum nd_metering_input {
    ND_METERING_INPUT_TEMPERATURE,
    ND_METERING_INPUT_BATTERY
} nd_metering_input_t;

typedef struct nd_metering_input_port {
    void *ctx;
    int (*read)(void *ctx, nd_metering_input_t input, int32_t *value);
} nd_metering_input_port_t;

int nd_metering_input_read(const nd_metering_input_port_t *port,
                           nd_metering_input_t input, int32_t *value);

#endif
