#ifndef ND_IDIS_PROFILE_TABLE_H
#define ND_IDIS_PROFILE_TABLE_H

#include <stdint.h>

#include "domain/common/store.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_clock_port.h"
#include "targets/idis_ip_meter_freertos/cosem/nd_idis_profiles.h"

#define ND_IDIS_PROFILE_TABLE_COUNT 22

_Static_assert(ND_IDIS_PROFILE_TABLE_COUNT <= ND_IDIS_PROFILE_MAX,
               "profile table must fit the profiles aggregate");

/* Each profile owns whole sectors: capacity slots plus one spare sector the
 * ring erases before it reuses the sector. */
#define PROFILE_SECTOR_BYTES 4096u
#define PROFILE_SLOT_BYTES 128u
#define PROFILE_SECTOR_SLOTS (PROFILE_SECTOR_BYTES / PROFILE_SLOT_BYTES)
#define PROFILE_REGION_BYTES(cap_slots)                                      \
    ((((cap_slots) + PROFILE_SECTOR_SLOTS - 1u) / PROFILE_SECTOR_SLOTS + 1u) \
     * PROFILE_SECTOR_BYTES)

/* The largest region ends below 640 KiB. */
#define ND_IDIS_PROFILE_FLASH_BYTES (640u * 1024u)

_Static_assert(ND_IDIS_RELEASE_MARKER_OFFSET % ND_IDIS_RELEASE_MARKER_BYTES
                   == 0u,
               "release marker must be sector aligned");
_Static_assert(ND_IDIS_RELEASE_MARKER_OFFSET + ND_IDIS_RELEASE_MARKER_BYTES
                   <= ND_IDIS_PROFILE_FLASH_BYTES,
               "release marker must fit the profile flash budget");

/*
 * Fills one config per Pack3 profile in IDIS object order. Every profile uses
 * the same flash store and clock; the caller passes both once. Each config
 * carries the frozen pre-v4 base so init can find an image the v4 layout move
 * left behind. Returns the number of configs written, or -1 when max is too
 * small.
 */
int nd_idis_profile_table_build(nd_idis_profile_config_t *configs, uint8_t max,
                                nd_store_t *flash,
                                const nd_clock_port_t *clock,
                                const nd_profile_watchdog_t *watchdog);

/*
 * Initializes all 22 profile states without a temporary config array.
 * reset_legacy chooses the boot handling of a detected pre-v4 image; see
 * nd_idis_profiles_init. Returns the profile count, or -1 on a device error.
 */
int nd_idis_profile_table_init(nd_idis_profiles_t *profiles, nd_store_t *flash,
                               const nd_clock_port_t *clock,
                               const nd_profile_watchdog_t *watchdog,
                               uint8_t reset_legacy);

/*
 * Boot gate around nd_idis_profile_table_init with reset_legacy == 0.
 * Returns 0 when every profile registered, -1 on a device error. The product
 * boot must stop on -1: running on marks every class-7 object undefined and
 * leaves both event logs empty, while the watchdog restarts the scan.
 */
int nd_idis_profile_table_boot(nd_idis_profiles_t *profiles, nd_store_t *flash,
                               const nd_clock_port_t *clock,
                               const nd_profile_watchdog_t *watchdog);

#endif
