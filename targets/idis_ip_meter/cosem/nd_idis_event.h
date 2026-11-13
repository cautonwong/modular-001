#ifndef ND_IDIS_EVENT_H
#define ND_IDIS_EVENT_H

#include <stdint.h>

#include "domain/common/profile_status.h"
#include "domain/events/event_log.h"
#include "targets/idis_ip_meter/cosem/nd_alarm_state.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profile.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"

#define ND_IDIS_EVENT_MAGIC 0x45564E32u

/* Log profiles a raised event can be captured into. */
typedef enum {
    ND_IDIS_EVENT_LOG_STANDARD = 0,
    ND_IDIS_EVENT_LOG_FRAUD,
    ND_IDIS_EVENT_LOG_DISCONNECTOR,
    ND_IDIS_EVENT_LOG_MBUS,
    ND_IDIS_EVENT_LOG_PQ,
    ND_IDIS_EVENT_LOG_COMMUNICATION,
    ND_IDIS_EVENT_LOG_POWER_FAILURE,
    ND_IDIS_EVENT_LOG_CATEGORY_COUNT
} nd_idis_event_log_category_t;

typedef enum {
    ND_IDIS_COMMUNICATION_REMOTE = 0,
    ND_IDIS_COMMUNICATION_LOCAL
} nd_idis_communication_interface_t;

typedef struct {
    uint32_t magic;
    uint8_t latest_event_code;
    uint8_t profile_status;
    uint8_t log_count;
    uint8_t log_head;
    uint32_t counters[12];
    uint32_t registers[2];
    uint32_t filters[2];
    uint32_t descriptors[2];
    uint32_t error_register;
    nd_event_record_t log[ND_EVENT_LOG_MAX];
    uint16_t log_codes[ND_EVENT_LOG_MAX];
} nd_idis_event_record_t;

typedef struct {
    nd_alarm_state_t alarm;
    nd_event_log_t log;
    uint16_t log_codes[ND_EVENT_LOG_MAX];
    uint8_t profile_status;
    uint8_t latest_event_code;
    uint32_t counters[12];
    uint32_t descriptors[2];
    uint32_t error_register;
    nd_idis_profile_state_t *log_profile;
    nd_idis_profile_state_t
        *category_logs[ND_IDIS_EVENT_LOG_CATEGORY_COUNT];
    nd_idis_profiles_t *profiles;
    const nd_cosem_port_t *port;
    const nd_clock_port_t *clock;
    uint8_t client_sap;
} nd_idis_event_state_t;

void nd_idis_event_init(nd_idis_event_state_t *state,
                        const nd_idis_event_rule_t *rules, uint16_t count);

void nd_idis_event_bind_log(nd_idis_event_state_t *state,
                            nd_idis_profile_state_t *log_profile,
                            const nd_cosem_port_t *port,
                            const nd_clock_port_t *clock, uint8_t client_sap);
void nd_idis_event_bind_category_log(nd_idis_event_state_t *state,
                                     nd_idis_event_log_category_t category,
                                     nd_idis_profile_state_t *log_profile);
void nd_idis_event_bind_profiles(nd_idis_event_state_t *state,
                                 nd_idis_profiles_t *profiles);

/* Category of a raised record, used to select the matching log profile. */
nd_idis_event_log_category_t
nd_idis_event_category_for_kind(nd_event_kind_t kind);

/* Latest event code in a category, or 255 when the category has no events. */
uint8_t nd_idis_event_latest_code(const nd_idis_event_state_t *state,
                                  nd_idis_event_log_category_t category);

int nd_idis_event_raise(nd_idis_event_state_t *state, uint16_t event_code);
int nd_idis_event_raise_ex(nd_idis_event_state_t *state, uint16_t event_code,
                           uint8_t source, uint8_t severity,
                           uint32_t duration_seconds, int32_t value);
int nd_idis_event_raise_kind(nd_idis_event_state_t *state,
                             nd_event_kind_t kind, uint16_t event_code,
                             uint8_t source, uint8_t severity,
                             uint32_t duration_seconds, int32_t value);

/* 26/27 remote interface start/end, 28/29 local interface start/end. */
void nd_idis_event_communication(nd_idis_event_state_t *state,
                                 nd_idis_communication_interface_t interface,
                                 int start);

int nd_idis_event_save(const nd_idis_event_state_t *state, nd_store_t *store,
                       uint32_t offset);

int nd_idis_event_load(nd_idis_event_state_t *state, nd_store_t *store,
                       uint32_t offset);

#endif
