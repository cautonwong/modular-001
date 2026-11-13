#include "domain/report/report_retry.h"

static int retry_due(const nd_report_retry_t *state, uint8_t item)
{
    return (state->retry_dly[item] == 0u)
           && (state->retry_cnt[item] < ND_REPORT_RETRY_LIMIT);
}

nd_report_action_t nd_report_retry_next(const nd_report_retry_t *state,
                                        const nd_report_retry_input_t *input)
{
    if (!input->power_on) {
        if (input->reset_flag == ND_REPORT_RESET_FLAG) {
            return ND_REPORT_ACTION_NONE;
        }
        if ((input->ac_power_off_sec >= 1u) && (input->ac_power_off_sec <= 3u)
            && (input->power_on_sec >= input->power_on_sec_limit)
            && (state->retry_cnt[ND_REPORT_RETRY_3011_START]
                < ND_REPORT_RETRY_LIMIT)) {
            return ND_REPORT_ACTION_3011_START;
        }
        return ND_REPORT_ACTION_NONE;
    }

    if (input->power_on_report_delay < 5u) {
        return ND_REPORT_ACTION_WAIT;
    }

    if (state->cmdtype & ND_REPORT_CMD_3011_END) {
        if (retry_due(state, ND_REPORT_RETRY_3011_END)) {
            return ND_REPORT_ACTION_3011_END;
        }
    } else if (state->cmdtype & ND_REPORT_CMD_3011_START) {
        return ND_REPORT_ACTION_3011_START;
    }

    if ((state->cmdtype & ND_REPORT_CMD_3011_END)
        && (state->retry_cnt[ND_REPORT_RETRY_3011_END]
            < ND_REPORT_RETRY_LIMIT)) {
        return ND_REPORT_ACTION_NONE;
    }
    if (state->retry_dly[ND_REPORT_RETRY_3011_END] != 0u) {
        return ND_REPORT_ACTION_NONE;
    }
    if (state->cmdtype & (ND_REPORT_CMD_3320 | ND_REPORT_CMD_2015)) {
        return ND_REPORT_ACTION_NORMAL;
    }
    return ND_REPORT_ACTION_NONE;
}

void nd_report_retry_3011_sent(nd_report_retry_t *state, uint8_t event_type,
                               uint8_t get_data, int success)
{
    uint8_t item = event_type ? ND_REPORT_RETRY_3011_END
                              : ND_REPORT_RETRY_3011_START;

    if (!get_data) {
        return;
    }
    if (!success) {
        state->cmdtype &= (uint16_t)~(1u << item);
        return;
    }

    if (event_type) {
        state->retry_dly[item] = ND_REPORT_RETRY_DELAY_SEC;
    }
    state->retry_cnt[item]++;
    if (state->retry_cnt[item] == ND_REPORT_RETRY_LIMIT) {
        state->piid++;
        if (!event_type) {
            state->cmdtype &= (uint16_t)~ND_REPORT_CMD_3011_START;
        }
    }
}

void nd_report_retry_3011_start_off_sent(nd_report_retry_t *state,
                                         uint8_t get_data, int success)
{
    if (get_data && success) {
        state->retry_cnt[ND_REPORT_RETRY_3011_START]++;
    }
}

int nd_report_retry_normal_prepare(nd_report_retry_t *state, uint8_t get_data,
                                   int has_3320, int has_2015,
                                   int *include_3320, int *include_2015)
{
    int need_piid = 0;

    *include_3320 = 0;
    *include_2015 = 0;

    if ((state->cmdtype & ND_REPORT_CMD_3320)
        && retry_due(state, ND_REPORT_RETRY_3320)) {
        if (has_3320) {
            *include_3320 = 1;
            if (get_data) {
                state->retry_cnt[ND_REPORT_RETRY_3320]++;
                if (state->retry_cnt[ND_REPORT_RETRY_3320]
                    == ND_REPORT_RETRY_LIMIT) {
                    need_piid = 1;
                }
            }
        } else if (get_data) {
            state->cmdtype &= (uint16_t)~ND_REPORT_CMD_3320;
        }
    }

    if ((state->cmdtype & ND_REPORT_CMD_2015)
        && (state->retry_dly[ND_REPORT_RETRY_3320] == 0u)
        && (state->retry_cnt[ND_REPORT_RETRY_2015] < ND_REPORT_RETRY_LIMIT)) {
        if (has_2015) {
            *include_2015 = 1;
            if (get_data) {
                state->retry_cnt[ND_REPORT_RETRY_2015]++;
                if (state->retry_cnt[ND_REPORT_RETRY_2015]
                    == ND_REPORT_RETRY_LIMIT) {
                    need_piid = 1;
                }
            }
        } else if (get_data) {
            state->cmdtype &= (uint16_t)~ND_REPORT_CMD_2015;
        }
    }

    if (need_piid) {
        state->piid++;
    }
    return need_piid;
}

void nd_report_retry_normal_sent(nd_report_retry_t *state, uint8_t get_data,
                                 int success)
{
    if (get_data && success) {
        state->retry_dly[ND_REPORT_RETRY_3320] = ND_REPORT_RETRY_DELAY_SEC;
        state->retry_dly[ND_REPORT_RETRY_2015] = ND_REPORT_RETRY_DELAY_SEC;
    }
}
