#ifndef ND_IC_IMAGE_TRANSFER_H
#define ND_IC_IMAGE_TRANSFER_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"

#define ND_IDIS_IMAGE_BLOCKS_MAX 64

typedef struct {
    nd_obis_t ln;
    uint32_t block_size;
    uint32_t image_size;
    uint32_t first_missing;
    uint8_t block_count;
    uint8_t blocks[ND_IDIS_IMAGE_BLOCKS_MAX / 8];
    uint8_t enabled;
    uint8_t status;
    uint8_t verified;
    uint8_t id_len;
    uint8_t id[16];
    nd_store_t *store; /* optional block storage */
    uint32_t store_offset;
} nd_image_state_t;

void nd_idis_image_init(nd_image_state_t *state);
void nd_idis_image_bind_store(nd_image_state_t *state, nd_store_t *store,
                              uint32_t offset);

int nd_ic_image_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, const nd_axdr_reader_t *selector,
                    nd_axdr_writer_t *out);
int nd_ic_image_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t method_id, nd_axdr_reader_t *in,
                       nd_axdr_writer_t *out);

int nd_idis_image_save(const nd_image_state_t *state, nd_store_t *store,
                       uint32_t offset);
int nd_idis_image_load(nd_image_state_t *state, nd_store_t *store,
                       uint32_t offset);

#endif
