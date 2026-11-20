/*
 * Bounded proof for the profile record slot reader.
 *
 * nd_profile_store.c is compiled directly, so read_slot, its CRC and
 * nd_profile_store_read are the production code. --nondet-static makes the
 * nine decision bytes of every slot symbolic: magic, version, length and
 * stored CRC. Those are the only inputs that steer accept or reject, so the
 * harness covers every header decision a real sector can hold. The payload
 * and filler bytes are deterministic: their content does not change any
 * bound, and a symbolic 108-byte payload only makes the CRC arithmetic
 * intractable. The reader must reject a malformed slot, keep an accepted
 * payload inside slot_size - 20 and inside the caller's capacity, and never
 * read outside the store.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "targets/idis_ip_meter/cosem/nd_profile_store.c"

#define STORE_BYTES 1024u
#define SLOT_BYTES 128u
#define PHYS_SLOTS 8u
#define ENTRIES_MAX 6u
#define SECTOR_SLOTS 2u
#define CALLER_CAPACITY 256u
#define PAYLOAD_MAX (SLOT_BYTES - PROFILE_RECORD_HEADER_SIZE)
#define SYMBOL_BYTES 9u

static uint8_t memory[STORE_BYTES];
static uint8_t symbols[PHYS_SLOTS][SYMBOL_BYTES];
static unsigned int read_calls;

static int mock_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    (void)ctx;
    read_calls++;
    __CPROVER_assert(offset <= STORE_BYTES && length <= STORE_BYTES - offset,
                     "every store read stays inside the device");
    if (offset > STORE_BYTES || length > STORE_BYTES - offset) {
        return -1;
    }
    memcpy(buffer, memory + offset, length);
    return 0;
}

static int mock_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                      size_t length)
{
    (void)ctx;
    (void)offset;
    (void)buffer;
    (void)length;
    return 0;
}

static int mock_erase(void *ctx, uint32_t offset, size_t length)
{
    (void)ctx;
    (void)offset;
    (void)length;
    return 0;
}

static const nd_store_t store = {
    .ctx = NULL,
    .read = mock_read,
    .write = mock_write,
    .erase = mock_erase,
};

static nd_profile_store_t make_profile(void)
{
    return (nd_profile_store_t){
        .store = &store,
        .base_offset = 0u,
        .capacity = ENTRIES_MAX,
        .slot_size = SLOT_BYTES,
        .sector_slots = SECTOR_SLOTS,
        .head = 0u,
        .entries = 0u,
        .next_sequence = 1u,
    };
}

static void seed_memory(void)
{
    for (uint16_t slot = 0; slot < PHYS_SLOTS; slot++) {
        for (uint32_t index = 0; index < SLOT_BYTES; index++) {
            memory[(uint32_t)slot * SLOT_BYTES + index] =
                (uint8_t)((index * 7u + slot) & 0xFFu);
        }
        uint8_t *raw = memory + (uint32_t)slot * SLOT_BYTES;
        raw[0] = symbols[slot][0];
        raw[1] = symbols[slot][1];
        raw[2] = symbols[slot][2];
        raw[3] = symbols[slot][3];
        raw[4] = symbols[slot][4];
        raw[13] = symbols[slot][5];
        raw[14] = symbols[slot][6];
        raw[18] = symbols[slot][7];
        raw[19] = symbols[slot][8];
    }
}

static void read_slot_checks(void)
{
    nd_profile_store_t profile = make_profile();
    struct slot_view view;
    uint8_t payload[CALLER_CAPACITY];
    size_t capacity;
    uint16_t slot;

    __CPROVER_assume(slot < PHYS_SLOTS);
    __CPROVER_assume(capacity <= CALLER_CAPACITY);

    int result = read_slot(&profile, slot, &view, payload, capacity);

    const uint8_t *raw = memory + (uint32_t)slot * SLOT_BYTES;
    uint16_t length = get_u16(&raw[13]);
    uint32_t magic = get_u32(raw);
    int version_ok = raw[4] == PROFILE_VERSION;
    int record_shape = magic == PROFILE_RECORD_MAGIC && length > 0u
        && length <= PAYLOAD_MAX;
    int mark_shape = magic == PROFILE_MARK_MAGIC && length == 0u;
    int record_crc_ok = 0;
    int mark_crc_ok = 0;
    if (version_ok && record_shape) {
        record_crc_ok = get_u16(&raw[18])
            == crc16_update(crc16(raw, 18),
                            &raw[PROFILE_RECORD_HEADER_SIZE], length);
    }
    if (version_ok && mark_shape) {
        mark_crc_ok = get_u16(&raw[18]) == crc16(raw, 18);
    }

    __CPROVER_assert(result == SLOT_OK || result == SLOT_INVALID
                         || result == SLOT_DEVICE_ERROR,
                     "read_slot returns a defined status");
    if (!version_ok) {
        __CPROVER_assert(result != 0, "a wrong version is rejected");
    }
    if (version_ok && magic == PROFILE_RECORD_MAGIC && !record_shape) {
        __CPROVER_assert(result != 0,
                         "a record length outside 1..slot_size-20 is rejected");
    }
    if (version_ok && record_shape && !record_crc_ok) {
        __CPROVER_assert(result != 0, "a record with a bad CRC is rejected");
    }
    if (version_ok && mark_shape && !mark_crc_ok) {
        __CPROVER_assert(result != 0, "a mark with a bad CRC is rejected");
    }
    if (version_ok && !record_shape && !mark_shape) {
        __CPROVER_assert(result != 0, "an unknown slot is rejected");
    }
    if (result == 0) {
        __CPROVER_assert(version_ok, "accepted slots carry the version");
        __CPROVER_assert(view.kind == SLOT_RECORD || view.kind == SLOT_MARK,
                         "accepted slots carry a known kind");
        __CPROVER_assert(view.length <= PAYLOAD_MAX,
                         "accepted payloads stay inside the slot");
        if (view.kind == SLOT_RECORD) {
            __CPROVER_assert(record_shape && record_crc_ok,
                             "accepted records carry a valid length and CRC");
            __CPROVER_assert(view.length <= capacity,
                             "the payload copy stays inside the caller buffer");
            for (size_t index = 0; index < view.length; index++) {
                __CPROVER_assert(payload[index] == raw[20 + index],
                                 "the payload copy is the stored payload");
            }
        } else {
            __CPROVER_assert(mark_shape && mark_crc_ok,
                             "accepted marks carry a valid CRC");
            __CPROVER_assert(view.length == 0, "accepted marks have no payload");
        }
    }
}

static void store_read_rejects_out_of_window(void)
{
    nd_profile_store_t profile = make_profile();
    uint8_t record[CALLER_CAPACITY];
    uint16_t entries;
    uint16_t head;
    uint32_t index;
    size_t capacity;
    size_t length = 0;

    __CPROVER_assume(entries <= ENTRIES_MAX);
    __CPROVER_assume(head < PHYS_SLOTS);
    __CPROVER_assume(capacity <= CALLER_CAPACITY);
    profile.entries = entries;
    profile.head = head;

    __CPROVER_assume(index >= profile.entries);
    read_calls = 0;
    int result =
        nd_profile_store_read(&profile, index, record, capacity, &length);
    __CPROVER_assert(result == -1,
                     "an index outside the live window is rejected");
    __CPROVER_assert(read_calls == 0, "a rejected index reads no slot");
}

static void store_read_reads_in_window(void)
{
    nd_profile_store_t profile = make_profile();
    uint8_t record[CALLER_CAPACITY];
    uint16_t entries;
    uint16_t head;
    uint32_t index;
    size_t capacity;
    size_t length = 0;

    __CPROVER_assume(entries <= ENTRIES_MAX);
    __CPROVER_assume(head < PHYS_SLOTS);
    __CPROVER_assume(capacity <= CALLER_CAPACITY);
    profile.entries = entries;
    profile.head = head;

    __CPROVER_assume(index < profile.entries);
    read_calls = 0;
    int result =
        nd_profile_store_read(&profile, index, record, capacity, &length);
    __CPROVER_assert(result == 0 || result == -1,
                     "nd_profile_store_read returns 0 or -1");
    /* store.c is compiled alongside this harness; without it nd_store_read is
     * an unlinked stub, mock_read never runs, and the read-path properties
     * become vacuous. This assert fails on that silent loss. */
    __CPROVER_assert(read_calls == 1,
                     "the accepted read goes through the store port");
    if (result == 0) {
        __CPROVER_assert(length > 0 && length <= PAYLOAD_MAX,
                         "a record payload stays inside the slot");
        __CPROVER_assert(length <= capacity,
                         "the record copy stays inside the caller buffer");
    }
}

int main(void)
{
    seed_memory();
    read_slot_checks();
    store_read_rejects_out_of_window();
    store_read_reads_in_window();
    return 0;
}
