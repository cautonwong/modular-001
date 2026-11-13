#include "targets/idis_ip_meter/cosem/nd_idis_profile_table.h"

#include <string.h>

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

/* Flash regions. Each profile has its own 4 KiB aligned area. */
#define FLASH_BILLING 0u
#define FLASH_LP1 4096u
#define FLASH_LP2 135168u
#define FLASH_PQ 167936u
#define FLASH_EVENT_STANDARD 331776u
#define FLASH_EVENT_FRAUD 348160u
#define FLASH_EVENT_DISCONNECTOR 364544u
#define FLASH_EVENT_MBUS 380928u
#define FLASH_EVENT_PQ 397312u
#define FLASH_EVENT_COMM 413696u
#define FLASH_EVENT_COMM_DETAILS 430080u
#define FLASH_POWER_FAILURE 446464u
#define FLASH_MBUS_LOAD_1 450560u
#define FLASH_MBUS_LOAD_2 483328u
#define FLASH_MBUS_LOAD_3 516096u
#define FLASH_MBUS_LOAD_4 548864u
#define FLASH_MBUS_CONTROL_1 581632u
#define FLASH_MBUS_CONTROL_2 585728u
#define FLASH_MBUS_CONTROL_3 589824u
#define FLASH_MBUS_CONTROL_4 593920u
#define FLASH_DISPLAY_GENERAL 598016u
#define FLASH_DISPLAY_ALTERNATE 602112u

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
    nd_capture_object_t captures[ND_IDIS_PROFILE_MAX_CAPTURES];
} nd_idis_profile_desc_t;

static const nd_idis_profile_desc_t table[ND_IDIS_PROFILE_TABLE_COUNT] = {
    /* 1: Data of billing period 1. */
    {
        .ln = ND_OBIS_BILLING_PROFILE,
        .base_offset = FLASH_BILLING,
        .capacity = 13,
        .slot_size = 128,
        .capture_period = 0,
        .compressed = 1,
        .capture_count = 5,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2), /* 1.8.1 */
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_REACTIVE_ENERGY_IMPORT, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_REACTIVE_ENERGY_EXPORT, 2),
        },
    },
    /* 2: Load profile 1. */
    {
        .ln = ND_OBIS_LOAD_PROFILE_1,
        .base_offset = FLASH_LP1,
        .capacity = 960,
        .slot_size = 128,
        .capture_period = 900,
        .compressed = 1,
        .delta = 1,
        .capture_count = 4,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
        },
    },
    /* 3: Load profile 2. */
    {
        .ln = ND_OBIS_LOAD_PROFILE_2,
        .base_offset = FLASH_LP2,
        .capacity = 240,
        .slot_size = 128,
        .capture_period = 86400,
        .compressed = 1,
        .delta = 1,
        .capture_count = 4,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_2, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_EXPORT, 2),
        },
    },
    /* 4: Power quality profile. */
    {
        .ln = ND_OBIS_PQ_PROFILE,
        .base_offset = FLASH_PQ,
        .capacity = 1152,
        .slot_size = 128,
        .capture_period = 60,
        .compressed = 1,
        .delta = 1,
        .capture_count = 12,
        .captures = {
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
        },
    },
    /* 5: Standard event log. */
    {
        .ln = ND_OBIS_STANDARD_EVENT_LOG,
        .base_offset = FLASH_EVENT_STANDARD,
        .capacity = 100,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_STANDARD, 2),
        },
    },
    /* 6: Fraud detection log. */
    {
        .ln = ND_OBIS_FRAUD_LOG,
        .base_offset = FLASH_EVENT_FRAUD,
        .capacity = 30,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_FRAUD, 2),
        },
    },
    /* 7: Disconnector control log. */
    {
        .ln = ND_OBIS_DISCONNECTOR_LOG,
        .base_offset = FLASH_EVENT_DISCONNECTOR,
        .capacity = 10,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 3,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_DISCONNECTOR, 2),
            CAP(ND_IC_LIMITER, ND_OBIS_LIMITER, 3),
        },
    },
    /* 8: M-Bus event log. */
    {
        .ln = ND_OBIS_MBUS_EVENT_LOG,
        .base_offset = FLASH_EVENT_MBUS,
        .capacity = 10,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
        },
    },
    /* 9: Power quality log. */
    {
        .ln = ND_OBIS_PQ_LOG,
        .base_offset = FLASH_EVENT_PQ,
        .capacity = 100,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_PQ, 2),
        },
    },
    /* 10: Communication log. */
    {
        .ln = ND_OBIS_COMM_LOG,
        .base_offset = FLASH_EVENT_COMM,
        .capacity = 100,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_COMM, 2),
        },
    },
    /* 11: Communication details log. */
    {
        .ln = ND_OBIS_COMM_DETAILS_LOG,
        .base_offset = FLASH_EVENT_COMM_DETAILS,
        .capacity = 100,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_COMM_DETAILS, 2),
        },
    },
    /* 12: Power failure event log. */
    {
        .ln = ND_OBIS_POWER_FAILURE_LOG,
        .base_offset = FLASH_POWER_FAILURE,
        .capacity = 10,
        .slot_size = 128,
        .capture_period = 0,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_POWER_FAILURE_DURATION, 2),
        },
    },
    /* 13..16: M-Bus load profiles, one per channel. */
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_1,
        .base_offset = FLASH_MBUS_LOAD_1,
        .capacity = 240,
        .slot_size = 128,
        .capture_period = 3600,
        .capture_count = 10,
        .captures = {
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
        },
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_2,
        .base_offset = FLASH_MBUS_LOAD_2,
        .capacity = 240,
        .slot_size = 128,
        .capture_period = 3600,
        .capture_count = 10,
        .captures = {
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
        },
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_3,
        .base_offset = FLASH_MBUS_LOAD_3,
        .capacity = 240,
        .slot_size = 128,
        .capture_period = 3600,
        .capture_count = 10,
        .captures = {
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
        },
    },
    {
        .ln = ND_OBIS_MBUS_LOAD_PROFILE_4,
        .base_offset = FLASH_MBUS_LOAD_4,
        .capacity = 240,
        .slot_size = 128,
        .capture_period = 3600,
        .capture_count = 10,
        .captures = {
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
        },
    },
    /* 17..20: M-Bus control logs, one per channel. */
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_1,
        .base_offset = FLASH_MBUS_CONTROL_1,
        .capacity = 10,
        .slot_size = 128,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
        },
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_2,
        .base_offset = FLASH_MBUS_CONTROL_2,
        .capacity = 10,
        .slot_size = 128,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
        },
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_3,
        .base_offset = FLASH_MBUS_CONTROL_3,
        .capacity = 10,
        .slot_size = 128,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
        },
    },
    {
        .ln = ND_OBIS_MBUS_CONTROL_LOG_4,
        .base_offset = FLASH_MBUS_CONTROL_4,
        .capacity = 10,
        .slot_size = 128,
        .capture_count = 2,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_DATA, ND_OBIS_EVENT_OBJECT_MBUS, 2),
        },
    },
    /* 21..22: Display profiles. Column lists are vendor defined. */
    {
        .ln = ND_OBIS_DISPLAY_GENERAL,
        .base_offset = FLASH_DISPLAY_GENERAL,
        .capacity = 10,
        .slot_size = 128,
        .capture_count = 3,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
            CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1, 2),
        },
    },
    {
        .ln = ND_OBIS_DISPLAY_ALTERNATE,
        .base_offset = FLASH_DISPLAY_ALTERNATE,
        .capacity = 10,
        .slot_size = 128,
        .capture_count = 3,
        .captures = {
            CAP(ND_IC_CLOCK, ND_OBIS_CLOCK, 2),
            CAP(ND_IC_REGISTER, ND_OBIS_ACTIVE_ENERGY_IMPORT, 2),
            CAP(ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1, 2),
        },
    },
};

int nd_idis_profile_table_build(nd_idis_profile_config_t *configs, uint8_t max,
                                nd_store_t *flash,
                                const nd_clock_port_t *clock)
{
    if (configs == NULL || flash == NULL || clock == NULL
        || max < ND_IDIS_PROFILE_TABLE_COUNT) {
        return -1;
    }
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        const nd_idis_profile_desc_t *desc = &table[index];
        configs[index] = (nd_idis_profile_config_t){
            .ln = desc->ln,
            .store = flash,
            .base_offset = desc->base_offset,
            .capacity = desc->capacity,
            .slot_size = desc->slot_size,
            .clock = clock,
            .capture_count = desc->capture_count,
            .capture_period = desc->capture_period,
            .compressed = desc->compressed,
            .delta = desc->delta,
        };
        memcpy(configs[index].captures, desc->captures,
               sizeof(desc->captures));
    }
    return ND_IDIS_PROFILE_TABLE_COUNT;
}

int nd_idis_profile_table_init(nd_idis_profiles_t *profiles, nd_store_t *flash,
                               const nd_clock_port_t *clock)
{
    if (profiles == NULL || flash == NULL || clock == NULL) {
        return -1;
    }
    memset(profiles, 0, sizeof(*profiles));
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        const nd_idis_profile_desc_t *desc = &table[index];
        if (nd_idis_profile_init(&profiles->items[index], flash,
                                 desc->base_offset, desc->capacity,
                                 desc->slot_size, &desc->ln, clock,
                                 desc->captures, desc->capture_count,
                                 desc->capture_period, desc->compressed,
                                 desc->delta) != 0) {
            return -1;
        }
        nd_profile_store_set_sector_slots(&profiles->items[index].store, 32u);
    }
    profiles->count = ND_IDIS_PROFILE_TABLE_COUNT;
    return ND_IDIS_PROFILE_TABLE_COUNT;
}
