/*
 * Bounded proof for the memory-backed byte store.
 *
 * byte_store.c is compiled directly, so the port read and write are
 * production code. The backing bytes and every request are symbolic. A
 * request outside the store is rejected before any byte moves, an accepted
 * read returns the stored bytes, and an accepted write changes exactly the
 * requested range. --bounds-check and --pointer-check turn any out-of-range
 * memcpy into a failed property.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "infra/memory/byte_store.c"

#define STORE_BYTES 8u

static uint8_t memory[STORE_BYTES]; /* --nondet-static: every byte symbolic */
static nd_byte_store_t store;

static int outside(uint32_t offset, size_t length)
{
    return offset > STORE_BYTES || length > STORE_BYTES - offset;
}

static void read_checks(void)
{
    uint32_t offset; /* CBMC models an uninitialized local as nondet */
    size_t length;
    uint8_t dest[STORE_BYTES];
    uint8_t dest_before[STORE_BYTES];
    uint8_t before[STORE_BYTES];

    memcpy(before, memory, STORE_BYTES);
    memcpy(dest_before, dest, STORE_BYTES);
    int result =
        nd_store_read(nd_byte_store_port(&store), offset, dest, length);
    if (outside(offset, length)) {
        __CPROVER_assert(result == -1, "an out-of-range read is rejected");
        __CPROVER_assert(memcmp(dest, dest_before, STORE_BYTES) == 0,
                         "a rejected read leaves the destination alone");
        __CPROVER_assert(memcmp(memory, before, STORE_BYTES) == 0,
                         "a rejected read leaves the backing alone");
        return;
    }
    __CPROVER_assert(result == 0, "an in-range read succeeds");
    for (uint32_t index = 0; index < STORE_BYTES; index++) {
        if (index < length) {
            __CPROVER_assert(dest[index] == before[offset + index],
                             "an in-range read returns the stored byte");
        } else {
            __CPROVER_assert(dest[index] == dest_before[index],
                             "an in-range read writes only length bytes");
        }
    }
}

static void write_checks(void)
{
    uint32_t offset;
    size_t length;
    uint8_t source[STORE_BYTES];
    uint8_t before[STORE_BYTES];

    memcpy(before, memory, STORE_BYTES);
    int result =
        nd_store_write(nd_byte_store_port(&store), offset, source, length);
    if (outside(offset, length)) {
        __CPROVER_assert(result == -1, "an out-of-range write is rejected");
        __CPROVER_assert(memcmp(memory, before, STORE_BYTES) == 0,
                         "a rejected write leaves the backing alone");
        return;
    }
    __CPROVER_assert(result == 0, "an in-range write succeeds");
    for (uint32_t index = 0; index < STORE_BYTES; index++) {
        if (index >= offset && index - offset < length) {
            __CPROVER_assert(memory[index] == source[index - offset],
                             "an in-range write stores the source byte");
        } else {
            __CPROVER_assert(memory[index] == before[index],
                             "an in-range write leaves the other bytes alone");
        }
    }
}

int main(void)
{
    nd_byte_store_init(&store, memory, STORE_BYTES);
    read_checks();
    write_checks();
    return 0;
}
