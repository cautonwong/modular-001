#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#include <string.h>

#define PROFILE_RECORD_MAGIC 0x52454331u
#define PROFILE_MARK_MAGIC 0x544D4231u
#define PROFILE_VERSION 4u
#define PROFILE_RECORD_HEADER_SIZE 20u
/* Whole-slot read buffer. Bounds init so a slot always fits on the stack. */
#define PROFILE_SLOT_SIZE_MAX 256u

/* Pre-v4 layout: two 32-byte metadata copies at the base, records at
 * base + 64. Kept only so init fails closed instead of starting empty. */
#define LEGACY_META_MAGIC 0x49445031u
#define LEGACY_META_SIZE 32u
#define LEGACY_META_AREA (2u * LEGACY_META_SIZE)
#define LEGACY_VERSION_MIN 2u
#define LEGACY_VERSION_MAX 3u

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static uint16_t get_u16(const uint8_t *in)
{
    return (uint16_t)(((uint16_t)in[0] << 8) | in[1]);
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static uint32_t get_u32(const uint8_t *in)
{
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16)
        | ((uint32_t)in[2] << 8) | in[3];
}

static uint16_t crc16_update(uint16_t crc, const uint8_t *data,
                             size_t length)
{
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000u)
                ? (uint16_t)((((uint32_t)crc << 1) ^ 0x1021u) & 0xFFFFu)
                : (uint16_t)(((uint32_t)crc << 1) & 0xFFFFu);
        }
    }
    return crc;
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
    return crc16_update(0xFFFFu, data, length);
}

/* One spare sector after the ring keeps the erase target outside the live
 * window: entries <= capacity <= phys - sector_slots. */
static uint16_t phys_slots(const nd_profile_store_t *profile)
{
    uint32_t sector_slots = profile->sector_slots;
    if (sector_slots == 0u) {
        return profile->capacity;
    }
    return (uint16_t)(((uint32_t)profile->capacity + sector_slots - 1u)
                          / sector_slots * sector_slots
                      + sector_slots);
}

static uint32_t slot_offset(const nd_profile_store_t *profile, uint16_t slot)
{
    return profile->base_offset + (uint32_t)slot * profile->slot_size;
}

enum slot_kind {
    SLOT_RECORD = 1,
    SLOT_MARK,
};

/* A device error must not look like an empty or invalid slot: recovery would
 * move head over live records, or report an empty store and erase it. */
enum slot_status {
    SLOT_OK = 0,
    SLOT_INVALID = 1,
    SLOT_DEVICE_ERROR = -1,
};

struct slot_view {
    enum slot_kind kind;
    uint16_t sequence;
    uint16_t length;
};

static void feed(const nd_profile_store_t *profile)
{
    if (profile->watchdog.feed != NULL) {
        profile->watchdog.feed(profile->watchdog.ctx);
    }
}

/* One store read of the whole slot; CRC covers header plus payload once.
 * payload == NULL validates without copying (recovery only needs the
 * sequence and kind). */
static int read_slot(const nd_profile_store_t *profile, uint16_t slot,
                     struct slot_view *view, uint8_t *payload,
                     size_t capacity)
{
    if (profile->slot_size > PROFILE_SLOT_SIZE_MAX) {
        return SLOT_INVALID;
    }
    uint8_t bytes[PROFILE_SLOT_SIZE_MAX];
    int status = nd_store_read(profile->store, slot_offset(profile, slot),
                               bytes, profile->slot_size);
    feed(profile);
    if (status != 0 || bytes[4] != PROFILE_VERSION) {
        return status != 0 ? SLOT_DEVICE_ERROR : SLOT_INVALID;
    }
    uint16_t length = get_u16(&bytes[13]);
    uint32_t magic = get_u32(&bytes[0]);
    if (magic == PROFILE_RECORD_MAGIC && length > 0u
        && length <= profile->slot_size - PROFILE_RECORD_HEADER_SIZE) {
        if (get_u16(&bytes[18])
            != crc16_update(crc16(bytes, 18),
                            &bytes[PROFILE_RECORD_HEADER_SIZE], length)) {
            return SLOT_INVALID;
        }
        if (payload != NULL) {
            if (length > capacity) {
                return SLOT_INVALID;
            }
            memcpy(payload, &bytes[PROFILE_RECORD_HEADER_SIZE], length);
        }
        view->sequence = get_u16(&bytes[15]);
        view->kind = SLOT_RECORD;
        view->length = length;
        return SLOT_OK;
    }
    if (magic == PROFILE_MARK_MAGIC && length == 0u) {
        if (get_u16(&bytes[18]) != crc16(bytes, 18)) {
            return SLOT_INVALID;
        }
        view->sequence = get_u16(&bytes[15]);
        view->kind = SLOT_MARK;
        view->length = 0;
        return SLOT_OK;
    }
    return SLOT_INVALID;
}

/* 0 invalid, 1 valid, -1 device error. */
static int legacy_record_valid(const nd_profile_store_t *profile,
                               uint32_t offset)
{
    uint8_t header[PROFILE_RECORD_HEADER_SIZE];
    uint8_t record[256];
    int status = nd_store_read(profile->store, offset, header, sizeof(header));
    feed(profile);
    if (status != 0) {
        return SLOT_DEVICE_ERROR;
    }
    if (get_u32(&header[0]) != PROFILE_RECORD_MAGIC
        || header[4] < LEGACY_VERSION_MIN || header[4] > LEGACY_VERSION_MAX) {
        return SLOT_INVALID;
    }
    uint16_t length = get_u16(&header[13]);
    if (length == 0u
        || length > profile->slot_size - PROFILE_RECORD_HEADER_SIZE
        || length > sizeof(record)) {
        return SLOT_INVALID;
    }
    if (nd_store_read(profile->store, offset + sizeof(header), record,
                      length) != 0) {
        feed(profile);
        return SLOT_DEVICE_ERROR;
    }
    feed(profile);
    return get_u16(&header[18])
               == crc16_update(crc16(header, 18), record, length)
           ? SLOT_OK
           : SLOT_INVALID;
}

/* 0 absent, 1 present, -1 device error. */
static int legacy_image_present(const nd_profile_store_t *profile,
                                uint32_t legacy_base_offset)
{
    uint8_t meta[LEGACY_META_SIZE];
    if (nd_store_read(profile->store, legacy_base_offset, meta,
                      sizeof(meta)) != 0) {
        feed(profile);
        return SLOT_DEVICE_ERROR;
    }
    feed(profile);
    if (get_u32(&meta[0]) == LEGACY_META_MAGIC
        && meta[4] >= LEGACY_VERSION_MIN && meta[4] <= LEGACY_VERSION_MAX
        && get_u16(&meta[30]) == crc16(meta, 30)) {
        return 1;
    }
    for (uint16_t slot = 0; slot < profile->capacity; slot++) {
        int status = legacy_record_valid(
            profile, legacy_base_offset + LEGACY_META_AREA
                         + (uint32_t)slot * profile->slot_size);
        if (status == SLOT_DEVICE_ERROR) {
            return SLOT_DEVICE_ERROR;
        }
        if (status == SLOT_OK) {
            return 1;
        }
    }
    return 0;
}

static int recover_records(nd_profile_store_t *profile)
{
    uint16_t phys = phys_slots(profile);
    uint16_t best_slot = 0;
    uint16_t best_sequence = 0;
    enum slot_kind best_kind = SLOT_RECORD;
    int found = 0;
    for (uint16_t slot = 0; slot < phys; slot++) {
        struct slot_view view;
        int status = read_slot(profile, slot, &view, NULL, 0);
        if (status == SLOT_DEVICE_ERROR) {
            return -1;
        }
        if (status != SLOT_OK) {
            continue;
        }
        if (!found || (int16_t)(view.sequence - best_sequence) > 0) {
            best_slot = slot;
            best_sequence = view.sequence;
            best_kind = view.kind;
        }
        found = 1;
    }
    if (!found) {
        profile->head = 0;
        profile->entries = 0;
        profile->next_sequence = 1;
        return 0;
    }

    /* Sequence adjacency drops valid stale records left behind by a torn
     * erase, something a contiguous-run walk cannot tell apart. */
    profile->head = (uint16_t)((best_slot + 1u) % phys);
    profile->next_sequence = (uint16_t)(best_sequence + 1u);
    if (best_kind == SLOT_MARK) {
        profile->entries = 0;
        return 1;
    }
    profile->entries = 1;
    uint16_t want = (uint16_t)(best_sequence - 1u);
    uint16_t slot = best_slot;
    for (uint16_t count = 1; count < profile->capacity; count++) {
        slot = (uint16_t)((slot + phys - 1u) % phys);
        struct slot_view view;
        int status = read_slot(profile, slot, &view, NULL, 0);
        if (status == SLOT_DEVICE_ERROR) {
            return -1;
        }
        if (status != SLOT_OK || view.kind != SLOT_RECORD
            || view.sequence != want) {
            break;
        }
        profile->entries++;
        want = (uint16_t)(want - 1u);
    }
    return 1;
}

int nd_profile_store_init(nd_profile_store_t *profile, nd_store_t *store,
                          uint32_t base_offset, uint16_t capacity,
                          uint16_t slot_size, uint32_t sector_size,
                          const nd_profile_store_opts_t *opts)
{
    if (profile == NULL || store == NULL || capacity == 0
        || slot_size <= PROFILE_RECORD_HEADER_SIZE
        || slot_size > PROFILE_SLOT_SIZE_MAX) {
        return -1;
    }
    uint32_t sector_slots = 0;
    if (sector_size > 0u) {
        if (sector_size % slot_size != 0
            || sector_size < 2u * (uint32_t)slot_size
            || base_offset % sector_size != 0
            || sector_size / slot_size > 0xFFFFu) {
            return -1;
        }
        sector_slots = sector_size / slot_size;
        if (store->erase == NULL) {
            sector_slots = 0;
        }
    }
    uint32_t phys = capacity;
    if (sector_slots > 0u) {
        phys = ((uint32_t)capacity + sector_slots - 1u) / sector_slots
                   * sector_slots
               + sector_slots;
    }
    if (phys > 32767u) {
        return -1;
    }
    *profile = (nd_profile_store_t){
        .store = store,
        .base_offset = base_offset,
        .capacity = capacity,
        .slot_size = slot_size,
        .sector_slots = (uint16_t)sector_slots,
        .head = 0,
        .entries = 0,
        .next_sequence = 1,
        .watchdog = opts != NULL && opts->watchdog != NULL
                        ? *opts->watchdog
                        : (nd_profile_watchdog_t){0},
        .legacy = 0,
    };
    int recovered = recover_records(profile);
    if (recovered < 0) {
        return -1;
    }
    if (recovered == 0 && opts != NULL
        && opts->legacy_base_offset != ND_PROFILE_STORE_LEGACY_NONE) {
        int legacy = legacy_image_present(profile, opts->legacy_base_offset);
        if (legacy < 0) {
            return -1;
        }
        if (legacy > 0) {
            profile->legacy = 1;
            return ND_PROFILE_STORE_LEGACY_IMAGE;
        }
    }
    return 0;
}

int nd_profile_store_append(nd_profile_store_t *profile,
                            const nd_datetime_t *time, uint8_t status,
                            const uint8_t *record, size_t length)
{
    uint8_t packed[256];
    if (profile == NULL || time == NULL || record == NULL || profile->legacy
        || length == 0
        || length > profile->slot_size - PROFILE_RECORD_HEADER_SIZE
        || length + PROFILE_RECORD_HEADER_SIZE > sizeof(packed)) {
        return -1;
    }

    uint8_t *header = packed;
    memset(header, 0, PROFILE_RECORD_HEADER_SIZE);
    put_u32(&header[0], PROFILE_RECORD_MAGIC);
    header[4] = PROFILE_VERSION;
    header[5] = status;
    put_u16(&header[6], time->year);
    header[8] = time->month;
    header[9] = time->day;
    header[10] = time->hour;
    header[11] = time->minute;
    header[12] = time->second;
    put_u16(&header[13], (uint16_t)length);
    put_u16(&header[15], profile->next_sequence);
    memcpy(&packed[PROFILE_RECORD_HEADER_SIZE], record, length);
    put_u16(&header[18], crc16_update(crc16(header, 18), record, length));

    uint16_t phys = phys_slots(profile);
    if (profile->sector_slots > 0
        && profile->head % profile->sector_slots == 0) {
        if (nd_store_erase(profile->store, slot_offset(profile, profile->head),
                           (size_t)profile->sector_slots
                               * profile->slot_size) != 0) {
            return -1;
        }
    }
    if (nd_store_write(profile->store, slot_offset(profile, profile->head),
                       packed, PROFILE_RECORD_HEADER_SIZE + length) != 0) {
        return -1;
    }
    profile->head = (uint16_t)((profile->head + 1u) % phys);
    if (profile->entries < profile->capacity) {
        profile->entries++;
    }
    profile->next_sequence++;
    return 0;
}

void nd_profile_store_snapshot(const nd_profile_store_t *profile,
                               nd_profile_store_snapshot_t *snapshot)
{
    if (profile == NULL || snapshot == NULL) {
        return;
    }
    snapshot->head = profile->head;
    snapshot->entries = profile->entries;
    snapshot->next_sequence = profile->next_sequence;
}

int nd_profile_store_read_at(const nd_profile_store_t *profile,
                             const nd_profile_store_snapshot_t *snapshot,
                             uint32_t index, uint8_t *record,
                             size_t capacity, size_t *length)
{
    if (profile == NULL || snapshot == NULL || record == NULL
        || length == NULL || index >= snapshot->entries) {
        return -1;
    }

    uint16_t phys = phys_slots(profile);
    uint16_t oldest = (uint16_t)((snapshot->head + phys - snapshot->entries)
                                 % phys);
    uint16_t slot = (uint16_t)(((uint32_t)oldest + index) % phys);
    struct slot_view view;
    uint16_t expected = (uint16_t)(((uint32_t)snapshot->next_sequence
                                    + 0x10000u - snapshot->entries + index)
                                   & 0xFFFFu);
    if (read_slot(profile, slot, &view, record, capacity) != 0
        || view.kind != SLOT_RECORD
        || view.sequence != expected) {
        return -1;
    }

    *length = view.length;
    return 0;
}

int nd_profile_store_read(const nd_profile_store_t *profile, uint32_t index,
                          uint8_t *record, size_t capacity, size_t *length)
{
    if (profile == NULL) {
        return -1;
    }
    nd_profile_store_snapshot_t snapshot;
    nd_profile_store_snapshot(profile, &snapshot);
    return nd_profile_store_read_at(profile, &snapshot, index, record,
                                    capacity, length);
}

int nd_profile_store_reset(nd_profile_store_t *profile)
{
    uint8_t mark[PROFILE_RECORD_HEADER_SIZE] = {0};
    if (profile == NULL) {
        return -1;
    }
    uint16_t phys = phys_slots(profile);
    if (profile->sector_slots > 0
        && profile->head % profile->sector_slots == 0) {
        if (nd_store_erase(profile->store, slot_offset(profile, profile->head),
                           (size_t)profile->sector_slots
                               * profile->slot_size) != 0) {
            return -1;
        }
    }
    put_u32(&mark[0], PROFILE_MARK_MAGIC);
    mark[4] = PROFILE_VERSION;
    put_u16(&mark[15], profile->next_sequence);
    put_u16(&mark[18], crc16(mark, 18));
    if (nd_store_write(profile->store, slot_offset(profile, profile->head),
                       mark, sizeof(mark)) != 0) {
        return -1;
    }
    profile->head = (uint16_t)((profile->head + 1u) % phys);
    profile->entries = 0;
    profile->next_sequence++;
    profile->legacy = 0;
    return 0;
}

void nd_profile_store_hold(nd_profile_store_t *profile)
{
    if (profile != NULL) {
        profile->legacy = 1;
    }
}

uint16_t nd_profile_store_entries(const nd_profile_store_t *profile)
{
    return profile == NULL ? 0 : profile->entries;
}

uint16_t nd_profile_store_capacity(const nd_profile_store_t *profile)
{
    return profile == NULL ? 0 : profile->capacity;
}
