#ifndef ND_INFRA_STORAGE_PULSE_STORE_H
#define ND_INFRA_STORAGE_PULSE_STORE_H

#include <stdint.h>

#include "domain/metering/pulse_store.h"
#include "infra/storage/block.h"

#define ND_PULSE_STORE_STORAGE_SIZE 96u
#define ND_PULSE_PROGRAM_GRANULARITY 1u

typedef union {
    uint64_t align;
    uint8_t bytes[ND_PULSE_STORE_STORAGE_SIZE];
} nd_pulse_store_storage_t;

typedef struct {
    const nd_block_ops_t *block;      /* journal device */
    const nd_block_ops_t *base_block; /* base slots; NULL means block */
    uint32_t journal_offset; /* region start, aligned to block->erase_unit */
    uint32_t journal_bytes;  /* whole region, multiple of block->erase_unit */
    uint32_t base_offset[2]; /* two alternating base slots */
    uint32_t base_capacity;  /* bytes per base slot */
    uint8_t granularity;     /* 1..8; N delays a program, losing up to N-1 */
} nd_pulse_store_config_t;

/* Returns 0 when the geometry fits the block and the port may be used. The
 * config is copied, so a stack literal is fine. */
int nd_pulse_store_init(nd_pulse_store_storage_t *storage,
                        const nd_pulse_store_config_t *config);
nd_pulse_store_t *nd_pulse_store_port(nd_pulse_store_storage_t *storage);

#endif
