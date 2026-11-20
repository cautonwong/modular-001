#include "infra/storage/pulse_snapshot.h"

#include <stddef.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/crc32.h"
#include "infra/storage/pulse_store_ops.h"

#define ND_PULSE_SNAPSHOT_READ_CHUNK 32u

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t bytes;
    uint32_t generation;
} nd_pulse_snapshot_header_t;

_Static_assert(sizeof(nd_pulse_snapshot_header_t)
                   == ND_PULSE_SNAPSHOT_HEADER_BYTES,
               "snapshot header layout");

typedef struct nd_pulse_snapshot {
    nd_pulse_store_t port;
    const nd_block_ops_t *block;
    uint32_t offset;
    uint32_t blob_bytes;
    uint32_t record_bytes;
    uint32_t generation;
    uint8_t attached;
    uint8_t has_base;
    uint8_t fault;
    uint8_t slot;
} nd_pulse_snapshot_t;

_Static_assert(sizeof(nd_pulse_snapshot_t) <= ND_PULSE_SNAPSHOT_STORAGE_SIZE,
               "pulse snapshot storage too small");

static uint32_t load_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
         | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void store_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static int all_ff(const void *data, size_t length)
{
    const uint8_t *bytes = data;

    for (size_t index = 0; index < length; index++) {
        if (bytes[index] != 0xFFu) {
            return 0;
        }
    }
    return 1;
}

static uint32_t slot_offset(const nd_pulse_snapshot_t *snapshot, uint32_t slot)
{
    return snapshot->offset + slot * snapshot->record_bytes;
}

static uint32_t record_crc(const nd_pulse_snapshot_header_t *header,
                           const uint8_t *blob, uint32_t length)
{
    uint32_t crc = nd_crc32_update(ND_CRC32_INIT, (const uint8_t *)header,
                                   sizeof(*header));

    crc = nd_crc32_update(crc, blob, length);
    return nd_crc32_finish(crc);
}

/* 1 valid, 0 invalid, -1 device error. A header left blank by an erased
 * region is reported through *blank so a torn record can be told apart from
 * a first boot. */
static int read_record(nd_pulse_snapshot_t *snapshot, uint32_t slot,
                       uint8_t *buffer, uint32_t *generation, uint8_t *blank)
{
    nd_pulse_snapshot_header_t header;
    uint8_t tail[ND_PULSE_SNAPSHOT_CRC_BYTES];
    uint8_t chunk[ND_PULSE_SNAPSHOT_READ_CHUNK];
    uint32_t offset = slot_offset(snapshot, slot)
                    + ND_PULSE_SNAPSHOT_HEADER_BYTES;
    uint32_t remaining = snapshot->blob_bytes;
    uint32_t written = 0;
    uint32_t crc;

    if (nd_block_read(snapshot->block, slot_offset(snapshot, slot),
                      (uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }
    *blank = (uint8_t)all_ff(&header, sizeof(header));
    if (*blank) {
        return 0;
    }
    if (header.magic != ND_PULSE_SNAPSHOT_MAGIC
        || header.version != ND_PULSE_SNAPSHOT_VERSION
        || header.bytes != (uint16_t)snapshot->blob_bytes) {
        return 0;
    }
    crc = nd_crc32_update(ND_CRC32_INIT, (const uint8_t *)&header,
                          sizeof(header));
    while (remaining > 0u) {
        uint32_t take = remaining < sizeof(chunk) ? remaining
                                                  : (uint32_t)sizeof(chunk);

        if (nd_block_read(snapshot->block, offset, chunk, take) != 0) {
            return -1;
        }
        crc = nd_crc32_update(crc, chunk, take);
        if (buffer != NULL) {
            memcpy(buffer + written, chunk, take);
        }
        offset += take;
        written += take;
        remaining -= take;
    }
    if (nd_block_read(snapshot->block, offset, tail, sizeof(tail)) != 0) {
        return -1;
    }
    if (nd_crc32_finish(crc) != load_u32(tail)) {
        return 0;
    }
    *generation = header.generation;
    return 1;
}

/* OK when a base exists, EMPTY when the region was never written, ERROR when
 * a non-blank region holds no valid record, IO_ERROR when a device read
 * failed. A torn slot with the other slot valid is not an error: the valid
 * slot is the previous commit. */
static nd_pulse_status_t attach(nd_pulse_snapshot_t *snapshot)
{
    uint32_t best = 0;
    uint32_t best_slot = 0;
    int found = 0;
    int nonblank = 0;

    if (snapshot->attached) {
        if (snapshot->fault) {
            return ND_PULSE_ERROR;
        }
        return snapshot->has_base ? ND_PULSE_OK : ND_PULSE_EMPTY;
    }
    for (uint32_t slot = 0; slot < ND_PULSE_SNAPSHOT_SLOTS; slot++) {
        uint32_t generation = 0;
        uint8_t blank = 0;
        int valid = read_record(snapshot, slot, NULL, &generation, &blank);

        if (valid < 0) {
            /* Leave the engine unattached so a later call retries the scan. */
            return ND_PULSE_IO_ERROR;
        }
        if (!blank) {
            nonblank = 1;
        }
        if (valid > 0 && (found == 0 || generation > best)) {
            best = generation;
            best_slot = slot;
            found = 1;
        }
    }
    if (found) {
        snapshot->generation = best;
        snapshot->slot = (uint8_t)best_slot;
        snapshot->has_base = 1;
        snapshot->fault = 0;
        snapshot->attached = 1;
        return ND_PULSE_OK;
    }
    if (nonblank) {
        /* No valid record, but the region is not blank: report a fault and
         * keep reporting it, so a torn store never masquerades as EMPTY and
         * triggers legacy adoption. The generation resets so an explicit
         * commit can still write a fresh generation 1 over the damage. */
        snapshot->generation = 0;
        snapshot->has_base = 0;
        snapshot->fault = 1;
        snapshot->attached = 1;
        return ND_PULSE_ERROR;
    }
    snapshot->generation = 0;
    snapshot->has_base = 0;
    snapshot->fault = 0;
    snapshot->attached = 1;
    return ND_PULSE_EMPTY;
}

static nd_pulse_status_t snapshot_append(void *ctx, uint8_t lane)
{
    nd_pulse_snapshot_t *snapshot = ctx;
    nd_pulse_status_t status;

    if (snapshot == NULL || lane >= ND_PULSE_LANES) {
        return ND_PULSE_ERROR;
    }
    status = attach(snapshot);
    if (status != ND_PULSE_OK) {
        return ND_PULSE_ERROR;
    }
    return status;
}

static nd_pulse_status_t snapshot_replay(void *ctx, nd_pulse_replay_fn sink,
                                         void *sink_ctx)
{
    nd_pulse_snapshot_t *snapshot = ctx;
    nd_pulse_status_t status;

    (void)sink_ctx;
    if (snapshot == NULL || sink == NULL) {
        return ND_PULSE_ERROR;
    }
    status = attach(snapshot);
    return status == ND_PULSE_IO_ERROR ? ND_PULSE_ERROR : status;
}

static nd_pulse_status_t snapshot_base(void *ctx, uint8_t *buffer,
                                       size_t capacity, size_t *length)
{
    nd_pulse_snapshot_t *snapshot = ctx;
    nd_pulse_status_t status;
    uint32_t generation = 0;
    uint8_t blank = 0;

    if (snapshot == NULL || buffer == NULL || length == NULL) {
        return ND_PULSE_ERROR;
    }
    *length = 0;
    if (snapshot->blob_bytes > capacity) {
        *length = snapshot->blob_bytes;
        return ND_PULSE_FULL;
    }
    status = attach(snapshot);
    if (status != ND_PULSE_OK) {
        return status;
    }
    /* attach selected this slot with a successful scan; losing it now is a
     * read fault, not a torn store. The consumer must retry, never adopt. */
    if (read_record(snapshot, snapshot->slot, buffer, &generation, &blank) != 1
        || generation != snapshot->generation) {
        return ND_PULSE_IO_ERROR;
    }
    *length = snapshot->blob_bytes;
    return ND_PULSE_OK;
}

static nd_pulse_status_t snapshot_commit(void *ctx, const uint8_t *blob,
                                         size_t length)
{
    nd_pulse_snapshot_t *snapshot = ctx;
    nd_pulse_snapshot_header_t header;
    nd_pulse_status_t status;
    uint8_t tail[ND_PULSE_SNAPSHOT_CRC_BYTES];
    uint32_t next;
    uint32_t slot;
    uint32_t offset;
    uint32_t blob_offset;
    uint32_t generation = 0;
    uint8_t blank = 0;

    if (snapshot == NULL || blob == NULL
        || length != snapshot->blob_bytes) {
        return ND_PULSE_ERROR;
    }
    status = attach(snapshot);
    if (status == ND_PULSE_IO_ERROR) {
        /* A device read error is not a known state; a torn region is, and an
         * explicit commit may replace it with generation 1. */
        return ND_PULSE_IO_ERROR;
    }
    next = snapshot->generation + 1u;
    slot = next & 1u;
    offset = slot_offset(snapshot, slot);
    blob_offset = offset + (uint32_t)sizeof(header);
    header.magic = ND_PULSE_SNAPSHOT_MAGIC;
    header.version = ND_PULSE_SNAPSHOT_VERSION;
    header.bytes = (uint16_t)snapshot->blob_bytes;
    header.generation = next;
    store_u32(tail, record_crc(&header, blob, snapshot->blob_bytes));
    if (nd_block_program(snapshot->block, offset, (const uint8_t *)&header,
                         sizeof(header)) != 0
        || nd_block_program(snapshot->block, blob_offset, blob,
                            snapshot->blob_bytes) != 0
        || nd_block_program(snapshot->block,
                            blob_offset + snapshot->blob_bytes, tail,
                            sizeof(tail)) != 0) {
        return ND_PULSE_ERROR;
    }
    if (read_record(snapshot, slot, NULL, &generation, &blank) != 1
        || blank || generation != next) {
        return ND_PULSE_ERROR;
    }
    snapshot->generation = next;
    snapshot->slot = (uint8_t)slot;
    snapshot->has_base = 1;
    snapshot->fault = 0;
    snapshot->attached = 1;
    return ND_PULSE_OK;
}

static const nd_pulse_store_ops_t snapshot_ops = {
    .append = snapshot_append,
    .replay = snapshot_replay,
    .base = snapshot_base,
    .commit = snapshot_commit,
};

int nd_pulse_snapshot_init(nd_pulse_snapshot_storage_t *storage,
                           const nd_pulse_snapshot_config_t *config)
{
    nd_pulse_snapshot_t *snapshot = (nd_pulse_snapshot_t *)storage;
    uint32_t record_bytes;

    if (storage == NULL || config == NULL || config->block == NULL
        || config->block->read == NULL || config->block->program == NULL
        || config->blob_bytes == 0u
        || config->blob_bytes > 0xFFFFu) {
        return -1;
    }
    record_bytes = ND_PULSE_SNAPSHOT_RECORD_BYTES(config->blob_bytes);
    if (config->offset > config->block->size
        || record_bytes > (config->block->size - config->offset)
                              / ND_PULSE_SNAPSHOT_SLOTS) {
        return -1;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->port.ops = &snapshot_ops;
    snapshot->port.ctx = snapshot;
    snapshot->block = config->block;
    snapshot->offset = config->offset;
    snapshot->blob_bytes = config->blob_bytes;
    snapshot->record_bytes = record_bytes;
    return 0;
}

nd_pulse_store_t *nd_pulse_snapshot_port(nd_pulse_snapshot_storage_t *storage)
{
    nd_pulse_snapshot_t *snapshot = (nd_pulse_snapshot_t *)storage;

    return storage != NULL ? &snapshot->port : NULL;
}
