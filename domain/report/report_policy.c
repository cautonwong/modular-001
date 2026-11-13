#include "domain/report/report_policy.h"

static uint32_t channel_oad(uint16_t channel)
{
    switch (channel) {
    case ND_REPORT_CHANNEL_IR:
        return 0xF2020201u;
    case ND_REPORT_CHANNEL_RS485_1:
        return 0xF2010201u;
    case ND_REPORT_CHANNEL_PLC:
        return 0xF2090201u;
    case ND_REPORT_CHANNEL_RS485_2:
        return 0xF2010202u;
    default:
        return 0;
    }
}

int nd_report_flag_matches(uint8_t report_flag, uint8_t scheme)
{
    switch (report_flag) {
    case ND_REPORT_FLAG_START:
        return (scheme == 1u) || (scheme == 2u);
    case ND_REPORT_FLAG_END:
        return (scheme == 3u) || (scheme == 4u);
    case ND_REPORT_FLAG_ALL:
        return (scheme >= 1u) && (scheme <= 4u);
    default:
        return 0;
    }
}

int nd_report_channel_enabled(uint16_t channel, const uint8_t *buffer,
                              size_t length)
{
    uint32_t expected = channel_oad(channel);
    uint8_t count;
    uint8_t i;

    if ((buffer == NULL) || (length < 1u) || (expected == 0u)) {
        return 0;
    }

    count = buffer[0];
    if ((count == 0u) || (count > 4u) || (length < (size_t)(1u + 4u * count))) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        const uint8_t *p = buffer + 1u + 4u * i;
        uint32_t oad = (uint32_t)p[0] | ((uint32_t)p[1] << 8)
                       | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);

        if (oad == expected) {
            return 1;
        }
    }
    return 0;
}

int nd_report_2015_enabled(uint32_t configured_mask, uint32_t state_mask)
{
    return (configured_mask & state_mask) != 0u;
}

int nd_report_3011_active_allowed(uint8_t event_type, uint8_t report_flag,
                                  uint8_t report_mode, int has_start_record)
{
    if ((event_type == 1u) && !has_start_record) {
        return 0;
    }

    if (event_type == 1u) {
        if ((report_flag != ND_REPORT_FLAG_END)
            && (report_flag != ND_REPORT_FLAG_ALL)) {
            return 0;
        }
    } else if ((report_flag != ND_REPORT_FLAG_START)
               && (report_flag != ND_REPORT_FLAG_ALL)) {
        return 0;
    }

    return report_mode == ND_REPORT_MODE_ACTIVE;
}

uint32_t nd_report_3320_variant(uint32_t oad, unsigned index)
{
    return ((oad & 0xffff1fffu) + 0x100u * index) & 0xffff1fffu;
}
