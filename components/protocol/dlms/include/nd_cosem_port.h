#ifndef ND_COSEM_PORT_H
#define ND_COSEM_PORT_H

#include <stdint.h>

#include "components/protocol/dlms/include/nd_axdr.h"

typedef enum {
    ND_OK = 0,
    ND_ERR_OBJECT_UNDEFINED = 2,
    ND_ERR_ACCESS_DENIED = 3,
    ND_ERR_ATTRIBUTE_UNDEFINED = 6,
    ND_ERR_TYPE_MISMATCH = 8,
    ND_ERR_OPERATION_NOT_POSSIBLE = 9
} nd_cosem_result_t;

/*
 * Consumer-defined COSEM port. DLMS passes the wire fields through; the target
 * implementation dispatches by class_id and never exposes an object table.
 * selector is NULL for a normal GET, otherwise it holds the Green Book
 * Selective-Access-Descriptor: structure { access-selector, access-parameters }.
 */
typedef struct nd_cosem_port {
    void *ctx;
    int (*get)(void *ctx, uint8_t client_sap, uint16_t class_id,
               const nd_obis_t *ln, uint8_t attribute_id,
               const nd_axdr_reader_t *selector, nd_axdr_writer_t *out);
    int (*set)(void *ctx, uint8_t client_sap, uint16_t class_id,
               const nd_obis_t *ln, uint8_t attribute_id,
               nd_axdr_reader_t *in);
    int (*action)(void *ctx, uint8_t client_sap, uint16_t class_id,
                  const nd_obis_t *ln, uint8_t method_id,
                  nd_axdr_reader_t *in, nd_axdr_writer_t *out);
} nd_cosem_port_t;

#endif
