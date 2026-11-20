#include "targets/idis_ip_meter_freertos/cosem/nd_idis_profile_table.h"

#include <string.h>

#include "targets/idis_ip_meter_freertos/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter_freertos/mapper_idis/nd_idis_ids.h"

/* Flash regions. Each profile owns whole 4 KiB sectors and packs against the
 * next; the spare sector in every region sits before its end. */
#define CAP_BILLING 13u
#define CAP_LOAD_PROFILE_1 960u
#define CAP_LOAD_PROFILE_2 240u
#define CAP_PQ_PROFILE 1152u
#define CAP_STANDARD_EVENT_LOG 100u
#define CAP_FRAUD_LOG 30u
#define CAP_DISCONNECTOR_LOG 10u
#define CAP_MBUS_EVENT_LOG 10u
#define CAP_PQ_LOG 100u
#define CAP_COMM_LOG 100u
#define CAP_COMM_DETAILS_LOG 100u
#define CAP_POWER_FAILURE_LOG 10u
#define CAP_MBUS_LOAD_PROFILE_1 240u
#define CAP_MBUS_LOAD_PROFILE_2 240u
#define CAP_MBUS_LOAD_PROFILE_3 240u
#define CAP_MBUS_LOAD_PROFILE_4 240u
#define CAP_MBUS_CONTROL_LOG_1 10u
#define CAP_MBUS_CONTROL_LOG_2 10u
#define CAP_MBUS_CONTROL_LOG_3 10u
#define CAP_MBUS_CONTROL_LOG_4 10u
#define CAP_DISPLAY_GENERAL 10u
#define CAP_DISPLAY_ALTERNATE 10u

#define FLASH_BILLING 0u
#define FLASH_LP1 (FLASH_BILLING + PROFILE_REGION_BYTES(CAP_BILLING))
#define FLASH_LP2 (FLASH_LP1 + PROFILE_REGION_BYTES(CAP_LOAD_PROFILE_1))
#define FLASH_PQ (FLASH_LP2 + PROFILE_REGION_BYTES(CAP_LOAD_PROFILE_2))
#define FLASH_EVENT_STANDARD \
    (FLASH_PQ + PROFILE_REGION_BYTES(CAP_PQ_PROFILE))
#define FLASH_EVENT_FRAUD \
    (FLASH_EVENT_STANDARD + PROFILE_REGION_BYTES(CAP_STANDARD_EVENT_LOG))
#define FLASH_EVENT_DISCONNECTOR \
    (FLASH_EVENT_FRAUD + PROFILE_REGION_BYTES(CAP_FRAUD_LOG))
#define FLASH_EVENT_MBUS \
    (FLASH_EVENT_DISCONNECTOR + PROFILE_REGION_BYTES(CAP_DISCONNECTOR_LOG))
#define FLASH_EVENT_PQ \
    (FLASH_EVENT_MBUS + PROFILE_REGION_BYTES(CAP_MBUS_EVENT_LOG))
#define FLASH_EVENT_COMM (FLASH_EVENT_PQ + PROFILE_REGION_BYTES(CAP_PQ_LOG))
#define FLASH_EVENT_COMM_DETAILS \
    (FLASH_EVENT_COMM + PROFILE_REGION_BYTES(CAP_COMM_LOG))
#define FLASH_POWER_FAILURE \
    (FLASH_EVENT_COMM_DETAILS + PROFILE_REGION_BYTES(CAP_COMM_DETAILS_LOG))
#define FLASH_MBUS_LOAD_1 \
    (FLASH_POWER_FAILURE + PROFILE_REGION_BYTES(CAP_POWER_FAILURE_LOG))
#define FLASH_MBUS_LOAD_2 \
    (FLASH_MBUS_LOAD_1 + PROFILE_REGION_BYTES(CAP_MBUS_LOAD_PROFILE_1))
#define FLASH_MBUS_LOAD_3 \
    (FLASH_MBUS_LOAD_2 + PROFILE_REGION_BYTES(CAP_MBUS_LOAD_PROFILE_2))
#define FLASH_MBUS_LOAD_4 \
    (FLASH_MBUS_LOAD_3 + PROFILE_REGION_BYTES(CAP_MBUS_LOAD_PROFILE_3))
#define FLASH_MBUS_CONTROL_1 \
    (FLASH_MBUS_LOAD_4 + PROFILE_REGION_BYTES(CAP_MBUS_LOAD_PROFILE_4))
#define FLASH_MBUS_CONTROL_2 \
    (FLASH_MBUS_CONTROL_1 + PROFILE_REGION_BYTES(CAP_MBUS_CONTROL_LOG_1))
#define FLASH_MBUS_CONTROL_3 \
    (FLASH_MBUS_CONTROL_2 + PROFILE_REGION_BYTES(CAP_MBUS_CONTROL_LOG_2))
#define FLASH_MBUS_CONTROL_4 \
    (FLASH_MBUS_CONTROL_3 + PROFILE_REGION_BYTES(CAP_MBUS_CONTROL_LOG_3))
#define FLASH_DISPLAY_GENERAL \
    (FLASH_MBUS_CONTROL_4 + PROFILE_REGION_BYTES(CAP_MBUS_CONTROL_LOG_4))
#define FLASH_DISPLAY_ALTERNATE \
    (FLASH_DISPLAY_GENERAL + PROFILE_REGION_BYTES(CAP_DISPLAY_GENERAL))

_Static_assert(PROFILE_SLOT_BYTES == 128u,
               "the region macro counts 128-byte slots");
_Static_assert(FLASH_DISPLAY_ALTERNATE
                       + PROFILE_REGION_BYTES(CAP_DISPLAY_ALTERNATE)
                   <= ND_IDIS_RELEASE_MARKER_OFFSET,
               "the release marker sector must follow the last region");
_Static_assert(ND_IDIS_RELEASE_MARKER_OFFSET
                       + ND_IDIS_RELEASE_MARKER_BYTES
                   <= ND_IDIS_PROFILE_FLASH_BYTES,
               "profile regions and marker must fit the 640 KiB budget");

#define CAP(cls, ln, attr) {cls, ln, attr, 0}

typedef struct {
    nd_obis_t ln;
    uint32_t base_offset;
    uint16_t capacity;
    uint16_t slot_size;
    uint32_t capture_period;
    uint8_t compressed;
    uint8_t delta;
    uint8_t capture_count;
    uint16_t capture_first;
} nd_idis_profile_desc_t;

static const nd_capture_object_t capture_pool[96] = {
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_REACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_REACTIVE_ENERGY_EXPORT, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_2, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_PQ, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_AVERAGE_VOLTAGE_L1, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_AVERAGE_VOLTAGE_L2, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_AVERAGE_VOLTAGE_L3, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_AVERAGE_CURRENT_L1, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_AVERAGE_CURRENT_L2, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_AVERAGE_CURRENT_L3, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_REACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_REACTIVE_ENERGY_EXPORT, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_STANDARD, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_FRAUD, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_DISCONNECTOR, 2),
    CAP(ND_IC_LIMITER, ND_OBIS_LIMITER, 3),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_PQ, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_COMM, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_COMM_DETAILS, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_POWER_FAILURE_DURATION, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_MBUS_1, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_1, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_2, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_3, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_4, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_5, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_6, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_7, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_1_8, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_MBUS_2, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_1, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_2, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_3, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_4, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_5, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_6, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_7, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_2_8, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_MBUS_3, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_1, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_2, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_3, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_4, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_5, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_6, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_7, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_3_8, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_MBUS_4, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_1, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_2, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_3, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_4, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_5, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_6, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_7, 2),
    CAP(ND_IC_EXTENDED_REGISTER, ND_OBIS_MBUS_VALUE_4_8, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1, 2),
    CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
    CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
    CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1, 2),
};

static const nd_idis_profile_desc_t table[ND_IDIS_PROFILE_TABLE_COUNT] = {
    {
        .ln = ND_OBIS_BILLING_PROFILE,
        .base_offset = FLASH_BILLING,
        .capacity = CAP_BILLING,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .compressed = 1,
        .capture_count = 5,
        .capture_first = 0,
    },
    {
        .ln = ND_OBIS_LOAD_PROFILE_1,
        .base_offset = FLASH_LP1,
        .capacity = CAP_LOAD_PROFILE_1,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 900,
        .compressed = 1,
        .delta = 1,
        .capture_count = 4,
        .capture_first = 5,
    },
    {
        .ln = ND_OBIS_LOAD_PROFILE_2,
        .base_offset = FLASH_LP2,
        .capacity = CAP_LOAD_PROFILE_2,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 86400,
        .compressed = 1,
        .delta = 1,
        .capture_count = 4,
        .capture_first = 9,
    },
    {
        .ln = ND_OBIS_PQ_PROFILE,
        .base_offset = FLASH_PQ,
        .capacity = CAP_PQ_PROFILE,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 60,
        .compressed = 1,
        .delta = 1,
        .capture_count = 12,
        .capture_first = 13,
    },
    {
        .ln = ND_OBIS_STANDARD_EVENT_LOG,
        .base_offset = FLASH_EVENT_STANDARD,
        .capacity = CAP_STANDARD_EVENT_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 25,
    },
    {
        .ln = ND_OBIS_FRAUD_LOG,
        .base_offset = FLASH_EVENT_FRAUD,
        .capacity = CAP_FRAUD_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 27,
    },
    {
        .ln = ND_OBIS_DISCONNECTOR_LOG,
        .base_offset = FLASH_EVENT_DISCONNECTOR,
        .capacity = CAP_DISCONNECTOR_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 3,
        .capture_first = 29,
    },
    {
        .ln = ND_OBIS_MBUS_EVENT_LOG,
        .base_offset = FLASH_EVENT_MBUS,
        .capacity = CAP_MBUS_EVENT_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 32,
    },
    {
        .ln = ND_OBIS_PQ_LOG,
        .base_offset = FLASH_EVENT_PQ,
        .capacity = CAP_PQ_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 34,
    },
    {
        .ln = ND_OBIS_COMM_LOG,
        .base_offset = FLASH_EVENT_COMM,
        .capacity = CAP_COMM_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 36,
    },
    {
        .ln = ND_OBIS_COMM_DETAILS_LOG,
        .base_offset = FLASH_EVENT_COMM_DETAILS,
        .capacity = CAP_COMM_DETAILS_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 38,
    },
    {
        .ln = ND_OBIS_POWER_FAILURE_LOG,
        .base_offset = FLASH_POWER_FAILURE,
        .capacity = CAP_POWER_FAILURE_LOG,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 0,
        .capture_count = 2,
        .capture_first = 40,
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_1,
        .base_offset = FLASH_MBUS_LOAD_1,
        .capacity = CAP_MBUS_LOAD_PROFILE_1,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 3600,
        .capture_count = 10,
        .capture_first = 42,
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_2,
        .base_offset = FLASH_MBUS_LOAD_2,
        .capacity = CAP_MBUS_LOAD_PROFILE_2,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 3600,
        .capture_count = 10,
        .capture_first = 52,
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_3,
        .base_offset = FLASH_MBUS_LOAD_3,
        .capacity = CAP_MBUS_LOAD_PROFILE_3,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 3600,
        .capture_count = 10,
        .capture_first = 62,
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_4,
        .base_offset = FLASH_MBUS_LOAD_4,
        .capacity = CAP_MBUS_LOAD_PROFILE_4,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_period = 3600,
        .capture_count = 10,
        .capture_first = 72,
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_1,
        .base_offset = FLASH_MBUS_CONTROL_1,
        .capacity = CAP_MBUS_CONTROL_LOG_1,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_count = 2,
        .capture_first = 82,
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_2,
        .base_offset = FLASH_MBUS_CONTROL_2,
        .capacity = CAP_MBUS_CONTROL_LOG_2,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_count = 2,
        .capture_first = 84,
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_3,
        .base_offset = FLASH_MBUS_CONTROL_3,
        .capacity = CAP_MBUS_CONTROL_LOG_3,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_count = 2,
        .capture_first = 86,
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_4,
        .base_offset = FLASH_MBUS_CONTROL_4,
        .capacity = CAP_MBUS_CONTROL_LOG_4,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_count = 2,
        .capture_first = 88,
    },
    {
        .ln = ND_OBIS_DISPLAY_GENERAL,
        .base_offset = FLASH_DISPLAY_GENERAL,
        .capacity = CAP_DISPLAY_GENERAL,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_count = 3,
        .capture_first = 90,
    },
    {
        .ln = ND_OBIS_DISPLAY_ALTERNATE,
        .base_offset = FLASH_DISPLAY_ALTERNATE,
        .capacity = CAP_DISPLAY_ALTERNATE,
        .slot_size = PROFILE_SLOT_BYTES,
        .capture_count = 3,
        .capture_first = 93,
    },
};

static int desc_is_valid(const nd_idis_profile_desc_t *desc)
{
    size_t pool_size = sizeof(capture_pool) / sizeof(capture_pool[0]);

    return desc->capture_count <= ND_IDIS_PROFILE_MAX_CAPTURES
           && (size_t)desc->capture_first + desc->capture_count <= pool_size
           && desc->slot_size == PROFILE_SLOT_BYTES
           && (uint64_t)desc->base_offset
                      + PROFILE_REGION_BYTES(desc->capacity)
                  <= ND_IDIS_RELEASE_MARKER_OFFSET;
}

/* Frozen pre-v4 region bases in table order. The v4 layout (commit ab9bcd9c)
 * moved 19 of them; the legacy probe reads these addresses, never the v4
 * base, or a pre-v4 image stays invisible and a new v4 ring overwrites it. */
static const uint32_t legacy_base_offsets[ND_IDIS_PROFILE_TABLE_COUNT] = {
    0u,      4096u,   135168u, 167936u, 331776u, 348160u,
    364544u, 380928u, 397312u, 413696u, 430080u, 446464u,
    450560u, 483328u, 516096u, 548864u, 581632u, 585728u,
    589824u, 593920u, 598016u, 602112u,
};

int nd_idis_profile_table_build(nd_idis_profile_config_t *configs, uint8_t max,
                                nd_store_t *flash,
                                const nd_clock_port_t *clock,
                                const nd_profile_watchdog_t *watchdog)
{
    if (configs == NULL || flash == NULL || clock == NULL
        || max < ND_IDIS_PROFILE_TABLE_COUNT) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        const nd_idis_profile_desc_t *desc = &table[index];
        if (!desc_is_valid(desc)) {
            return -1;
        }
        configs[index] = (nd_idis_profile_config_t){
            .ln = desc->ln,
            .store = flash,
            .base_offset = desc->base_offset,
            .capacity = desc->capacity,
            .slot_size = desc->slot_size,
            .sector_size = PROFILE_SECTOR_BYTES,
            .store_opts = {
                .legacy_base_offset = legacy_base_offsets[index],
                .watchdog = watchdog,
            },
            .clock = clock,
            .capture_count = desc->capture_count,
            .capture_period = desc->capture_period,
            .compressed = desc->compressed,
            .delta = desc->delta,
        };
        memcpy(configs[index].captures,
               &capture_pool[desc->capture_first],
               (size_t)desc->capture_count * sizeof(nd_capture_object_t));
    }
    return ND_IDIS_PROFILE_TABLE_COUNT;
}

int nd_idis_profile_table_init(nd_idis_profiles_t *profiles, nd_store_t *flash,
                               const nd_clock_port_t *clock,
                               const nd_profile_watchdog_t *watchdog,
                               uint8_t reset_legacy)
{
    if (profiles == NULL || flash == NULL || clock == NULL) {
        return -1;
    }
    memset(profiles, 0, sizeof(*profiles));
    nd_idis_release_marker_t marker = {
        .store = flash,
        .offset = ND_IDIS_RELEASE_MARKER_OFFSET,
    };
    int release_pending = nd_idis_profiles_marker_begin(profiles, &marker,
                                                         watchdog);
    if (release_pending < 0) {
        return -1;
    }
    uint8_t legacy = 0;
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        const nd_idis_profile_desc_t *desc = &table[index];
        if (!desc_is_valid(desc)) {
            return -1;
        }
        nd_profile_store_opts_t opts = {
            .legacy_base_offset = legacy_base_offsets[index],
            .watchdog = watchdog,
        };
        int status = nd_idis_profile_init(
            &profiles->items[index], flash, desc->base_offset, desc->capacity,
            desc->slot_size, PROFILE_SECTOR_BYTES, &opts, &desc->ln, clock,
            &capture_pool[desc->capture_first], desc->capture_count,
            desc->capture_period, desc->compressed, desc->delta);
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
    profiles->count = ND_IDIS_PROFILE_TABLE_COUNT;
    if (legacy != 0 || release_pending) {
        for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
            nd_profile_store_hold(&profiles->items[index].store);
        }
    }
    return ND_IDIS_PROFILE_TABLE_COUNT;
}

int nd_idis_profile_table_boot(nd_idis_profiles_t *profiles, nd_store_t *flash,
                               const nd_clock_port_t *clock,
                               const nd_profile_watchdog_t *watchdog)
{
    return nd_idis_profile_table_init(profiles, flash, clock, watchdog, 0) > 0
               ? 0
               : -1;
}
