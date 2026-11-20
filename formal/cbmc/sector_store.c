/*
 * Bounded proof for the sector store's guard, read-through window and
 * read-modify-erase-write paths.
 *
 * sector_store.c is compiled directly. The device is byte-addressed NOR: an
 * erase fills one sector with 0xFF, and a program only clears bits
 * (memory &= buffer), so a write to a non-blank cell cannot silently pass as
 * the requested bytes. That is what makes the erase-before-program path
 * load-bearing: with the read-modify-erase-write branch removed, a rewrite of
 * a non-blank sector is proved to fail. The proof runs on two 4-byte sectors,
 * the geometry nd_sector_store_init now checks: sector_size is a nonzero power
 * of two, capacity is a whole number of sectors, and the sector buffer holds a
 * sector. The binding uses 4096-byte sectors over a whole number of sectors.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "infra/memory/sector_store.c"

#define DEVICE_BYTES 8u
#define SECTOR_BYTES 4u

static uint8_t memory[DEVICE_BYTES]; /* --nondet-static: every byte symbolic */
static uint8_t window_sector[DEVICE_BYTES];
static uint8_t direct_sector[DEVICE_BYTES];
static nd_sector_store_t windowed;
static nd_sector_store_t direct;

static int device_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    __CPROVER_assert(offset <= DEVICE_BYTES && length <= DEVICE_BYTES - offset,
                     "every device read stays inside the device");
    if (offset > DEVICE_BYTES || length > DEVICE_BYTES - offset) {
        return -1;
    }
    memcpy(buffer, memory + offset, length);
    return 0;
}

static int device_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    __CPROVER_assert((offset & (SECTOR_BYTES - 1u)) == 0
                         && offset < DEVICE_BYTES
                         && SECTOR_BYTES <= DEVICE_BYTES - offset,
                     "every erased sector stays inside the device");
    if ((offset & (SECTOR_BYTES - 1u)) != 0 || offset >= DEVICE_BYTES
        || SECTOR_BYTES > DEVICE_BYTES - offset) {
        return -1;
    }
    memset(memory + offset, 0xFF, SECTOR_BYTES);
    return 0;
}

static int device_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    __CPROVER_assert(offset <= DEVICE_BYTES && length <= DEVICE_BYTES - offset,
                     "every device write stays inside the device");
    if (offset > DEVICE_BYTES || length > DEVICE_BYTES - offset) {
        return -1;
    }
    for (size_t index = 0; index < length; index++) {
        memory[offset + index] &= buffer[index];
    }
    return 0;
}

static int failing_read(void *ctx, uint32_t offset, uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    (void)offset;
    (void)buffer;
    (void)length;
    return -1;
}

static int failing_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    (void)offset;
    return -1;
}

static int failing_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                         size_t length)
{
    (void)ctx;
    (void)offset;
    (void)buffer;
    (void)length;
    return -1;
}

static void init_stores(void)
{
    nd_sector_store_init(&windowed, NULL, device_read, device_erase,
                         device_write, DEVICE_BYTES, SECTOR_BYTES,
                         window_sector, 1u);
    nd_sector_store_init(&direct, NULL, device_read, device_erase,
                         device_write, DEVICE_BYTES, SECTOR_BYTES,
                         direct_sector, 0u);
}

static int range_outside(uint32_t offset, uint32_t length)
{
    return offset > DEVICE_BYTES || length > DEVICE_BYTES - offset;
}

static int erase_outside(uint32_t offset, uint32_t length)
{
    return (offset & (SECTOR_BYTES - 1u)) != 0
        || (length % SECTOR_BYTES) != 0
        || range_outside(offset, length);
}

static void read_checks(void)
{
    uint32_t offset; /* CBMC models an uninitialized local as nondet */
    uint32_t length;
    uint8_t dest[DEVICE_BYTES];
    uint8_t dest_before[DEVICE_BYTES];
    uint8_t before[DEVICE_BYTES];

    memcpy(before, memory, DEVICE_BYTES);
    memcpy(dest_before, dest, DEVICE_BYTES);
    int result =
        nd_store_read(nd_sector_store_port(&windowed), offset, dest, length);
    if (range_outside(offset, length)) {
        __CPROVER_assert(result == -1, "an out-of-range read is rejected");
        __CPROVER_assert(memcmp(dest, dest_before, DEVICE_BYTES) == 0,
                         "a rejected read leaves the destination alone");
        __CPROVER_assert(memcmp(memory, before, DEVICE_BYTES) == 0,
                         "a rejected read leaves the backing alone");
        return;
    }
    __CPROVER_assert(result == 0, "an in-range read succeeds");
    for (uint32_t index = 0; index < DEVICE_BYTES; index++) {
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
    uint32_t length;
    uint8_t source[DEVICE_BYTES];
    uint8_t before[DEVICE_BYTES];

    memcpy(before, memory, DEVICE_BYTES);
    int result =
        nd_store_write(nd_sector_store_port(&windowed), offset, source,
                       length);
    if (range_outside(offset, length)) {
        __CPROVER_assert(result == -1, "an out-of-range write is rejected");
        __CPROVER_assert(memcmp(memory, before, DEVICE_BYTES) == 0,
                         "a rejected write leaves the backing alone");
        return;
    }
    __CPROVER_assert(result == 0, "an in-range write succeeds");
    for (uint32_t index = 0; index < DEVICE_BYTES; index++) {
        if (index >= offset && index - offset < length) {
            __CPROVER_assert(memory[index] == source[index - offset],
                             "an in-range write stores the source byte");
        } else {
            __CPROVER_assert(memory[index] == before[index],
                             "a write preserves every byte outside its range");
        }
    }
}

static void erase_checks(void)
{
    uint32_t offset;
    uint32_t length;
    uint8_t before[DEVICE_BYTES];

    memcpy(before, memory, DEVICE_BYTES);
    int result =
        nd_store_erase(nd_sector_store_port(&windowed), offset, length);
    if (erase_outside(offset, length)) {
        __CPROVER_assert(result == -1,
                         "an unaligned or out-of-range erase is rejected");
        __CPROVER_assert(memcmp(memory, before, DEVICE_BYTES) == 0,
                         "a rejected erase leaves the backing alone");
        return;
    }
    __CPROVER_assert(result == 0, "an aligned in-range erase succeeds");
    for (uint32_t index = 0; index < DEVICE_BYTES; index++) {
        if (index >= offset && index - offset < length) {
            __CPROVER_assert(memory[index] == 0xFFu,
                             "an erase leaves the requested bytes erased");
        } else {
            __CPROVER_assert(memory[index] == before[index],
                             "an erase leaves the other sectors alone");
        }
    }
}

/*
 * The window path only runs for a nonzero read inside one sector that has a
 * full sector behind it. The second read is a cache hit; both must agree with
 * a direct read of the same range.
 */
static void window_read_matches_direct(void)
{
    uint32_t offset;
    uint32_t length;
    uint32_t base;
    uint8_t first[DEVICE_BYTES];
    uint8_t second[DEVICE_BYTES];
    uint8_t plain[DEVICE_BYTES];

    init_stores();
    __CPROVER_assume(length > 0u && !range_outside(offset, length));
    __CPROVER_assume(length <= SECTOR_BYTES);
    base = offset & ~(SECTOR_BYTES - 1u);
    __CPROVER_assume(((offset + length - 1u) & ~(SECTOR_BYTES - 1u)) == base);
    __CPROVER_assume(DEVICE_BYTES - base >= SECTOR_BYTES);

    __CPROVER_assert(nd_store_read(nd_sector_store_port(&windowed), offset,
                                   first, length) == 0,
                     "the windowed read succeeds");
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&windowed), offset,
                                   second, length) == 0,
                     "the cached read succeeds");
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&direct), offset,
                                   plain, length) == 0,
                     "the direct read succeeds");
    for (uint32_t index = 0; index < DEVICE_BYTES; index++) {
        if (index < length) {
            __CPROVER_assert(first[index] == plain[index],
                             "a window fill returns the direct bytes");
            __CPROVER_assert(second[index] == plain[index],
                             "a window hit returns the direct bytes");
        }
    }
}

/*
 * A write and an erase both invalidate the window. Reading the sector before
 * and after proves it: a stale window would answer with the old bytes.
 */
static void write_invalidates_window(void)
{
    uint32_t base;
    uint32_t offset;
    uint32_t length;
    uint8_t source[DEVICE_BYTES];
    uint8_t before[DEVICE_BYTES];
    uint8_t after[DEVICE_BYTES];
    uint8_t plain[DEVICE_BYTES];

    init_stores();
    __CPROVER_assume((base & (SECTOR_BYTES - 1u)) == 0);
    __CPROVER_assume(base < DEVICE_BYTES
                     && DEVICE_BYTES - base >= SECTOR_BYTES);
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&windowed), base,
                                   before, SECTOR_BYTES) == 0,
                     "the sector read fills the window");
    __CPROVER_assume(offset >= base && offset - base < SECTOR_BYTES);
    __CPROVER_assume(length > 0u && length <= SECTOR_BYTES - (offset - base));
    __CPROVER_assert(nd_store_write(nd_sector_store_port(&windowed), offset,
                                    source, length) == 0,
                     "the in-sector write succeeds");
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&windowed), base,
                                   after, SECTOR_BYTES) == 0,
                     "the post-write sector read succeeds");
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&direct), base, plain,
                                   SECTOR_BYTES) == 0,
                     "the direct sector read succeeds");
    for (uint32_t index = 0; index < DEVICE_BYTES; index++) {
        if (index < SECTOR_BYTES) {
            __CPROVER_assert(after[index] == plain[index],
                             "a read after a write sees the new sector bytes");
        }
    }
}

static void erase_invalidates_window(void)
{
    uint32_t base;
    uint8_t before[DEVICE_BYTES];
    uint8_t after[DEVICE_BYTES];
    uint8_t plain[DEVICE_BYTES];

    init_stores();
    __CPROVER_assume((base & (SECTOR_BYTES - 1u)) == 0);
    __CPROVER_assume(base < DEVICE_BYTES
                     && DEVICE_BYTES - base >= SECTOR_BYTES);
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&windowed), base,
                                   before, SECTOR_BYTES) == 0,
                     "the sector read fills the window");
    __CPROVER_assert(nd_store_erase(nd_sector_store_port(&windowed), base,
                                    SECTOR_BYTES) == 0,
                     "the sector erase succeeds");
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&windowed), base,
                                   after, SECTOR_BYTES) == 0,
                     "the post-erase sector read succeeds");
    __CPROVER_assert(nd_store_read(nd_sector_store_port(&direct), base, plain,
                                   SECTOR_BYTES) == 0,
                     "the direct sector read succeeds");
    for (uint32_t index = 0; index < DEVICE_BYTES; index++) {
        if (index < SECTOR_BYTES) {
            __CPROVER_assert(after[index] == plain[index],
                             "a read after an erase sees the erased sector");
        }
    }
}

/*
 * Each device callback fails on its own store, so the device-error return of
 * store_read (window fill and direct read), store_erase and each device call
 * of store_write is proved to reach the caller instead of a silent success.
 */
static void fault_checks(void)
{
    nd_sector_store_t no_read;
    nd_sector_store_t no_erase;
    nd_sector_store_t no_write;
    uint8_t source[DEVICE_BYTES] = {0x5Au, 0x5Au, 0x5Au, 0x5Au,
                                    0x5Au, 0x5Au, 0x5Au, 0x5Au};
    uint8_t dest[DEVICE_BYTES];
    uint8_t before[DEVICE_BYTES];
    int result;

    nd_sector_store_init(&no_read, NULL, failing_read, device_erase,
                         device_write, DEVICE_BYTES, SECTOR_BYTES,
                         window_sector, 1u);
    result = nd_store_read(nd_sector_store_port(&no_read), 0u, dest, 1u);
    __CPROVER_assert(result == -1, "a device read failure fails the read");
    result = nd_store_write(nd_sector_store_port(&no_read), 0u, source, 1u);
    __CPROVER_assert(result == -1,
                     "a device read failure fails the write's sector read");
    result = nd_store_read(nd_sector_store_port(&no_read), 0u, dest,
                           DEVICE_BYTES);
    __CPROVER_assert(result == -1,
                     "a device read failure fails a direct read");

    nd_sector_store_init(&no_erase, NULL, device_read, failing_erase,
                         device_write, DEVICE_BYTES, SECTOR_BYTES,
                         window_sector, 1u);
    memory[0] = 0x00u;
    memcpy(before, memory, DEVICE_BYTES);
    result = nd_store_write(nd_sector_store_port(&no_erase), 0u, source, 1u);
    __CPROVER_assert(result == -1,
                     "a device erase failure fails the read-modify-write");
    __CPROVER_assert(memcmp(memory, before, DEVICE_BYTES) == 0,
                     "a failed erase leaves the device alone");
    result = nd_store_erase(nd_sector_store_port(&no_erase), 0u, SECTOR_BYTES);
    __CPROVER_assert(result == -1, "a device erase failure fails the erase");
    __CPROVER_assert(memcmp(memory, before, DEVICE_BYTES) == 0,
                     "a failed erase leaves the device alone");

    nd_sector_store_init(&no_write, NULL, device_read, device_erase,
                         failing_write, DEVICE_BYTES, SECTOR_BYTES,
                         window_sector, 1u);
    memset(memory, 0xFF, DEVICE_BYTES);
    memcpy(before, memory, DEVICE_BYTES);
    result = nd_store_write(nd_sector_store_port(&no_write), 0u, source, 1u);
    __CPROVER_assert(result == -1,
                     "a device write failure fails the blank-sector write");
    __CPROVER_assert(memcmp(memory, before, DEVICE_BYTES) == 0,
                     "a failed blank-sector write leaves the device alone");

    memory[0] = 0x00u;
    result = nd_store_write(nd_sector_store_port(&no_write), 0u, source, 1u);
    __CPROVER_assert(result == -1,
                     "a device write failure fails the read-modify-write");
}

int main(void)
{
    init_stores();
    read_checks();
    write_checks();
    erase_checks();
    window_read_matches_direct();
    write_invalidates_window();
    erase_invalidates_window();
    fault_checks();
    return 0;
}
