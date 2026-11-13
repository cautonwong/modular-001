#ifndef ND_IC_PUSH_H
#define ND_IC_PUSH_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "domain/common/store.h"
#include "domain/push/push.h"

#define ND_IDIS_PUSH_OBJECTS_MAX 16

/* Target-side push_object_list entry (class_id stays out of the domain). */
typedef struct {
    uint16_t class_id;
    nd_obis_t ln;
    uint8_t attribute_index;
    uint16_t data_index;
} nd_idis_push_object_t;

#define ND_IDIS_PUSH_WINDOWS_MAX 4
#define ND_IDIS_PUSH_WINDOW_LEN 8
#define ND_IDIS_PUSH_PROTECTION_MAX 4
#define ND_IDIS_PUSH_GATEWAY_LEN 16

typedef struct {
    uint8_t start_len;
    uint8_t end_len;
    uint8_t start[ND_IDIS_PUSH_WINDOW_LEN];
    uint8_t end[ND_IDIS_PUSH_WINDOW_LEN];
} nd_idis_push_window_t;

typedef struct {
    nd_push_state_t state;
    uint8_t object_count[ND_PUSH_SETUPS];
    nd_idis_push_object_t objects[ND_PUSH_SETUPS][ND_IDIS_PUSH_OBJECTS_MAX];
    uint8_t window_count[ND_PUSH_SETUPS];
    nd_idis_push_window_t windows[ND_PUSH_SETUPS][ND_IDIS_PUSH_WINDOWS_MAX];
    uint8_t protection_count[ND_PUSH_SETUPS];
    uint8_t protection_len[ND_PUSH_SETUPS][ND_IDIS_PUSH_PROTECTION_MAX];
    uint8_t protection[ND_PUSH_SETUPS][ND_IDIS_PUSH_PROTECTION_MAX]
                     [ND_IDIS_PUSH_WINDOW_LEN];
    uint8_t gateway_len[ND_PUSH_SETUPS];
    uint8_t gateway[ND_PUSH_SETUPS][ND_IDIS_PUSH_GATEWAY_LEN];
} nd_idis_push_t;

void nd_idis_push_init(nd_idis_push_t *push);

int nd_ic_push_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id, const nd_axdr_reader_t *selector,
                   nd_axdr_writer_t *out);
int nd_ic_push_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                   uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_push_action(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                      uint8_t method_id, nd_axdr_reader_t *in,
                      nd_axdr_writer_t *out);

int nd_idis_push_save(const nd_idis_push_t *push, nd_store_t *store,
                      uint32_t offset);
int nd_idis_push_load(nd_idis_push_t *push, nd_store_t *store,
                      uint32_t offset);

#endif
