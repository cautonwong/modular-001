#ifndef ND_INFRA_MEMORY_PARAMETER_STORE_H
#define ND_INFRA_MEMORY_PARAMETER_STORE_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/parameter_store.h"

typedef struct {
    uint16_t id;
    uint8_t *data;
    size_t length;
} nd_param_record_t;

typedef struct {
    nd_parameter_store_t port;
    nd_param_record_t *records;
    size_t count;
} nd_param_table_t;

void nd_param_table_init(nd_param_table_t *table, nd_param_record_t *records,
                         size_t count);

nd_parameter_store_t *nd_param_table_port(nd_param_table_t *table);

#endif
