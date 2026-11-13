#ifndef ND_TEST_FILE_STORE_H
#define ND_TEST_FILE_STORE_H

#include <stdint.h>

#include "domain/common/store.h"

/*
 * Host-side file-backed nd_store_t. Mirrors the flat byte-addressed EEPROM and
 * NOR flash images used by the target: a fixed size, erased value 0xFF, and
 * absolute-offset reads and writes with bounds checks.
 */
typedef struct {
    nd_store_t port;
    int fd;
    uint32_t size;
    uint8_t erase_value;
} file_store_t;

/* Opens or creates path. A new or resized file is filled with erase_value. */
int file_store_open(file_store_t *store, const char *path, uint32_t size,
                    uint8_t erase_value);

void file_store_close(file_store_t *store);

nd_store_t *file_store_port(file_store_t *store);

/* Fills [offset, offset + length) with erase_value. */
int file_store_erase(file_store_t *store, uint32_t offset, uint32_t length);

#endif
