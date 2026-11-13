#ifndef ND_IDIS_PROFILE_TABLE_H
#define ND_IDIS_PROFILE_TABLE_H

#include <stdint.h>

#include "domain/common/store.h"
#include "targets/idis_ip_meter/cosem/nd_clock_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"

#define ND_IDIS_PROFILE_TABLE_COUNT 22

/* Each profile owns a fixed flash region; the largest ends below 640 KiB. */
#define ND_IDIS_PROFILE_FLASH_BYTES (640u * 1024u)

/*
 * Fills one config per Pack3 profile in IDIS object order. Every profile uses
 * the same flash store and clock; the caller passes both once.
 * Returns the number of configs written, or -1 when max is too small.
 */
int nd_idis_profile_table_build(nd_idis_profile_config_t *configs, uint8_t max,
                                nd_store_t *flash,
                                const nd_clock_port_t *clock);

/* Initializes all 22 profile states without a temporary config array. */
int nd_idis_profile_table_init(nd_idis_profiles_t *profiles, nd_store_t *flash,
                               const nd_clock_port_t *clock);

#endif
