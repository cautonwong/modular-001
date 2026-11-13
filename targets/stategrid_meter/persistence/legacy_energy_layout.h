#ifndef ND_TARGET_STATEGRID_LEGACY_ENERGY_LAYOUT_H
#define ND_TARGET_STATEGRID_LEGACY_ENERGY_LAYOUT_H

#include <stdint.h>

#include "targets/stategrid_meter/persistence/legacy_energy_record.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

/*
 * Byte spans used by the legacy energy backup and restore code. All layout
 * comes from the generated meter_record.h; this header only names the spans.
 */

/* active_p from the start of the persistent section. */
static inline uint32_t legacy_energy_persistent_offset(void)
{
    return nd_mbase_offset_active_p() -
           nd_mbase_offset_persistent_section_start();
}

/* ec_save_success from the start of the persistent section. */
static inline uint32_t legacy_energy_ec_offset(void)
{
    return nd_mbase_offset_ec_save_success() -
           nd_mbase_offset_persistent_section_start();
}

/* active_p through the energy EC label. */
static inline uint16_t legacy_energy_block_span(void)
{
    return (uint16_t)(nd_mbase_offset_persistent_energy_EC_label() -
                      nd_mbase_offset_active_p());
}

/* active_p through the energy checksum. */
static inline uint16_t legacy_energy_checksum_span(void)
{
    return (uint16_t)(nd_mbase_offset_Energy_chksum() -
                      nd_mbase_offset_active_p());
}

/* MonthEnegery_dec through the use-energy checksum. */
static inline uint16_t legacy_energy_use_checksum_span(void)
{
    return (uint16_t)(nd_mbase_offset_UseEnergy_chksum() -
                      nd_mbase_offset_MonthEnegery_dec());
}

static inline uint16_t legacy_energy_checksum_size(void)
{
    return (uint16_t)nd_mbase_size_Energy_chksum();
}

static inline uint16_t legacy_energy_use_checksum_size(void)
{
    return (uint16_t)nd_mbase_size_UseEnergy_chksum();
}

static inline uint16_t legacy_energy_ec_size(void)
{
    return (uint16_t)nd_mbase_size_ec_save_success();
}

_Static_assert(nd_mbase_offset_active_p() -
                       nd_mbase_offset_persistent_section_start() ==
                   ND_LEGACY_ENERGY_OFFSET_IN_BLOCK,
               "energy record offset changed");

#endif
