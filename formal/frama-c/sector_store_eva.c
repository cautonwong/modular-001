/*
 * Eva (abstract interpretation) proof for the memory-backed sector store.
 *
 * Run from the repository root:
 *   frama-c -eva -eva-slevel 512 -eva-min-loop-unroll 8 \
 *     -cpp-extra-args="-I ." formal/frama-c/sector_store_eva.c \
 *     infra/memory/sector_store.c
 *
 * The device is 8 bytes of NOR in two 4-byte sectors: erase fills a sector
 * with 0xFF, a program only clears bits (backing &= buffer), reads copy the
 * bytes back. Frama_C_interval_split
 * gives every offset and length its own abstract state (the split range
 * reaches one aligned out-of-range erase, 12, so that guard is also
 * executed), and -eva-min-loop-unroll unrolls the bounded loops. Each state
 * then has an exact sector base, so the sector arithmetic is decidable and
 * the checker can require 0 alarms, the pinned statement totals, and no
 * unknown or invalid logical property.
 *
 * The harness binds the geometry that nd_sector_store_init checks: capacity 8
 * is a whole number of 4-byte sectors, sector_size is a nonzero power of two,
 * and the sector buffer holds a sector. docs/guides/verification.md records
 * the fail-closed init rule.
 *
 * Failures are injected deterministically so the device-error return of each
 * store call is executed and asserted. The window check covers the erase
 * invalidation; the write path's value effect is proved in
 * formal/cbmc/sector_store.c, because memcmp's result is opaque to the value
 * domain and the store splits on it.
 *
 * Five statements stay unreached; the statement totals are pinned in
 * tools/check_formal.sh, and this list is the whole gap:
 * - store_read, store_write and store_erase each have the guard clause
 *   `store->sector == NULL`. A failed nd_sector_store_init zeroes the device
 *   half, so an earlier NULL-callback clause fires first; the sector clause
 *   only triggers on a store struct built or mutated outside init.
 * - nd_sector_store_init has two rejected-geometry assignments the harness
 *   does not take: a nonzero non-power-of-two sector_size, and a capacity
 *   that is not a whole number of sectors. The harness passes sector_size 0
 *   or 4 with capacity 8, so only the sector_size == 0 and
 *   sector_buffer == NULL rejects run.
 */
#include <stddef.h>
#include <stdint.h>
#include "__fc_builtin.h"
#include "infra/memory/sector_store.h"

#define CAP 8u
#define SECTOR 4u

static uint8_t backing[CAP];
static uint8_t window_buf[SECTOR];
static uint8_t direct_buf[SECTOR];
static uint8_t dst[CAP];
static uint8_t src[CAP];
static uint8_t windowed_read[SECTOR];
static uint8_t direct_read[SECTOR];

/* Deterministic fault injection: the harness sets a flag, the device obeys. */
static int fail_read;
static int fail_erase;
static int fail_write;

static int device_read(void *ctx, uint32_t offset, uint8_t *buffer,
                       size_t length)
{
    (void)ctx;
    if (offset > CAP || length > CAP - offset) {
        return -1;
    }
    if (fail_read != 0) {
        return -1;
    }
    for (size_t i = 0; i < length; i++) {
        buffer[i] = backing[offset + i];
    }
    return 0;
}

static int device_erase(void *ctx, uint32_t offset)
{
    (void)ctx;
    if (offset > CAP || SECTOR > CAP - offset) {
        return -1;
    }
    if (fail_erase != 0) {
        return -1;
    }
    for (size_t i = 0; i < SECTOR; i++) {
        backing[offset + i] = 0xFFu;
    }
    return 0;
}

static int device_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                        size_t length)
{
    (void)ctx;
    if (offset > CAP || length > CAP - offset) {
        return -1;
    }
    if (fail_write != 0) {
        return -1;
    }
    for (size_t i = 0; i < length; i++) {
        backing[offset + i] &= buffer[i];
    }
    return 0;
}

static void run(nd_sector_store_t *store, uint32_t offset, size_t length)
{
    (void)store->port.read(store->port.ctx, offset, dst, length);
    (void)store->port.read(store->port.ctx, offset, dst, length);
    (void)store->port.write(store->port.ctx, offset, src, length);
    (void)store->port.erase(store->port.ctx, offset, length);
}

/*
 * The erase invalidates the window: the read after it must refill from the
 * erased device instead of serving the bytes cached before the erase.
 */
static void check_window(nd_sector_store_t *windowed,
                         nd_sector_store_t *direct_store)
{
    backing[0] = 0x11u;
    backing[1] = 0x22u;
    backing[2] = 0x33u;
    backing[3] = 0x44u;
    (void)windowed->port.read(windowed->port.ctx, 0u, windowed_read, 4u);
    (void)windowed->port.erase(windowed->port.ctx, 0u, 4u);
    (void)windowed->port.read(windowed->port.ctx, 0u, windowed_read, 4u);
    (void)direct_store->port.read(direct_store->port.ctx, 0u, direct_read, 4u);
    for (uint32_t i = 0; i < SECTOR; i++) {
        /*@ assert erase_window_fresh: windowed_read[i] == direct_read[i]; */
        /*@ assert erased: windowed_read[i] == 0xFFu; */
    }
}

int main(void)
{
    nd_sector_store_t store;
    nd_sector_store_t direct;
    nd_sector_store_t null;
    nd_sector_store_t no_read;
    nd_sector_store_t no_erase;
    nd_sector_store_t no_write;
    nd_sector_store_t no_sector;
    nd_sector_store_t bad_geometry;

    Frama_C_make_unknown((char *)backing, CAP);
    Frama_C_make_unknown((char *)src, CAP);
    nd_sector_store_init(&store, NULL, device_read, device_erase,
                         device_write, CAP, SECTOR, window_buf, 1u);
    nd_sector_store_init(&direct, NULL, device_read, device_erase,
                         device_write, CAP, SECTOR, direct_buf, 0u);
    nd_sector_store_init(&null, NULL, NULL, NULL, NULL, CAP, SECTOR,
                         window_buf, 1u);
    nd_sector_store_init(&no_read, NULL, NULL, device_erase, device_write,
                         CAP, SECTOR, window_buf, 1u);
    nd_sector_store_init(&no_erase, NULL, device_read, NULL, device_write,
                         CAP, SECTOR, window_buf, 1u);
    nd_sector_store_init(&no_write, NULL, device_read, device_erase, NULL,
                         CAP, SECTOR, window_buf, 1u);
    nd_sector_store_init(&no_sector, NULL, device_read, device_erase,
                         device_write, CAP, SECTOR, NULL, 1u);
    /* The rejected geometry zeroes the device state, so every call fails -1. */
    nd_sector_store_init(&bad_geometry, NULL, device_read, device_erase,
                         device_write, CAP, 0u, window_buf, 1u);
    nd_sector_store_init(NULL, NULL, NULL, NULL, NULL, CAP, SECTOR,
                         window_buf, 1u);
    (void)nd_sector_store_port(&store);
    (void)nd_sector_store_port(NULL);

    uint32_t offset = (uint32_t)Frama_C_interval_split(0, CAP + 4);
    size_t length = (size_t)Frama_C_interval_split(0, CAP + 4);
    run(&store, offset, length);
    run(&direct, offset, length);
    run(&null, offset, length);
    run(&no_read, offset, length);
    run(&no_erase, offset, length);
    run(&no_write, offset, length);
    run(&no_sector, offset, length);
    run(&bad_geometry, offset, length);

    /* A window fill in one sector followed by a refill in the other. */
    (void)store.port.read(store.port.ctx, 0u, dst, 4u);
    (void)store.port.read(store.port.ctx, 4u, dst, 4u);

    check_window(&store, &direct);

    /* Every device failure the store propagates, one path at a time. */
    fail_read = 1;
    run(&store, offset, length);
    fail_read = 0;

    backing[0] = 0x00u; /* non-blank sector: read-modify-erase-write path */
    fail_erase = 1;
    (void)store.port.write(store.port.ctx, 0u, src, 2u);
    fail_erase = 0;
    fail_write = 1;
    (void)store.port.write(store.port.ctx, 0u, src, 2u);
    fail_write = 0;

    backing[0] = 0xFFu; /* blank sector: direct write path */
    (void)store.port.write(store.port.ctx, 0u, src, 2u);
    fail_write = 1;
    (void)store.port.write(store.port.ctx, 0u, src, 2u);
    fail_write = 0;

    fail_erase = 1;
    int erase_result = store.port.erase(store.port.ctx, 0u, 4u);
    /*@ assert erase_failure_reported: erase_result == -1; */
    fail_erase = 0;

    (void)store.port.read(NULL, offset, dst, length);
    (void)store.port.write(NULL, offset, src, length);
    (void)store.port.erase(NULL, offset, length);
    (void)device_read(NULL, CAP + 1, dst, 1);
    (void)device_read(NULL, 0, dst, CAP + 1);
    (void)device_erase(NULL, CAP + 1);
    (void)device_erase(NULL, CAP);
    (void)device_write(NULL, CAP + 1, src, 1);
    (void)device_write(NULL, 0, src, CAP + 1);
    return 0;
}
