#ifndef ND_IDIS_PROFILES_H
#define ND_IDIS_PROFILES_H

#include <stdint.h>

#include "targets/idis_ip_meter/cosem/nd_idis_profile.h"

#define ND_IDIS_PROFILE_MAX 22

/* ASCII "RELG". Operator confirmation for the destructive legacy release.
 * Class-7 method 1 has no parameters, so a front end cannot carry this value
 * in the ACTION; the local factory entry verifies it instead. */
#define ND_IDIS_LEGACY_RELEASE_CONFIRMATION 0x52454C47u

/* The release resets 22 rings. A failure after the first reset leaves reset
 * marks that switch the legacy probe off, so a restart can no longer detect
 * the retained image. Persist an in-release marker before the first reset and
 * erase it only after the last: any ACTIVE record forces every store to stay
 * held across a reboot. The marker owns one 4 KiB sector at the end of the
 * 640 KiB profile flash budget; the last profile region ends before it. */
#define ND_IDIS_RELEASE_MARKER_OFFSET 0x9F000u
#define ND_IDIS_RELEASE_MARKER_BYTES 4096u

typedef struct {
    /* NULL disables persistence for callers with a store smaller than the
     * marker offset (custom unit fixtures). */
    nd_store_t *store;
    uint32_t offset;
} nd_idis_release_marker_t;

typedef struct {
    nd_obis_t ln;
    nd_store_t *store;
    uint32_t base_offset;
    uint16_t capacity;
    uint16_t slot_size;
    uint16_t sector_size;
    nd_profile_store_opts_t store_opts;
    const nd_clock_port_t *clock;
    nd_capture_object_t captures[ND_IDIS_PROFILE_MAX_CAPTURES];
    uint8_t capture_count;
    uint32_t capture_period;
    uint8_t compressed;
    uint8_t delta;
} nd_idis_profile_config_t;

typedef struct {
    nd_idis_profile_state_t items[ND_IDIS_PROFILE_MAX];
    uint8_t count;
    nd_store_t *release_store; /* NULL = no persisted release marker */
    uint32_t release_offset;
    nd_profile_watchdog_t watchdog; /* feeds the marker scan and I/O */
} nd_idis_profiles_t;

/* reset_legacy: 0 retains a pre-v4 image and holds every profile store so no
 * v4 ring can overwrite it; nonzero runs nd_profile_store_reset and continues.
 * Either way all profiles and event logs are registered. An ACTIVE marker
 * always wins: it holds every store and suppresses the automatic reset, so a
 * boot policy never completes an interrupted operator-confirmed release. */
int nd_idis_profiles_init(nd_idis_profiles_t *profiles,
                          const nd_idis_profile_config_t *configs,
                          uint8_t count, uint8_t reset_legacy,
                          const nd_idis_release_marker_t *marker);

/* Reads the persisted release marker and binds it to the aggregate. Returns 1
 * while a release is in progress, 0 when not, -1 on a device error. */
int nd_idis_profiles_marker_begin(nd_idis_profiles_t *profiles,
                                  const nd_idis_release_marker_t *marker,
                                  const nd_profile_watchdog_t *watchdog);

void nd_idis_profiles_bind_port(nd_idis_profiles_t *profiles,
                                const nd_cosem_port_t *port);

nd_idis_profile_state_t *nd_idis_profiles_find(nd_idis_profiles_t *profiles,
                                               const nd_obis_t *ln);

/* 1 while any store retains a pre-v4 image. The boot composition reports it
 * so an operator can see why no profile grows. */
int nd_idis_profiles_legacy_held(const nd_idis_profiles_t *profiles);

/* Maintenance release: resets every profile ring, discarding its records, and
 * clears every hold. No-op when nothing is held. The persisted marker is
 * activated before the first ring reset and erased only after the last; a
 * device error re-holds every store so a partially released ring cannot erase
 * a still-held image, and a restart re-holds until the operator retries. */
int nd_idis_profiles_release_legacy(nd_idis_profiles_t *profiles);

/* Reset one profile's ring. Refused while any store is held: releasing one
 * store lets its v4 ring erase a neighbouring region that still holds a
 * pre-v4 image (BILLING covers the old LOAD_PROFILE_1 base). Release the
 * holds through nd_idis_profiles_release_legacy, which discards every one. */
int nd_idis_profiles_reset(nd_idis_profiles_t *profiles,
                           nd_idis_profile_state_t *profile);

int nd_idis_profiles_on_second(nd_idis_profiles_t *profiles,
                               const nd_cosem_port_t *port, uint8_t client_sap,
                               const nd_clock_value_t *now);

#endif
