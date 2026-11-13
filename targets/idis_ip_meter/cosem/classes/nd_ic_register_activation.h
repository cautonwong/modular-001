#ifndef ND_IC_REGISTER_ACTIVATION_H
#define ND_IC_REGISTER_ACTIVATION_H

#include "components/protocol/dlms/include/nd_cosem_port.h"

#define ND_ACTIVE_MASK_MAX 8

#define ND_ACTIVATION_MASKS_MAX 8
#define ND_ACTIVATION_MASK_LEN 8

typedef struct {
    uint8_t length;
    uint8_t mask[ND_ACTIVE_MASK_MAX];
    uint8_t mask_count; /* mask_list entries */
    uint8_t mask_len[ND_ACTIVATION_MASKS_MAX];
    uint8_t masks[ND_ACTIVATION_MASKS_MAX][ND_ACTIVATION_MASK_LEN];
    uint8_t assign_count; /* register_assignment entries */
    nd_obis_t assign[ND_ACTIVATION_MASKS_MAX];
} nd_register_activation_state_t;

typedef struct {
    nd_register_activation_state_t energy; /* 0-0:14.0.1.255 */
    nd_register_activation_state_t demand; /* 0-0:14.0.2.255 */
} nd_register_activation_t;

void nd_ic_register_activation_init(nd_register_activation_t *state);

/* First active-mask digit ("1".."4"); defaults to 1 when unset. */
uint8_t nd_ic_register_activation_tariff(
    const nd_register_activation_t *state);

int nd_ic_register_activation_get(void *ctx, uint8_t client_sap,
                                  const nd_obis_t *ln, uint8_t attribute_id,
                                  const nd_axdr_reader_t *selector,
                                  nd_axdr_writer_t *out);
int nd_ic_register_activation_set(void *ctx, uint8_t client_sap,
                                  const nd_obis_t *ln, uint8_t attribute_id,
                                  nd_axdr_reader_t *in);

#endif
