#ifndef ND_DOMAIN_REPORT_POLICY_H
#define ND_DOMAIN_REPORT_POLICY_H

#include <stddef.h>
#include <stdint.h>

/* 698 report flags and modes, kept numeric so the adapter can pass its wire
   values straight through. */
enum {
    ND_REPORT_FLAG_START = 1,
    ND_REPORT_FLAG_END = 2,
    ND_REPORT_FLAG_ALL = 3
};

enum {
    ND_REPORT_MODE_ACTIVE = 0,
    ND_REPORT_MODE_FOLLOW = 1
};

enum {
    ND_REPORT_CHANNEL_IR = 0,
    ND_REPORT_CHANNEL_RS485_1 = 1,
    ND_REPORT_CHANNEL_PLC = 2,
    ND_REPORT_CHANNEL_RS485_2 = 3
};

int nd_report_flag_matches(uint8_t report_flag, uint8_t scheme);
int nd_report_channel_enabled(uint16_t channel, const uint8_t *buffer,
                              size_t length);
int nd_report_2015_enabled(uint32_t configured_mask, uint32_t state_mask);
int nd_report_3011_active_allowed(uint8_t event_type, uint8_t report_flag,
                                  uint8_t report_mode, int has_start_record);
uint32_t nd_report_3320_variant(uint32_t oad, unsigned index);

#endif
