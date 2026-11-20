#ifndef ND_DOMAIN_METERING_PULSE_STORE_H
#define ND_DOMAIN_METERING_PULSE_STORE_H

#include <stddef.h>
#include <stdint.h>

/* Consumer-owned port. The engine, its journal geometry, its markers and its
 * device are behind the handle. A lane is an encoded direction and tariff
 * byte from pulse_lane.h. */
typedef struct nd_pulse_store nd_pulse_store_t;

typedef enum {
    ND_PULSE_OK = 0,
    ND_PULSE_EMPTY = 1, /* base(): no base has been committed yet */
    ND_PULSE_FULL = 2,  /* append(): not accepted; commit and retry */
    /* The operation failed. base() uses it for a non-blank region with no
     * valid base: a known torn state. Only the snapshot engine's commit may
     * replace that state with generation 1; the journal engine requires a
     * known valid base and returns ERROR from commit instead. */
    ND_PULSE_ERROR = -1,
    /* base(): a device read failed, so the region is unknown. Retry later;
     * never treat it as EMPTY or as a torn region to adopt over. commit():
     * the snapshot engine returns it when the base scan read fails; the
     * journal engine returns ERROR for the same fault. */
    ND_PULSE_IO_ERROR = -2
} nd_pulse_status_t;

/* Replay yields the pulses recorded since the last commit, per lane. */
typedef void (*nd_pulse_replay_fn)(void *ctx, uint8_t lane, uint32_t count);

/* OK means accepted. At program granularity 1 the pulse is durable when this
 * returns; at granularity N up to N-1 accepted pulses may be lost in a crash.
 * FULL means nothing was written: commit, then retry this pulse. ERROR means
 * the pulse is not durably accepted; do not retry the same pulse, count it in
 * the book and commit at the next safe point. */
nd_pulse_status_t nd_pulse_store_append(nd_pulse_store_t *store, uint8_t lane);

/* Read-only scan of the journal since the last commit. A corrupt journal is
 * reported as EMPTY by append/commit-driven recovery, never rewritten here. */
nd_pulse_status_t nd_pulse_store_replay(nd_pulse_store_t *store,
                                        nd_pulse_replay_fn sink, void *ctx);

/* Copy the blob written by the last commit. EMPTY means the caller must
 * migrate from the legacy record and commit once. ERROR means the region is
 * non-blank but carries no valid base. Only the snapshot engine's commit can
 * heal ERROR with generation 1; the journal engine requires a known valid
 * base and refuses. IO_ERROR means a device read failed: retry later and
 * never adopt a legacy record on it, because an error that clears before the
 * commit would write the legacy value over a real base. On FULL, *length is
 * the required capacity. */
nd_pulse_status_t nd_pulse_store_base(nd_pulse_store_t *store, uint8_t *buffer,
                                      size_t capacity, size_t *length);

/* Write blob as the new base, then reset the journal. The blob is opaque and
 * must already include every pulse append accepted since the previous commit.
 * Returns OK when the base is durable; a failed journal reset leaves the next
 * append to re-initialize it. A base scan read fault refuses the commit: the
 * snapshot engine returns IO_ERROR, the journal engine ERROR. The journal
 * also refuses when no base slot is valid, which is the torn state ERROR
 * describes; only the snapshot engine heals that state with generation 1.
 * No engine writes a generation that did not come from a completed scan. */
nd_pulse_status_t nd_pulse_store_commit(nd_pulse_store_t *store,
                                        const uint8_t *blob, size_t length);

#endif
