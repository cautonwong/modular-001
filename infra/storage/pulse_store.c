#include "infra/storage/pulse_store.h"

#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/crc32.h"
#include "infra/storage/pulse_store_ops.h"

#define ND_PULSE_JOURNAL_MAGIC 0x4E44504Au /* "NDPJ" */
#define ND_PULSE_BASE_MAGIC 0x4E445042u    /* "NDPB" */
#define ND_PULSE_FORMAT_VERSION 1u
#define ND_PULSE_BASE_HEADER_BYTES 12u
#define ND_PULSE_CRC_BYTES 4u
#define ND_PULSE_BASE_OVERHEAD (ND_PULSE_BASE_HEADER_BYTES + ND_PULSE_CRC_BYTES)
#define ND_PULSE_JOURNAL_HEADER_BYTES 16u
#define ND_PULSE_MARKER_BYTES 16u
#define ND_PULSE_MARKER_SLOTS 32u
#define ND_PULSE_METADATA_BYTES 544u /* two headers plus the marker slots */
#define ND_PULSE_BLOB_MAX 65535u
#define ND_PULSE_READ_CHUNK 32u

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t bytes;
    uint32_t generation;
} nd_pulse_base_header_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t bytes;
    uint32_t generation;
    uint32_t crc;
} nd_pulse_journal_header_t;

typedef struct {
    uint32_t cursor[ND_PULSE_DIRECTIONS];
    uint8_t tariff;
    uint8_t reserved[3];
    uint32_t crc;
} nd_pulse_marker_t;

_Static_assert(sizeof(nd_pulse_base_header_t) == ND_PULSE_BASE_HEADER_BYTES,
               "base header layout");
_Static_assert(sizeof(nd_pulse_journal_header_t) == ND_PULSE_JOURNAL_HEADER_BYTES,
               "journal header layout");
_Static_assert(sizeof(nd_pulse_marker_t) == ND_PULSE_MARKER_BYTES,
               "marker layout");

enum {
    ND_JOURNAL_NO_BASE = 0,
    ND_JOURNAL_INVALID = 1,
    ND_JOURNAL_ACTIVE = 2,
    ND_JOURNAL_FAULT = 3
};

typedef struct nd_pulse_journal {
    nd_pulse_store_t port;
    const nd_block_ops_t *block;
    const nd_block_ops_t *base_block;
    uint32_t journal_offset;
    uint32_t journal_bytes;
    uint32_t base_offset[2];
    uint32_t base_capacity;
    uint32_t lane_bytes;
    uint32_t capacity;
    uint32_t cursor[ND_PULSE_DIRECTIONS];
    uint32_t pending[ND_PULSE_DIRECTIONS];
    uint32_t generation;
    uint8_t shadow[ND_PULSE_DIRECTIONS];
    uint8_t tariff;
    uint8_t marker_count;
    uint8_t state;
    uint8_t granularity;
    uint8_t attached;
    uint8_t has_base;
    uint8_t marker_pending;
} nd_pulse_journal_t;

_Static_assert(sizeof(nd_pulse_journal_t) <= ND_PULSE_STORE_STORAGE_SIZE,
               "pulse store storage too small");

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

static int ranges_overlap(uint32_t first, uint32_t first_length,
                          uint32_t second, uint32_t second_length)
{
    return first < second + second_length
        && second < first + first_length;
}

static uint32_t base_crc(const nd_pulse_base_header_t *header,
                         const uint8_t *blob, uint32_t length)
{
    uint32_t crc = nd_crc32_update(ND_CRC32_INIT, (const uint8_t *)header,
                                   ND_PULSE_BASE_HEADER_BYTES);

    crc = nd_crc32_update(crc, blob, length);
    return nd_crc32_finish(crc);
}

static int write_base(nd_pulse_journal_t *journal, uint32_t slot,
                      const uint8_t *blob, uint32_t length,
                      uint32_t generation)
{
    nd_pulse_base_header_t header = {
        .magic = ND_PULSE_BASE_MAGIC,
        .version = ND_PULSE_FORMAT_VERSION,
        .bytes = (uint16_t)length,
        .generation = generation,
    };
    uint8_t tail[ND_PULSE_CRC_BYTES];
    uint32_t offset = journal->base_offset[slot];

    store_u32(tail, base_crc(&header, blob, length));
    if (nd_block_program(journal->base_block, offset,
                         (const uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }
    if (nd_block_program(journal->base_block,
                         offset + ND_PULSE_BASE_HEADER_BYTES, blob,
                         length) != 0) {
        return -1;
    }
    if (nd_block_program(journal->base_block,
                         offset + ND_PULSE_BASE_HEADER_BYTES + length, tail,
                         sizeof(tail)) != 0) {
        return -1;
    }
    return 0;
}

/* 1 valid, 0 invalid, -1 device error. A header left erased is reported
 * through *blank so a torn slot can be told apart from a first boot. */
static int read_base_record(nd_pulse_journal_t *journal, uint32_t slot,
                            uint32_t *generation, uint8_t *blank)
{
    nd_pulse_base_header_t header;
    uint8_t chunk[ND_PULSE_READ_CHUNK];
    uint8_t tail[ND_PULSE_CRC_BYTES];
    uint32_t crc = ND_CRC32_INIT;
    uint32_t remaining;
    uint32_t offset =
        journal->base_offset[slot] + ND_PULSE_BASE_HEADER_BYTES;

    if (nd_block_read(journal->base_block, journal->base_offset[slot],
                      (uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }
    *blank = (uint8_t)all_ff(&header, sizeof(header));
    if (*blank != 0u) {
        return 0;
    }
    if (header.magic != ND_PULSE_BASE_MAGIC
        || header.version != ND_PULSE_FORMAT_VERSION || header.bytes == 0u
        || (uint32_t)header.bytes
               > journal->base_capacity - ND_PULSE_BASE_OVERHEAD) {
        return 0;
    }
    crc = nd_crc32_update(crc, (const uint8_t *)&header, sizeof(header));
    remaining = header.bytes;
    while (remaining > 0u) {
        uint32_t take = remaining < ND_PULSE_READ_CHUNK ? remaining
                                                        : ND_PULSE_READ_CHUNK;

        if (nd_block_read(journal->base_block, offset, chunk, take) != 0) {
            return -1;
        }
        crc = nd_crc32_update(crc, chunk, take);
        offset += take;
        remaining -= take;
    }
    if (nd_block_read(journal->base_block, offset, tail, sizeof(tail)) != 0) {
        return -1;
    }
    if (nd_crc32_finish(crc) != load_u32(tail)) {
        return 0;
    }
    *generation = header.generation;
    return 1;
}

/* 1 found, 0 no base, -1 device error, -2 non-blank but invalid. A torn slot
 * must never read as EMPTY: the consumer would adopt a stale legacy record. */
static int select_base(nd_pulse_journal_t *journal, uint32_t *generation,
                       uint32_t *slot)
{
    uint32_t best = 0;
    uint32_t best_slot = 0;
    int found = 0;
    int nonblank = 0;

    for (uint32_t index = 0; index < 2u; index++) {
        uint32_t candidate = 0;
        uint8_t blank = 0;
        int valid = read_base_record(journal, index, &candidate, &blank);

        if (valid < 0) {
            return -1;
        }
        if (blank == 0u) {
            nonblank = 1;
        }
        if (valid > 0 && (found == 0 || candidate > best)) {
            best = candidate;
            best_slot = index;
            found = 1;
        }
    }
    *generation = best;
    if (slot != NULL) {
        *slot = best_slot;
    }
    if (found == 0 && nonblank != 0) {
        return -2;
    }
    return found;
}

static uint32_t record_crc(const void *record, size_t bytes)
{
    return nd_crc32_finish(nd_crc32_update(ND_CRC32_INIT,
                                           (const uint8_t *)record, bytes));
}

static int write_journal_header(nd_pulse_journal_t *journal, uint32_t lane)
{
    nd_pulse_journal_header_t header = {
        .magic = ND_PULSE_JOURNAL_MAGIC,
        .version = ND_PULSE_FORMAT_VERSION,
        .bytes = (uint16_t)sizeof(nd_pulse_journal_header_t),
        .generation = journal->generation,
    };
    uint8_t check[ND_PULSE_JOURNAL_HEADER_BYTES];
    uint32_t offset = journal->journal_offset + lane * ND_PULSE_JOURNAL_HEADER_BYTES;

    header.crc = record_crc(&header, ND_PULSE_JOURNAL_HEADER_BYTES
                                       - ND_PULSE_CRC_BYTES);
    if (nd_block_program(journal->block, offset, (const uint8_t *)&header,
                         sizeof(header)) != 0) {
        return -1;
    }
    if (nd_block_read(journal->block, offset, check, sizeof(check)) != 0) {
        return -1;
    }
    return memcmp(check, &header, sizeof(header)) == 0 ? 0 : -1;
}

/* 1 valid, 0 invalid, -1 device error. */
static int journal_header_valid(nd_pulse_journal_t *journal, uint32_t lane)
{
    nd_pulse_journal_header_t header;
    uint32_t offset = journal->journal_offset + lane * ND_PULSE_JOURNAL_HEADER_BYTES;

    if (nd_block_read(journal->block, offset, (uint8_t *)&header,
                      sizeof(header)) != 0) {
        return -1;
    }
    if (header.magic != ND_PULSE_JOURNAL_MAGIC
        || header.version != ND_PULSE_FORMAT_VERSION
        || header.bytes != (uint16_t)sizeof(header)
        || header.generation != journal->generation) {
        return 0;
    }
    return header.crc == record_crc(&header, ND_PULSE_JOURNAL_HEADER_BYTES
                                              - ND_PULSE_CRC_BYTES) ? 1 : 0;
}

static int marker_read(nd_pulse_journal_t *journal, uint32_t slot,
                       nd_pulse_marker_t *marker)
{
    uint32_t offset = journal->journal_offset
                    + 2u * ND_PULSE_JOURNAL_HEADER_BYTES
                    + slot * ND_PULSE_MARKER_BYTES;

    return nd_block_read(journal->block, offset, (uint8_t *)marker,
                         sizeof(*marker));
}

static int marker_valid(const nd_pulse_marker_t *marker)
{
    return marker->crc == record_crc(marker, ND_PULSE_MARKER_BYTES
                                             - ND_PULSE_CRC_BYTES);
}

static int write_marker(nd_pulse_journal_t *journal, uint8_t tariff)
{
    nd_pulse_marker_t marker = {
        .cursor = {journal->cursor[0], journal->cursor[1]},
        .tariff = tariff,
    };
    nd_pulse_marker_t check;
    uint32_t offset = journal->journal_offset
                    + 2u * ND_PULSE_JOURNAL_HEADER_BYTES
                    + (uint32_t)journal->marker_count * ND_PULSE_MARKER_BYTES;

    marker.crc = record_crc(&marker, ND_PULSE_MARKER_BYTES
                                     - ND_PULSE_CRC_BYTES);
    if (nd_block_program(journal->block, offset, (const uint8_t *)&marker,
                         sizeof(marker)) != 0) {
        return -1;
    }
    if (marker_read(journal, journal->marker_count, &check) != 0) {
        return -1;
    }
    return memcmp(&check, &marker, sizeof(marker)) == 0 ? 0 : -1;
}

static uint32_t cleared_prefix(uint8_t value)
{
    uint32_t count = 0;

    while ((value & 1u) == 0u) {
        count++;
        value = (uint8_t)(value >> 1);
    }
    return count;
}

/* Rebuild one direction's durable cursor and the byte under it. */
static int scan_bits(nd_pulse_journal_t *journal, uint32_t direction)
{
    uint32_t offset = journal->journal_offset + ND_PULSE_METADATA_BYTES
                    + direction * journal->lane_bytes;
    uint32_t count = 0;

    for (uint32_t index = 0; index < journal->lane_bytes; index++) {
        uint8_t value;

        if (nd_block_read(journal->block, offset + index, &value, 1u) != 0) {
            return -1;
        }
        if (value == 0xFFu) {
            journal->shadow[direction] = 0xFFu;
            journal->cursor[direction] = count;
            return 0;
        }
        if (value != 0x00u) {
            count += cleared_prefix(value);
            journal->shadow[direction] = value;
            journal->cursor[direction] = count;
            return 0;
        }
        count += 8u;
    }
    journal->shadow[direction] = 0xFFu;
    journal->cursor[direction] = count;
    return 0;
}

/* Restore marker_count and the current tariff. Without both, the first
 * post-reboot tariff change would rewrite slot 0. */
static int scan_markers(nd_pulse_journal_t *journal)
{
    uint32_t count = 0;
    uint8_t tariff = 0;

    for (uint32_t slot = 0; slot < ND_PULSE_MARKER_SLOTS; slot++) {
        nd_pulse_marker_t marker;

        if (marker_read(journal, slot, &marker) != 0) {
            return -1;
        }
        if (all_ff(&marker, sizeof(marker))) {
            break;
        }
        count++;
        if (marker_valid(&marker) && marker.tariff >= 1u
            && marker.tariff <= ND_PULSE_TARIFFS) {
            tariff = marker.tariff;
        }
    }
    journal->marker_count = (uint8_t)count;
    journal->tariff = tariff;
    return 0;
}

static int ensure_journal(nd_pulse_journal_t *journal)
{
    if (nd_block_erase(journal->block, journal->journal_offset,
                       journal->journal_bytes) != 0) {
        journal->state = ND_JOURNAL_INVALID;
        return -1;
    }
    if (write_journal_header(journal, 0) != 0
        || write_journal_header(journal, 1) != 0) {
        journal->state = ND_JOURNAL_INVALID;
        return -1;
    }
    journal->cursor[0] = 0;
    journal->cursor[1] = 0;
    journal->pending[0] = 0;
    journal->pending[1] = 0;
    journal->shadow[0] = 0xFFu;
    journal->shadow[1] = 0xFFu;
    journal->marker_count = 0;
    journal->tariff = 0;
    journal->marker_pending = 0;
    journal->state = ND_JOURNAL_ACTIVE;
    return 0;
}

/* 0 attached (or no base), -1 device error or a torn base region. */
static int attach(nd_pulse_journal_t *journal)
{
    int found;
    int first;
    int second;

    if (journal->attached != 0u) {
        return journal->state == ND_JOURNAL_FAULT ? -1 : 0;
    }
    journal->attached = 1;
    journal->state = ND_JOURNAL_INVALID;
    found = select_base(journal, &journal->generation, NULL);
    if (found < 0) {
        journal->state = ND_JOURNAL_FAULT;
        return -1;
    }
    if (found == 0) {
        journal->state = ND_JOURNAL_NO_BASE;
        return 0;
    }
    journal->has_base = 1;
    first = journal_header_valid(journal, 0);
    second = journal_header_valid(journal, 1);
    if (first < 0 || second < 0) {
        journal->state = ND_JOURNAL_FAULT;
        return -1;
    }
    if (first != 1 || second != 1) {
        return 0;
    }
    if (scan_bits(journal, 0) != 0 || scan_bits(journal, 1) != 0
        || scan_markers(journal) != 0) {
        journal->state = ND_JOURNAL_FAULT;
        return -1;
    }
    journal->state = ND_JOURNAL_ACTIVE;
    return 0;
}

static int flush_lane(nd_pulse_journal_t *journal, uint32_t direction)
{
    uint8_t value;
    uint8_t check;
    uint32_t offset;

    if (journal->pending[direction] == 0u) {
        return 0;
    }
    value = journal->shadow[direction];
    offset = journal->journal_offset + ND_PULSE_METADATA_BYTES
           + direction * journal->lane_bytes
           + journal->cursor[direction] / 8u;
    if (nd_block_program(journal->block, offset, &value, 1u) != 0) {
        return -1;
    }
    if (nd_block_read(journal->block, offset, &check, 1u) != 0) {
        return -1;
    }
    if (check != value) {
        return -1;
    }
    journal->cursor[direction] += journal->pending[direction];
    journal->pending[direction] = 0;
    journal->shadow[direction] =
        (journal->cursor[direction] & 7u) != 0u ? value : 0xFFu;
    return 0;
}

static nd_pulse_status_t journal_append(void *ctx, uint8_t lane)
{
    nd_pulse_journal_t *journal = ctx;
    uint32_t direction;
    uint8_t tariff;

    if (journal == NULL || lane >= ND_PULSE_LANES) {
        return ND_PULSE_ERROR;
    }
    if (attach(journal) != 0) {
        return ND_PULSE_ERROR;
    }
    if (journal->state == ND_JOURNAL_NO_BASE
        || journal->state == ND_JOURNAL_FAULT) {
        return ND_PULSE_ERROR;
    }
    if (journal->state == ND_JOURNAL_INVALID && ensure_journal(journal) != 0) {
        return ND_PULSE_ERROR;
    }
    direction = lane % ND_PULSE_DIRECTIONS;
    tariff = nd_energy_pulse_lane_tariff(lane);
    /* A failed marker write leaves its slot unconsumed. Until a marker is
     * verified there, an append must resolve it: the failed program may have
     * landed, and a bit written after it would replay under the wrong
     * tariff. */
    if (tariff != journal->tariff || journal->marker_pending != 0u) {
        if (journal->marker_count >= ND_PULSE_MARKER_SLOTS) {
            return ND_PULSE_FULL;
        }
        if (flush_lane(journal, 0) != 0 || flush_lane(journal, 1) != 0) {
            return ND_PULSE_ERROR;
        }
        if (write_marker(journal, tariff) != 0) {
            journal->marker_pending = 1;
            return ND_PULSE_ERROR;
        }
        journal->marker_pending = 0;
        journal->marker_count++;
        journal->tariff = tariff;
    }
    if (journal->cursor[direction] >= journal->capacity) {
        return ND_PULSE_FULL;
    }
    {
        uint32_t bit = journal->cursor[direction] + journal->pending[direction];
        uint8_t mask = (uint8_t)(1u << (bit & 7u));

        journal->shadow[direction] =
            (uint8_t)(journal->shadow[direction] & (uint8_t)~mask);
        journal->pending[direction]++;
        if (journal->pending[direction] >= journal->granularity
            || (bit & 7u) == 7u) {
            if (flush_lane(journal, direction) != 0) {
                return ND_PULSE_ERROR;
            }
        }
    }
    return ND_PULSE_OK;
}

static nd_pulse_status_t journal_replay(void *ctx, nd_pulse_replay_fn sink,
                                        void *sink_ctx)
{
    nd_pulse_journal_t *journal = ctx;

    if (journal == NULL || sink == NULL) {
        return ND_PULSE_ERROR;
    }
    if (attach(journal) != 0) {
        return ND_PULSE_ERROR;
    }
    if (journal->state == ND_JOURNAL_NO_BASE) {
        return ND_PULSE_EMPTY;
    }
    if (journal->state != ND_JOURNAL_ACTIVE) {
        return ND_PULSE_OK;
    }
    for (uint32_t direction = 0; direction < ND_PULSE_DIRECTIONS; direction++) {
        uint32_t position = 0;
        uint8_t tariff = 1;

        for (uint32_t slot = 0; slot < ND_PULSE_MARKER_SLOTS; slot++) {
            nd_pulse_marker_t marker;
            uint32_t next;

            if (marker_read(journal, slot, &marker) != 0) {
                return ND_PULSE_ERROR;
            }
            if (all_ff(&marker, sizeof(marker))) {
                break;
            }
            if (!marker_valid(&marker) || marker.tariff < 1u
                || marker.tariff > ND_PULSE_TARIFFS) {
                continue;
            }
            next = marker.cursor[direction];
            if (next < position || next > journal->cursor[direction]) {
                continue;
            }
            if (next > position) {
                sink(sink_ctx,
                     nd_energy_pulse_lane((nd_energy_direction_t)direction,
                                          tariff),
                     next - position);
            }
            position = next;
            tariff = marker.tariff;
        }
        if (journal->cursor[direction] > position) {
            sink(sink_ctx,
                 nd_energy_pulse_lane((nd_energy_direction_t)direction, tariff),
                 journal->cursor[direction] - position);
        }
    }
    return ND_PULSE_OK;
}

static nd_pulse_status_t journal_base(void *ctx, uint8_t *buffer,
                                      size_t capacity, size_t *length)
{
    nd_pulse_journal_t *journal = ctx;
    nd_pulse_base_header_t header;
    uint8_t tail[ND_PULSE_CRC_BYTES];
    uint32_t crc;
    uint32_t generation = 0;
    uint32_t slot = 0;
    uint32_t offset;
    int found;

    if (journal == NULL || buffer == NULL || length == NULL) {
        return ND_PULSE_ERROR;
    }
    *length = 0;
    found = select_base(journal, &generation, &slot);
    if (found == -1) {
        /* A device read error leaves the base region unknown. */
        return ND_PULSE_IO_ERROR;
    }
    if (found == -2) {
        /* Non-blank with no valid slot: a known torn region. */
        return ND_PULSE_ERROR;
    }
    if (found == 0) {
        return ND_PULSE_EMPTY;
    }
    /* select_base read this slot as valid. Any failure from here on is a read
     * inconsistency, not a torn region: report IO_ERROR so the consumer
     * retries instead of adopting a legacy record over a real base. */
    if (nd_block_read(journal->base_block, journal->base_offset[slot],
                      (uint8_t *)&header, sizeof(header)) != 0) {
        return ND_PULSE_IO_ERROR;
    }
    if (header.magic != ND_PULSE_BASE_MAGIC
        || header.version != ND_PULSE_FORMAT_VERSION || header.bytes == 0u
        || (uint32_t)header.bytes
               > journal->base_capacity - ND_PULSE_BASE_OVERHEAD) {
        return ND_PULSE_IO_ERROR;
    }
    if ((uint32_t)header.bytes > capacity) {
        *length = header.bytes;
        return ND_PULSE_FULL;
    }
    offset = journal->base_offset[slot] + ND_PULSE_BASE_HEADER_BYTES;
    if (nd_block_read(journal->base_block, offset, buffer, header.bytes) != 0) {
        return ND_PULSE_IO_ERROR;
    }
    if (nd_block_read(journal->base_block, offset + (uint32_t)header.bytes,
                      tail, sizeof(tail)) != 0) {
        return ND_PULSE_IO_ERROR;
    }
    crc = nd_crc32_update(ND_CRC32_INIT, (const uint8_t *)&header,
                          sizeof(header));
    crc = nd_crc32_update(crc, buffer, header.bytes);
    if (nd_crc32_finish(crc) != load_u32(tail)) {
        return ND_PULSE_IO_ERROR;
    }
    *length = header.bytes;
    return ND_PULSE_OK;
}

static nd_pulse_status_t journal_commit(void *ctx, const uint8_t *blob,
                                        size_t length)
{
    nd_pulse_journal_t *journal = ctx;
    uint32_t next;
    uint32_t slot;
    uint32_t check_generation = 0;
    uint8_t blank = 0;
    int valid;

    if (journal == NULL || blob == NULL || length == 0u
        || length > (size_t)(journal->base_capacity - ND_PULSE_BASE_OVERHEAD)
        || length > ND_PULSE_BLOB_MAX) {
        return ND_PULSE_ERROR;
    }
    if (attach(journal) != 0 && journal->has_base == 0u) {
        return ND_PULSE_ERROR;
    }
    next = journal->generation + 1u;
    slot = next & 1u;
    if (write_base(journal, slot, blob, (uint32_t)length, next) != 0) {
        return ND_PULSE_ERROR;
    }
    valid = read_base_record(journal, slot, &check_generation, &blank);
    if (valid > 0 && check_generation != next) {
        return ND_PULSE_ERROR;
    }
    /* write_base reported success, so a verify read that fails or rejects the
     * CRC cannot roll the committed epoch back: the disk slot may hold
     * generation next. Advance the in-RAM generation now so no append is
     * counted against the old base and the next one rebuilds the journal at
     * next. */
    journal->generation = next;
    journal->has_base = 1;
    journal->cursor[0] = 0;
    journal->cursor[1] = 0;
    journal->pending[0] = 0;
    journal->pending[1] = 0;
    journal->shadow[0] = 0xFFu;
    journal->shadow[1] = 0xFFu;
    journal->marker_count = 0;
    journal->tariff = 0;
    journal->marker_pending = 0;
    if (valid <= 0) {
        journal->state = ND_JOURNAL_INVALID;
        return ND_PULSE_ERROR;
    }
    if (nd_block_erase(journal->block, journal->journal_offset,
                       journal->journal_bytes) != 0) {
        journal->state = ND_JOURNAL_INVALID;
        return ND_PULSE_OK;
    }
    if (write_journal_header(journal, 0) != 0
        || write_journal_header(journal, 1) != 0) {
        journal->state = ND_JOURNAL_INVALID;
        return ND_PULSE_OK;
    }
    journal->state = ND_JOURNAL_ACTIVE;
    return ND_PULSE_OK;
}

static const nd_pulse_store_ops_t journal_ops = {
    .append = journal_append,
    .replay = journal_replay,
    .base = journal_base,
    .commit = journal_commit,
};

nd_pulse_status_t nd_pulse_store_append(nd_pulse_store_t *store, uint8_t lane)
{
    if (store == NULL || store->ops == NULL) {
        return ND_PULSE_ERROR;
    }
    return store->ops->append(store->ctx, lane);
}

nd_pulse_status_t nd_pulse_store_replay(nd_pulse_store_t *store,
                                        nd_pulse_replay_fn sink, void *ctx)
{
    if (store == NULL || store->ops == NULL) {
        return ND_PULSE_ERROR;
    }
    return store->ops->replay(store->ctx, sink, ctx);
}

nd_pulse_status_t nd_pulse_store_base(nd_pulse_store_t *store,
                                      uint8_t *buffer, size_t capacity,
                                      size_t *length)
{
    if (store == NULL || store->ops == NULL) {
        return ND_PULSE_ERROR;
    }
    return store->ops->base(store->ctx, buffer, capacity, length);
}

nd_pulse_status_t nd_pulse_store_commit(nd_pulse_store_t *store,
                                        const uint8_t *blob, size_t length)
{
    if (store == NULL || store->ops == NULL) {
        return ND_PULSE_ERROR;
    }
    return store->ops->commit(store->ctx, blob, length);
}

int nd_pulse_store_init(nd_pulse_store_storage_t *storage,
                        const nd_pulse_store_config_t *config)
{
    nd_pulse_journal_t *journal = (nd_pulse_journal_t *)storage;
    const nd_block_ops_t *base_block;
    uint32_t lane_bytes;

    if (storage == NULL || config == NULL || config->block == NULL
        || config->block->read == NULL || config->block->program == NULL
        || config->block->erase == NULL || config->block->size == 0u
        || config->block->erase_unit == 0u) {
        return -1;
    }
    base_block = config->base_block != NULL ? config->base_block
                                            : config->block;
    if (base_block->read == NULL || base_block->program == NULL
        || base_block->size == 0u) {
        return -1;
    }
    if (config->granularity < 1u || config->granularity > 8u) {
        return -1;
    }
    if (config->journal_offset > config->block->size
        || config->journal_bytes
               > config->block->size - config->journal_offset
        || config->journal_offset % config->block->erase_unit != 0u
        || config->journal_bytes % config->block->erase_unit != 0u) {
        return -1;
    }
    if (config->journal_bytes < ND_PULSE_METADATA_BYTES + 2u
        || (config->journal_bytes - ND_PULSE_METADATA_BYTES) % 2u != 0u) {
        return -1;
    }
    lane_bytes = (config->journal_bytes - ND_PULSE_METADATA_BYTES) / 2u;
    if (lane_bytes > 0xFFFFFFFFu / 8u
        || config->base_capacity < ND_PULSE_BASE_OVERHEAD + 1u) {
        return -1;
    }
    for (uint32_t index = 0; index < 2u; index++) {
        if (config->base_offset[index] > base_block->size
            || config->base_capacity
                   > base_block->size - config->base_offset[index]
            || (base_block == config->block
                && ranges_overlap(config->base_offset[index],
                                  config->base_capacity,
                                  config->journal_offset,
                                  config->journal_bytes))) {
            return -1;
        }
    }
    if (ranges_overlap(config->base_offset[0], config->base_capacity,
                       config->base_offset[1], config->base_capacity)) {
        return -1;
    }

    memset(journal, 0, sizeof(*journal));
    journal->port.ops = &journal_ops;
    journal->port.ctx = journal;
    journal->block = config->block;
    journal->base_block = base_block;
    journal->journal_offset = config->journal_offset;
    journal->journal_bytes = config->journal_bytes;
    journal->base_offset[0] = config->base_offset[0];
    journal->base_offset[1] = config->base_offset[1];
    journal->base_capacity = config->base_capacity;
    journal->lane_bytes = lane_bytes;
    journal->capacity = lane_bytes * 8u;
    journal->granularity = config->granularity;
    journal->shadow[0] = 0xFFu;
    journal->shadow[1] = 0xFFu;
    journal->state = ND_JOURNAL_INVALID;
    return 0;
}

nd_pulse_store_t *nd_pulse_store_port(nd_pulse_store_storage_t *storage)
{
    nd_pulse_journal_t *journal = (nd_pulse_journal_t *)storage;

    return storage != NULL ? &journal->port : NULL;
}
