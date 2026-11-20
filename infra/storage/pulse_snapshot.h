#ifndef ND_INFRA_STORAGE_PULSE_SNAPSHOT_H
#define ND_INFRA_STORAGE_PULSE_SNAPSHOT_H

#include <stdint.h>

#include "domain/metering/pulse_store.h"
#include "infra/storage/block.h"

#define ND_PULSE_SNAPSHOT_STORAGE_SIZE 48u
#define ND_PULSE_SNAPSHOT_SLOTS 2u
#define ND_PULSE_SNAPSHOT_MAGIC 0x4E445053u /* "NDPS" */
#define ND_PULSE_SNAPSHOT_VERSION 1u
#define ND_PULSE_SNAPSHOT_HEADER_BYTES 12u
#define ND_PULSE_SNAPSHOT_CRC_BYTES 4u
/* Slots alternate; the stride is 4-byte aligned so the second header is
 * aligned like the first. */
#define ND_PULSE_SNAPSHOT_RECORD_BYTES(blob_bytes)                           \
    ((ND_PULSE_SNAPSHOT_HEADER_BYTES + (uint32_t)(blob_bytes)                \
      + ND_PULSE_SNAPSHOT_CRC_BYTES + 3u)                                    \
     & ~3u)

typedef union {
    uint64_t align;
    uint8_t bytes[ND_PULSE_SNAPSHOT_STORAGE_SIZE];
} nd_pulse_snapshot_storage_t;

/* The fallback for products without the ROM for the journal. append advances
 * nothing; commit rewrites the whole record into the alternate slot with a
 * generation counter. The loss window is every pulse since the previous
 * commit. Slot 1 starts at offset + record stride. */
typedef struct {
    const nd_block_ops_t *block;
    uint32_t offset;
    uint32_t blob_bytes;
} nd_pulse_snapshot_config_t;

int nd_pulse_snapshot_init(nd_pulse_snapshot_storage_t *storage,
                           const nd_pulse_snapshot_config_t *config);
nd_pulse_store_t *nd_pulse_snapshot_port(nd_pulse_snapshot_storage_t *storage);

#endif
