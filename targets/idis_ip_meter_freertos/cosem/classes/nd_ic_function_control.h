#ifndef ND_IC_FUNCTION_CONTROL_H
#define ND_IC_FUNCTION_CONTROL_H

#include "components/protocol/dlms/include/nd_cosem_port.h"

#define ND_FUNCTION_CONTROL_MAX 8
#define ND_FUNCTION_NAME_MAX 16

typedef struct {
    uint8_t length;
    char name[ND_FUNCTION_NAME_MAX];
    uint8_t enabled;
} nd_function_status_t;

typedef struct {
    uint8_t count;
    nd_function_status_t functions[ND_FUNCTION_CONTROL_MAX];
} nd_function_control_state_t;

typedef struct {
    nd_function_control_state_t local; /* 0-0:44.1.0.255 */
    nd_function_control_state_t mbus;  /* 0-1:44.1.0.255 */
} nd_function_control_t;

void nd_ic_function_control_init(nd_function_control_t *state);

int nd_ic_function_control_get(void *ctx, uint8_t client_sap,
                               const nd_obis_t *ln, uint8_t attribute_id,
                               const nd_axdr_reader_t *selector,
                               nd_axdr_writer_t *out);
int nd_ic_function_control_action(void *ctx, uint8_t client_sap,
                                  const nd_obis_t *ln, uint8_t method_id,
                                  nd_axdr_reader_t *in,
                                  nd_axdr_writer_t *out);

#endif
