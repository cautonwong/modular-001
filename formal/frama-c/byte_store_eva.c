/*
 * Eva (abstract interpretation) proof for the memory-backed byte store.
 *
 * Run from the repository root:
 *   frama-c -eva -eva-slevel 512 -eva-min-loop-unroll 8 \
 *     -cpp-extra-args="-I ." formal/frama-c/byte_store_eva.c \
 *     infra/memory/byte_store.c
 *
 * byte_store.c is compiled as production code; requests come from symbolic
 * offsets and lengths. Frama_C_interval_split gives every value its own
 * abstract state and -eva-min-loop-unroll unrolls the fixed 8-byte loops, so
 * each offset and length is exact, the range arithmetic is decidable, and
 * the copied bytes can be compared. The checker requires 0 alarms, 100%
 * statements reached and no unknown or invalid logical property.
 *
 * The contents are a fixed pattern rather than symbolic bytes: the value
 * domain cannot prove equality between two non-singleton abstract values, so
 * a symbolic-content proof stays with formal/cbmc/byte_store.c. Here the
 * symbolic part is the request, which is what the guards and range copies
 * depend on.
 */
#include <stddef.h>
#include <stdint.h>
#include "__fc_builtin.h"
#include "infra/memory/byte_store.h"

#define CAP 8u

static nd_byte_store_t store;
static uint8_t backing[CAP];
static uint8_t dst[CAP];
static uint8_t src[CAP];
static uint8_t before[CAP];
static uint8_t dst_before[CAP];

static void snapshot(uint8_t *into, const uint8_t *from)
{
    for (uint32_t i = 0; i < CAP; i++) {
        into[i] = from[i];
    }
}

static void check_read(void)
{
    uint32_t offset = (uint32_t)Frama_C_interval_split(0, CAP + 2);
    size_t length = (size_t)Frama_C_interval_split(0, CAP + 2);

    snapshot(before, backing);
    snapshot(dst_before, dst);
    int result = store.port.read(store.port.ctx, offset, dst, length);
    if (offset > CAP || length > CAP - offset) {
        /*@ assert rejected: result == -1; */
        for (uint32_t i = 0; i < CAP; i++) {
            /*@ assert dest_untouched: dst[i] == dst_before[i]; */
        }
        for (uint32_t i = 0; i < CAP; i++) {
            /*@ assert source_untouched: backing[i] == before[i]; */
        }
    } else {
        /*@ assert accepted: result == 0; */
        for (uint32_t i = 0; i < CAP; i++) {
            if (i < length) {
                /*@ assert read_byte: dst[i] == before[offset + i]; */
            } else {
                /*@ assert dest_tail: dst[i] == dst_before[i]; */
            }
        }
    }
}

static void check_write(void)
{
    uint32_t offset = (uint32_t)Frama_C_interval_split(0, CAP + 2);
    size_t length = (size_t)Frama_C_interval_split(0, CAP + 2);

    snapshot(before, backing);
    int result = store.port.write(store.port.ctx, offset, src, length);
    if (offset > CAP || length > CAP - offset) {
        /*@ assert rejected_write: result == -1; */
        for (uint32_t i = 0; i < CAP; i++) {
            /*@ assert write_untouched: backing[i] == before[i]; */
        }
    } else {
        /*@ assert accepted_write: result == 0; */
        for (uint32_t i = 0; i < CAP; i++) {
            if (i >= offset && i - offset < length) {
                /*@ assert written_byte: backing[i] == src[i - offset]; */
            } else {
                /*@ assert written_tail: backing[i] == before[i]; */
            }
        }
    }
}

int main(void)
{
    for (uint32_t i = 0; i < CAP; i++) {
        backing[i] = (uint8_t)(i + 1u);
        src[i] = (uint8_t)(i + 101u);
        dst[i] = 0xEEu;
    }
    nd_byte_store_init(&store, backing, CAP);
    (void)nd_byte_store_port(&store);
    check_read();
    check_write();
    return 0;
}
