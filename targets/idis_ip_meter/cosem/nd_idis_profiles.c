#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

int nd_idis_profiles_init(nd_idis_profiles_t *profiles,
                          const nd_idis_profile_config_t *configs,
                          uint8_t count)
{
    if (profiles == NULL || configs == NULL || count > ND_IDIS_PROFILE_MAX) {
        return -1;
    }
    memset(profiles, 0, sizeof(*profiles));
    for (uint8_t index = 0; index < count; index++) {
        const nd_idis_profile_config_t *config = &configs[index];
        if (nd_idis_profile_init(&profiles->items[index], config->store,
                                 config->base_offset, config->capacity,
                                 config->slot_size, &config->ln, config->clock,
                                 config->captures, config->capture_count,
                                 config->capture_period, config->compressed,
                                 config->delta) != 0) {
            return -1;
        }
    }
    profiles->count = count;
    return 0;
}

void nd_idis_profiles_bind_port(nd_idis_profiles_t *profiles,
                                const nd_cosem_port_t *port)
{
    if (profiles == NULL) {
        return;
    }
    for (uint8_t index = 0; index < profiles->count; index++) {
        profiles->items[index].port = port;
    }
}

nd_idis_profile_state_t *nd_idis_profiles_find(nd_idis_profiles_t *profiles,
                                               const nd_obis_t *ln)
{
    if (profiles == NULL || ln == NULL) {
        return NULL;
    }
    for (uint8_t index = 0; index < profiles->count; index++) {
        if (nd_obis_eq(ln, &profiles->items[index].ln)) {
            return &profiles->items[index];
        }
    }
    return NULL;
}

int nd_idis_profiles_on_second(nd_idis_profiles_t *profiles,
                               const nd_cosem_port_t *port, uint8_t client_sap,
                               const nd_clock_value_t *now)
{
    if (profiles == NULL || port == NULL || now == NULL) {
        return -1;
    }
    int captured = 0;
    for (uint8_t index = 0; index < profiles->count; index++) {
        int result = nd_idis_profile_on_second(&profiles->items[index], port,
                                               client_sap, now);
        if (result < 0) {
            return -1;
        }
        captured += result;
    }
    return captured;
}

void nd_idis_profiles_sync(nd_idis_profiles_t *profiles)
{
    if (profiles == NULL) {
        return;
    }
    for (uint8_t index = 0; index < profiles->count; index++) {
        (void)nd_profile_store_sync(&profiles->items[index].store);
    }
}
