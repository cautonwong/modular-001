#ifndef ND_IDIS_COSEM_PORT_H
#define ND_IDIS_COSEM_PORT_H

#include "components/protocol/dlms/include/nd_cosem_port.h"

/* IDIS Pack3 client SAPs: management has full access, public is GET-only,
 * pre-established is SET/ACTION, CIP has no request access. */
#define ND_IDIS_SAP_PUBLIC 16u
#define ND_IDIS_SAP_MANAGEMENT 1u
#define ND_IDIS_SAP_PREESTABLISHED 102u
#define ND_IDIS_SAP_CIP 103u

int nd_cosem_port_get(void *ctx, uint8_t client_sap, uint16_t class_id,
                      const nd_obis_t *ln, uint8_t attribute_id,
                      const nd_axdr_reader_t *selector,
                      nd_axdr_writer_t *out);
int nd_cosem_port_set(void *ctx, uint8_t client_sap, uint16_t class_id,
                      const nd_obis_t *ln, uint8_t attribute_id,
                      nd_axdr_reader_t *in);
int nd_cosem_port_action(void *ctx, uint8_t client_sap, uint16_t class_id,
                         const nd_obis_t *ln, uint8_t method_id,
                         nd_axdr_reader_t *in, nd_axdr_writer_t *out);

/* Local factory maintenance entry, not a COSEM method. Class-7 method 1 has
 * no parameters, so a front end verifies the operator confirmation itself and
 * calls this directly. The management subject with a local client is the only
 * caller: any other SAP or a remote association is refused. */
int nd_cosem_port_release_legacy(void *ctx, uint8_t client_sap,
                                 uint32_t confirmation);

#endif
