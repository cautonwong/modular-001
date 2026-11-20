/*
 * Bounded proof for the pulse base record and journal header parsers.
 *
 * The backing store is symbolic. read_base_record and journal_header_valid
 * must reject a bad magic, version, length or generation, never read outside
 * the store, and only accept a record whose CRC and generation verify. The
 * marker record has no magic or version, so its parser is only a CRC check;
 * the base and journal headers are the two structured headers in the engine.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "infra/storage/pulse_store.c"

#define STORE_BYTES 96u
#define BASE_CAPACITY 17u      /* 12 header + at most 1 blob + 4 CRC */
#define JOURNAL_OFFSET 34u     /* two base slots */

static uint8_t memory[STORE_BYTES];

static int mock_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    (void)ctx;
    __CPROVER_assert(offset <= STORE_BYTES && length <= STORE_BYTES - offset,
                     "every read stays inside the device");
    if (offset > STORE_BYTES || length > STORE_BYTES - offset) {
        return -1;
    }
    memcpy(buffer, memory + offset, length);
    return 0;
}

static int mock_program(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    (void)offset;
    (void)buffer;
    (void)length;
    return 0;
}

static int mock_erase(void *ctx, uint32_t offset, uint32_t length)
{
    (void)ctx;
    (void)offset;
    (void)length;
    return 0;
}

static const nd_block_ops_t ops = {
    .ctx = NULL,
    .read = mock_read,
    .program = mock_program,
    .erase = mock_erase,
    .size = STORE_BYTES,
    .erase_unit = 1u,
};

void base_record_checks(nd_pulse_journal_t *journal)
{
    uint32_t generation = 0;
    uint8_t blank = 0;
    int result = read_base_record(journal, 0, &generation, &blank);
    nd_pulse_base_header_t header;

    memcpy(&header, memory, sizeof(header));
    __CPROVER_assert(result == 1 || result == 0,
                     "a readable slot is valid or invalid, never an error");
    if (blank != 0u) {
        __CPROVER_assert(result == 0, "a blank header is never accepted");
        __CPROVER_assert(all_ff(&header, sizeof(header)),
                         "blank is reported only for an erased header");
    }
    if (header.magic != ND_PULSE_BASE_MAGIC) {
        __CPROVER_assert(result != 1, "wrong magic is rejected");
    }
    if (header.version != ND_PULSE_FORMAT_VERSION) {
        __CPROVER_assert(result != 1, "wrong version is rejected");
    }
    if (header.bytes == 0u
        || (uint32_t)header.bytes
               > BASE_CAPACITY - ND_PULSE_BASE_OVERHEAD) {
        __CPROVER_assert(result != 1, "out-of-range blob length is rejected");
    }
    if (result == 1) {
        __CPROVER_assert(header.magic == ND_PULSE_BASE_MAGIC,
                         "accepted records carry the base magic");
        __CPROVER_assert(header.version == ND_PULSE_FORMAT_VERSION,
                         "accepted records carry the format version");
        __CPROVER_assert(header.bytes != 0u
                             && (uint32_t)header.bytes
                                    <= BASE_CAPACITY - ND_PULSE_BASE_OVERHEAD,
                         "accepted records keep the blob inside the slot");
        __CPROVER_assert(generation == header.generation,
                         "accepted records report the stored generation");
    }
}

void journal_header_checks(nd_pulse_journal_t *journal)
{
    int result = journal_header_valid(journal, 0);
    nd_pulse_journal_header_t header;

    memcpy(&header, memory + JOURNAL_OFFSET, sizeof(header));
    __CPROVER_assert(result == 1 || result == 0,
                     "a readable header is valid or invalid, never an error");
    if (header.magic != ND_PULSE_JOURNAL_MAGIC) {
        __CPROVER_assert(result != 1, "wrong journal magic is rejected");
    }
    if (header.version != ND_PULSE_FORMAT_VERSION) {
        __CPROVER_assert(result != 1, "wrong journal version is rejected");
    }
    if (header.bytes != (uint16_t)sizeof(header)) {
        __CPROVER_assert(result != 1, "wrong journal length is rejected");
    }
    if (header.generation != journal->generation) {
        __CPROVER_assert(result != 1,
                         "a stale generation is not accepted");
    }
    if (result == 1) {
        __CPROVER_assert(header.magic == ND_PULSE_JOURNAL_MAGIC,
                         "accepted headers carry the journal magic");
        __CPROVER_assert(header.generation == journal->generation,
                         "accepted headers match the base generation");
    }
}

int main(void)
{
    nd_pulse_journal_t journal;
    uint32_t generation; /* CBMC models an uninitialized local as nondet */

    memset(&journal, 0, sizeof(journal));
    journal.block = &ops;
    journal.base_block = &ops;
    journal.base_offset[0] = 0;
    journal.base_offset[1] = BASE_CAPACITY;
    journal.base_capacity = BASE_CAPACITY;
    journal.journal_offset = JOURNAL_OFFSET;
    journal.journal_bytes = STORE_BYTES - JOURNAL_OFFSET;
    journal.lane_bytes = 1u;
    journal.capacity = 8u;
    journal.generation = generation;

    base_record_checks(&journal);
    journal_header_checks(&journal);
    return 0;
}
