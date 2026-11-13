#ifndef ND_IC_SETUP_H
#define ND_IC_SETUP_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"

#define ND_SETUP_BLOB_MAX 48
#define ND_SETUP_MAX 162

typedef struct {
    uint32_t number; /* scalar value */
    uint8_t length;
    uint8_t blob[ND_SETUP_BLOB_MAX];
} nd_setup_value_t;

typedef struct {
    nd_store_t *store;
    uint32_t offset;
} nd_idis_setup_t;

void nd_idis_setup_init(nd_idis_setup_t *setup);
void nd_idis_setup_bind(nd_idis_setup_t *setup, nd_store_t *store,
                        uint32_t offset);

int nd_ic_setup_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, const nd_axdr_reader_t *selector,
                    nd_axdr_writer_t *out);
int nd_ic_setup_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                    uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_setup_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                       uint8_t method_id, nd_axdr_reader_t *in,
                       nd_axdr_writer_t *out);

int nd_idis_setup_save(const nd_idis_setup_t *setup, nd_store_t *store,
                       uint32_t offset);
int nd_idis_setup_load(nd_idis_setup_t *setup, nd_store_t *store,
                       uint32_t offset);

#endif
