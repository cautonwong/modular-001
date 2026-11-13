#ifndef ND_INFRA_MEMORY_FILE_STORE_H
#define ND_INFRA_MEMORY_FILE_STORE_H

#include <stdint.h>
#include <stdio.h>

#include "domain/common/store.h"

typedef struct {
    nd_store_t port;
    FILE *file;
    uint32_t size;
} nd_file_store_t;

int nd_file_store_open(nd_file_store_t *store, const char *path, uint32_t size);
int nd_file_store_close(nd_file_store_t *store);
nd_store_t *nd_file_store_port(nd_file_store_t *store);

#endif
