#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"

#include <string.h>

#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define RELEASE_MARKER_ACTIVE 0x52454C31u /* "REL1" */
#define RELEASE_MARKER_RECORD 8u
#define RELEASE_MARKER_SLOTS \
    (ND_IDIS_RELEASE_MARKER_BYTES / RELEASE_MARKER_RECORD)
#define RELEASE_MARKER_BLANK 0xFFFFFFFFu

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static uint32_t get_u32(const uint8_t *in)
{
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16)
        | ((uint32_t)in[2] << 8) | in[3];
}

static void marker_feed(const nd_profile_watchdog_t *watchdog)
{
    if (watchdog->feed != NULL) {
        watchdog->feed(watchdog->ctx);
    }
}

static uint32_t marker_slot_offset(const nd_idis_profiles_t *profiles,
                                   uint16_t slot)
{
    return profiles->release_offset
         + (uint32_t)slot * RELEASE_MARKER_RECORD;
}

/* The marker sector is append-only while an ACTIVE record exists, so a failed
 * release is never erased before its retry. */
static int release_marker_scan(const nd_idis_profiles_t *profiles, int *active,
                               uint16_t *blank)
{
    uint8_t record[RELEASE_MARKER_RECORD];
    *active = 0;
    *blank = RELEASE_MARKER_SLOTS;
    for (uint16_t slot = 0; slot < RELEASE_MARKER_SLOTS; slot++) {
        if (nd_store_read(profiles->release_store,
                          marker_slot_offset(profiles, slot), record,
                          sizeof(record)) != 0) {
            return -1;
        }
        marker_feed(&profiles->watchdog);
        uint32_t first = get_u32(&record[0]);
        uint32_t second = get_u32(&record[4]);
        if (first == RELEASE_MARKER_ACTIVE
            && second == (uint32_t)~RELEASE_MARKER_ACTIVE) {
            *active = 1;
        } else if (first == RELEASE_MARKER_BLANK
                   && second == RELEASE_MARKER_BLANK
                   && *blank == RELEASE_MARKER_SLOTS) {
            *blank = slot;
        }
    }
    return 0;
}

int nd_idis_profiles_marker_begin(nd_idis_profiles_t *profiles,
                                  const nd_idis_release_marker_t *marker,
                                  const nd_profile_watchdog_t *watchdog)
{
    if (profiles == NULL) {
        return -1;
    }
    profiles->release_store = NULL;
    profiles->release_offset = 0;
    profiles->watchdog = (nd_profile_watchdog_t){0};
    if (watchdog != NULL) {
        profiles->watchdog = *watchdog;
    }
    if (marker == NULL || marker->store == NULL) {
        return 0;
    }
    if (marker->offset % ND_IDIS_RELEASE_MARKER_BYTES != 0) {
        return -1;
    }
    profiles->release_store = marker->store;
    profiles->release_offset = marker->offset;
    int active = 0;
    uint16_t blank = RELEASE_MARKER_SLOTS;
    if (release_marker_scan(profiles, &active, &blank) != 0) {
        return -1;
    }
    return active;
}

static int release_marker_activate(nd_idis_profiles_t *profiles)
{
    if (profiles->release_store == NULL) {
        return 0;
    }
    int active = 0;
    uint16_t blank = RELEASE_MARKER_SLOTS;
    if (release_marker_scan(profiles, &active, &blank) != 0) {
        return -1;
    }
    uint16_t slot = 0;
    if (active) {
        if (blank >= RELEASE_MARKER_SLOTS) {
            return -1;
        }
        slot = blank;
    } else if (nd_store_erase(profiles->release_store,
                              profiles->release_offset,
                              ND_IDIS_RELEASE_MARKER_BYTES) != 0) {
        return -1;
    } else {
        marker_feed(&profiles->watchdog);
    }
    uint8_t record[RELEASE_MARKER_RECORD];
    put_u32(&record[0], RELEASE_MARKER_ACTIVE);
    put_u32(&record[4], (uint32_t)~RELEASE_MARKER_ACTIVE);
    uint32_t offset = marker_slot_offset(profiles, slot);
    if (nd_store_write(profiles->release_store, offset, record,
                       sizeof(record)) != 0) {
        return -1;
    }
    marker_feed(&profiles->watchdog);
    uint8_t check[RELEASE_MARKER_RECORD];
    if (nd_store_read(profiles->release_store, offset, check,
                      sizeof(check)) != 0) {
        return -1;
    }
    marker_feed(&profiles->watchdog);
    return memcmp(check, record, sizeof(check)) == 0 ? 0 : -1;
}

static int release_marker_clear(nd_idis_profiles_t *profiles)
{
    if (profiles->release_store == NULL) {
        return 0;
    }
    if (nd_store_erase(profiles->release_store, profiles->release_offset,
                       ND_IDIS_RELEASE_MARKER_BYTES) != 0) {
        return -1;
    }
    marker_feed(&profiles->watchdog);
    int active = 0;
    uint16_t blank = RELEASE_MARKER_SLOTS;
    if (release_marker_scan(profiles, &active, &blank) != 0) {
        return -1;
    }
    return active ? -1 : 0;
}

static void hold_every_store(nd_idis_profiles_t *profiles)
{
    for (uint8_t index = 0; index < profiles->count; index++) {
        nd_profile_store_hold(&profiles->items[index].store);
    }
}

int nd_idis_profiles_init(nd_idis_profiles_t *profiles,
                          const nd_idis_profile_config_t *configs,
                          uint8_t count, uint8_t reset_legacy,
                          const nd_idis_release_marker_t *marker)
{
    if (profiles == NULL || configs == NULL || count > ND_IDIS_PROFILE_MAX) {
        return -1;
    }
    memset(profiles, 0, sizeof(*profiles));
    int release_pending = nd_idis_profiles_marker_begin(
        profiles, marker,
        count > 0 ? configs[0].store_opts.watchdog : NULL);
    if (release_pending < 0) {
        return -1;
    }
    uint8_t legacy = 0;
    for (uint8_t index = 0; index < count; index++) {
        const nd_idis_profile_config_t *config = &configs[index];
        int status = nd_idis_profile_init(
            &profiles->items[index], config->store, config->base_offset,
            config->capacity, config->slot_size, config->sector_size,
            &config->store_opts, &config->ln, config->clock, config->captures,
            config->capture_count, config->capture_period, config->compressed,
            config->delta);
        if (status == ND_PROFILE_STORE_LEGACY_IMAGE) {
            if (reset_legacy == 0 || release_pending) {
                legacy = 1;
            } else if (nd_profile_store_reset(
                           &profiles->items[index].store) != 0) {
                return -1;
            }
            continue;
        }
        if (status != 0) {
            return -1;
        }
    }
    profiles->count = count;
    if (legacy != 0 || release_pending) {
        hold_every_store(profiles);
    }
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

int nd_idis_profiles_legacy_held(const nd_idis_profiles_t *profiles)
{
    if (profiles == NULL) {
        return 0;
    }
    for (uint8_t index = 0; index < profiles->count; index++) {
        if (profiles->items[index].store.legacy != 0) {
            return 1;
        }
    }
    return 0;
}

int nd_idis_profiles_release_legacy(nd_idis_profiles_t *profiles)
{
    if (profiles == NULL) {
        return -1;
    }
    if (!nd_idis_profiles_legacy_held(profiles)) {
        return 0;
    }
    if (release_marker_activate(profiles) != 0) {
        hold_every_store(profiles);
        return -1;
    }
    for (uint8_t index = 0; index < profiles->count; index++) {
        if (nd_profile_store_reset(&profiles->items[index].store) != 0) {
            hold_every_store(profiles);
            return -1;
        }
        /* Each ring erase can take the datasheet maximum, so the 22-ring
         * loop feeds like the boot scan instead of relying on on_second. */
        marker_feed(&profiles->watchdog);
    }
    if (release_marker_clear(profiles) != 0) {
        hold_every_store(profiles);
        return -1;
    }
    return 0;
}

int nd_idis_profiles_reset(nd_idis_profiles_t *profiles,
                           nd_idis_profile_state_t *profile)
{
    if (profiles == NULL || profile == NULL
        || nd_idis_profiles_legacy_held(profiles)) {
        return -1;
    }
    return nd_profile_store_reset(&profile->store);
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
