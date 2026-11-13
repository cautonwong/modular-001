#ifndef ND_DOMAIN_REPORT_RETRY_H
#define ND_DOMAIN_REPORT_RETRY_H

#include <stdint.h>

enum {
    ND_REPORT_RETRY_3320 = 0,
    ND_REPORT_RETRY_2015 = 1,
    ND_REPORT_RETRY_3011_END = 2,
    ND_REPORT_RETRY_3011_START = 3,
    ND_REPORT_RETRY_COUNT = 4
};

#define ND_REPORT_CMD_3320 (1u << ND_REPORT_RETRY_3320)
#define ND_REPORT_CMD_2015 (1u << ND_REPORT_RETRY_2015)
#define ND_REPORT_CMD_3011_END (1u << ND_REPORT_RETRY_3011_END)
#define ND_REPORT_CMD_3011_START (1u << ND_REPORT_RETRY_3011_START)

#define ND_REPORT_RETRY_LIMIT 3u
#define ND_REPORT_RETRY_DELAY_SEC 10u
#define ND_REPORT_RESET_FLAG 0x5Au

typedef struct {
    uint16_t cmdtype;
    uint8_t retry_dly[ND_REPORT_RETRY_COUNT];
    uint8_t retry_cnt[ND_REPORT_RETRY_COUNT];
    uint8_t piid;
} nd_report_retry_t;

typedef struct {
    uint8_t power_on;
    uint8_t reset_flag;
    uint8_t ac_power_off_sec;
    uint16_t power_on_sec;
    uint16_t power_on_sec_limit;
    uint8_t power_on_report_delay;
} nd_report_retry_input_t;

typedef enum {
    ND_REPORT_ACTION_NONE = 0,
    ND_REPORT_ACTION_3011_START,
    ND_REPORT_ACTION_3011_END,
    ND_REPORT_ACTION_NORMAL,
    ND_REPORT_ACTION_WAIT
} nd_report_action_t;

nd_report_action_t nd_report_retry_next(const nd_report_retry_t *state,
                                        const nd_report_retry_input_t *input);
void nd_report_retry_3011_sent(nd_report_retry_t *state, uint8_t event_type,
                               uint8_t get_data, int success);
void nd_report_retry_3011_start_off_sent(nd_report_retry_t *state,
                                         uint8_t get_data, int success);
int nd_report_retry_normal_prepare(nd_report_retry_t *state, uint8_t get_data,
                                   int has_3320, int has_2015,
                                   int *include_3320, int *include_2015);
void nd_report_retry_normal_sent(nd_report_retry_t *state, uint8_t get_data,
                                 int success);

#endif
