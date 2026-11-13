#ifndef ND_IDIS_PROFILES_H
#define ND_IDIS_PROFILES_H

#include <stdint.h>

#include "targets/idis_ip_meter/cosem/nd_idis_profile.h"

#define ND_IDIS_PROFILE_MAX 22

typedef struct {
    nd_obis_t ln;
    nd_store_t *store;
    uint32_t base_offset;
    uint16_t capacity;
    uint16_t slot_size;
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
} nd_idis_profiles_t;

int nd_idis_profiles_init(nd_idis_profiles_t *profiles,
                          const nd_idis_profile_config_t *configs,
                          uint8_t count);

void nd_idis_profiles_bind_port(nd_idis_profiles_t *profiles,
                                const nd_cosem_port_t *port);

nd_idis_profile_state_t *nd_idis_profiles_find(nd_idis_profiles_t *profiles,
                                               const nd_obis_t *ln);

int nd_idis_profiles_on_second(nd_idis_profiles_t *profiles,
                               const nd_cosem_port_t *port, uint8_t client_sap,
                               const nd_clock_value_t *now);

/* Flushes the profile metadata of every initialized profile. */
void nd_idis_profiles_sync(nd_idis_profiles_t *profiles);

#endif
