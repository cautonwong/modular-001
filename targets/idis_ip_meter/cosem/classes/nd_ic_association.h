#ifndef ND_IC_ASSOCIATION_H
#define ND_IC_ASSOCIATION_H

#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "components/protocol/dlms/include/nd_dlms_security.h"

#define ND_ASSOC_OBJECTS_MAX 8
#define ND_ASSOC_USERS_MAX 4
#define ND_ASSOC_USER_LEN 16
#define ND_SECURITY_CERT_MAX 64

typedef struct {
    uint16_t class_id;
    nd_obis_t ln;
    uint8_t attribute;
    uint16_t index;
} nd_assoc_object_t;

typedef struct {
    uint8_t object_count;
    nd_assoc_object_t objects[ND_ASSOC_OBJECTS_MAX];
    uint16_t client_sap;
    uint16_t server_sap;
    uint8_t app_context[16];
    uint8_t app_context_len;
    uint8_t secret[16];
    uint8_t secret_len;
    uint8_t status;
    nd_obis_t security_reference;
    uint8_t user_count;
    uint8_t users[ND_ASSOC_USERS_MAX][ND_ASSOC_USER_LEN];
    uint8_t user_len[ND_ASSOC_USERS_MAX];
} nd_assoc_state_t;

typedef struct {
    uint8_t policy;
    uint8_t suite;
    uint8_t client_title[8];
    uint8_t server_title[8];
    uint8_t cert[ND_SECURITY_CERT_MAX];
    uint8_t cert_len;
    uint8_t activated;
    uint8_t key_len[3];
    uint8_t keys[3][16];
} nd_security_state_t;

typedef struct {
    nd_assoc_state_t association; /* 0-0:40.0.0.255 */
    uint8_t sap_count;
    struct {
        uint8_t sap;
        nd_obis_t ln;
    } sap_list[ND_ASSOC_OBJECTS_MAX];
    nd_security_state_t security; /* 0-0:43.0.0.255 */
    nd_security_state_t consumer; /* 0-0:43.0.1.255 */
    const nd_crypto_port_t *crypto;
} nd_idis_association_t;

void nd_idis_association_init(nd_idis_association_t *state);
void nd_idis_association_bind_crypto(nd_idis_association_t *state,
                                     const nd_crypto_port_t *crypto);

int nd_ic_association_get(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id,
                          const nd_axdr_reader_t *selector,
                          nd_axdr_writer_t *out);
int nd_ic_association_set(void *ctx, uint8_t client_sap, const nd_obis_t *ln,
                          uint8_t attribute_id, nd_axdr_reader_t *in);
int nd_ic_association_action(void *ctx, uint8_t client_sap,
                             const nd_obis_t *ln, uint8_t method_id,
                             nd_axdr_reader_t *in, nd_axdr_writer_t *out);

#endif
