#include "targets/idis_ip_meter/cosem/nd_profile_store.h"

#include <string.h>

#define PROFILE_META_MAGIC 0x49445031u
#define PROFILE_RECORD_MAGIC 0x52454331u
#define PROFILE_VERSION 3u
#define PROFILE_META_INTERVAL 128u
#define PROFILE_META_SIZE 32u
#define PROFILE_META_COPIES 2u
#define PROFILE_META_AREA (PROFILE_META_SIZE * PROFILE_META_COPIES)
#define PROFILE_RECORD_HEADER_SIZE 20u

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
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
    return crc16_update(0xFFFFu, data, length);
}

static int write_meta_at(const nd_profile_store_t *profile, uint32_t offset)
{
    uint8_t meta[PROFILE_META_SIZE] = {0};
    put_u32(&meta[0], PROFILE_META_MAGIC);
    meta[4] = PROFILE_VERSION;
    put_u16(&meta[5], profile->slot_size);
    put_u16(&meta[7], profile->capacity);
    put_u16(&meta[9], profile->head);
    put_u16(&meta[11], profile->entries);
    put_u16(&meta[13], profile->generation);
    put_u16(&meta[15], profile->next_sequence);
    put_u16(&meta[30], crc16(meta, 30));
    return nd_store_write(profile->store, offset, meta, sizeof(meta));
}

static int write_meta(const nd_profile_store_t *profile)
{
    if (write_meta_at(profile, profile->base_offset) != 0) {
        return -1;
    }
    return write_meta_at(profile, profile->base_offset + PROFILE_META_SIZE);
}

static int read_meta_at(nd_profile_store_t *profile, uint32_t offset)
{
    uint8_t meta[PROFILE_META_SIZE];
    if (nd_store_read(profile->store, offset, meta, sizeof(meta)) != 0) {
        return -1;
    }
    if (get_u32(&meta[0]) != PROFILE_META_MAGIC
        || (meta[4] != 2u && meta[4] != PROFILE_VERSION)
        || get_u16(&meta[5]) != profile->slot_size
        || get_u16(&meta[7]) != profile->capacity
        || get_u16(&meta[30]) != crc16(meta, 30)) {
        return -1;
    }
    uint16_t head = get_u16(&meta[9]);
    uint16_t entries = get_u16(&meta[11]);
    if (head >= profile->capacity || entries > profile->capacity) {
        return -1;
    }
    profile->head = head;
    profile->entries = entries;
    profile->generation = meta[4] == PROFILE_VERSION ? get_u16(&meta[13]) : 0;
    profile->next_sequence = meta[4] == PROFILE_VERSION ? get_u16(&meta[15]) : 0;
    return 0;
}

static uint32_t slot_offset(const nd_profile_store_t *profile, uint16_t index);

static int read_meta(nd_profile_store_t *profile)
{
    if (read_meta_at(profile, profile->base_offset) == 0) {
        return 0;
    }
    if (read_meta_at(profile, profile->base_offset + PROFILE_META_SIZE) != 0) {
        return -1;
    }
    return write_meta_at(profile, profile->base_offset);
}


static int read_slot(const nd_profile_store_t *profile, uint16_t slot,
                     uint16_t *sequence)
{
    uint32_t offset = slot_offset(profile, slot);
    uint8_t header[PROFILE_RECORD_HEADER_SIZE];
    uint8_t record[256];
    if (nd_store_read(profile->store, offset, header, sizeof(header)) != 0) {
        return -1;
    }
    if (get_u32(&header[0]) != PROFILE_RECORD_MAGIC
        || (header[4] != 2u && header[4] != PROFILE_VERSION)) {
        return -1;
    }
    uint16_t length = get_u16(&header[13]);
    if (length == 0 || length > profile->slot_size - PROFILE_RECORD_HEADER_SIZE
        || length > sizeof(record)) {
        return -1;
    }
    if (nd_store_read(profile->store, offset + sizeof(header), record,
                      length) != 0) {
        return -1;
    }
    if (get_u16(&header[18]) != crc16_update(crc16(header, 18), record,
                                             length)) {
        return -1;
    }
    *sequence = header[4] == PROFILE_VERSION ? get_u16(&header[15]) : 0;
    if (profile->generation != 0
        && (int16_t)(*sequence - profile->generation) < 0) {
        return -1;
    }
    return 0;
}

static int recover_records(nd_profile_store_t *profile)
{
    uint16_t best_slot = 0;
    uint16_t best_sequence = 0;
    uint16_t found = 0;
    for (uint16_t slot = 0; slot < profile->capacity; slot++) {
        uint16_t sequence = 0;
        if (read_slot(profile, slot, &sequence) != 0) {
            continue;
        }
        if (found == 0 || (int16_t)(sequence - best_sequence) > 0) {
            best_slot = slot;
            best_sequence = sequence;
        }
        found++;
    }
    if (found == 0) {
        return 0;
    }

    /* Keep the contiguous run that ends at the newest record. A torn sector
     * erase can leave a hole; older records beyond it are not addressable. */
    uint16_t entries = 1;
    uint16_t slot = best_slot;
    while (entries < profile->capacity) {
        slot = (uint16_t)((slot + profile->capacity - 1u)
                          % profile->capacity);
        uint16_t sequence = 0;
        if (read_slot(profile, slot, &sequence) != 0) {
            break;
        }
        entries++;
    }
    profile->head = (uint16_t)((best_slot + 1u) % profile->capacity);
    profile->entries = entries;
    profile->next_sequence = (uint16_t)(best_sequence + 1u);
    return 1;
}

int nd_profile_store_init(nd_profile_store_t *profile, nd_store_t *store,
                          uint32_t base_offset, uint16_t capacity,
                          uint16_t slot_size)
{
    if (profile == NULL || store == NULL || capacity == 0
        || slot_size <= PROFILE_RECORD_HEADER_SIZE) {
        return -1;
    }
    profile->store = store;
    profile->base_offset = base_offset;
    profile->capacity = capacity;
    profile->slot_size = slot_size;
    profile->head = 0;
    profile->entries = 0;
    profile->generation = 0;
    profile->next_sequence = 0;
    profile->sector_slots = 0;
    profile->appends_since_meta = 0;
    int meta_valid = read_meta(profile) == 0;
    uint16_t old_head = profile->head;
    uint16_t old_entries = profile->entries;
    uint16_t old_next = profile->next_sequence;
    int recovered = recover_records(profile);
    if (!meta_valid || (recovered && (old_head != profile->head
                                      || old_entries != profile->entries
                                      || old_next != profile->next_sequence))) {
        return write_meta(profile);
    }
    return 0;
}

static uint32_t slot_offset(const nd_profile_store_t *profile, uint16_t index)
{
    return profile->base_offset + PROFILE_META_AREA
        + (uint32_t)index * profile->slot_size;
}

int nd_profile_store_append(nd_profile_store_t *profile,
                            const nd_datetime_t *time, uint8_t status,
                            const uint8_t *record, size_t length)
{
    uint8_t packed[256];
    if (profile == NULL || time == NULL || record == NULL
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

    uint32_t offset = slot_offset(profile, profile->head);
    if (profile->sector_slots > 1 && profile->head != 0
        && profile->head % profile->sector_slots == 0) {
        uint32_t span = (uint32_t)profile->sector_slots * profile->slot_size;
        uint32_t start = offset - ((offset - profile->base_offset) % span);
        if (nd_store_erase(profile->store, start, span) != 0) {
            return -1;
        }
    }
    if (nd_store_write(profile->store, offset, packed,
                       PROFILE_RECORD_HEADER_SIZE + length) != 0) {
        return -1;
    }
    profile->head = (uint16_t)((profile->head + 1) % profile->capacity);
    if (profile->entries < profile->capacity) {
        profile->entries++;
    }
    profile->next_sequence++;
    if (++profile->appends_since_meta >= PROFILE_META_INTERVAL) {
        profile->appends_since_meta = 0;
        return write_meta(profile);
    }
    return 0;
}

int nd_profile_store_read(const nd_profile_store_t *profile, uint32_t index,
                          uint8_t *record, size_t capacity, size_t *length)
{
    if (profile == NULL || record == NULL || length == NULL
        || index >= profile->entries) {
        return -1;
    }

    uint16_t oldest = (uint16_t)((profile->head + profile->capacity
                                  - profile->entries) % profile->capacity);
    uint16_t slot = (uint16_t)((oldest + index) % profile->capacity);
    uint32_t offset = slot_offset(profile, slot);
    uint8_t header[PROFILE_RECORD_HEADER_SIZE];
    if (nd_store_read(profile->store, offset, header, sizeof(header)) != 0) {
        return -1;
    }
    uint16_t stored_length = get_u16(&header[13]);
    if (get_u32(&header[0]) != PROFILE_RECORD_MAGIC
        || (header[4] != 2u && header[4] != PROFILE_VERSION)
        || stored_length == 0
        || stored_length > profile->slot_size - PROFILE_RECORD_HEADER_SIZE
        || stored_length > capacity) {
        return -1;
    }

    if (nd_store_read(profile->store, offset + sizeof(header), record,
                      stored_length) != 0) {
        return -1;
    }
    if (get_u16(&header[18]) != crc16_update(crc16(header, 18), record,
                                             stored_length)) {
        return -1;
    }

    *length = stored_length;
    return 0;
}

int nd_profile_store_reset(nd_profile_store_t *profile)
{
    if (profile == NULL) {
        return -1;
    }
    profile->head = 0;
    profile->entries = 0;
    profile->generation = profile->next_sequence;
    profile->next_sequence = (uint16_t)(profile->generation + 1u);
    profile->appends_since_meta = 0;
    return write_meta(profile);
}

int nd_profile_store_sync(nd_profile_store_t *profile)
{
    if (profile == NULL) {
        return -1;
    }
    if (profile->appends_since_meta == 0) {
        return 0;
    }
    profile->appends_since_meta = 0;
    return write_meta(profile);
}

uint16_t nd_profile_store_entries(const nd_profile_store_t *profile)
{
    return profile == NULL ? 0 : profile->entries;
}

uint16_t nd_profile_store_capacity(const nd_profile_store_t *profile)
{
    return profile == NULL ? 0 : profile->capacity;
}

void nd_profile_store_set_sector_slots(nd_profile_store_t *profile,
                                       uint16_t slots)
{
    if (profile == NULL || profile->store == NULL
        || profile->store->erase == NULL || slots <= 1
        || slots > profile->capacity) {
        return;
    }
    profile->sector_slots = slots;
}
