#include "nidavellir.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "kernel/legacy_control_block.h"
#include "domain/events/mac_suspend.h"

static nd_mac_suspend_state_t sec_state;

static void now_datetime(struct instance_t *instance, nd_datetime_t *now)
{
    now->second = (uint8_t)instance->time.tm_sec;
    now->minute = (uint8_t)instance->time.tm_min;
    now->hour = (uint8_t)instance->time.tm_hour;
    now->day = (uint8_t)instance->time.tm_mday;
    now->month = (uint8_t)(instance->time.tm_mon + 1);
    now->year = (uint16_t)(instance->time.tm_year + 1900);
}

static void state_from_mbase(struct mbase_control_block_t *mbase_cb,
                             nd_mac_suspend_state_t *state)
{
    state->error_count = nd_mbase_mac_err_cnt(mbase_cb);
    state->suspended = nd_mbase_mac_suspend_state(mbase_cb);
    state->suspend_date_set = nd_mbase_mac_suspend_date(mbase_cb)[0] != 0;
    state->suspend_day = nd_mbase_mac_suspend_date(mbase_cb)[0];
    state->suspend_month = nd_mbase_mac_suspend_date(mbase_cb)[1];
    state->suspend_year = (uint16_t)(nd_mbase_mac_suspend_date(mbase_cb)[2] + 1900);
}

static void state_to_mbase(struct mbase_control_block_t *mbase_cb,
                           const nd_mac_suspend_state_t *state)
{
    nd_mbase_mac_err_cnt(mbase_cb) = (uint8_t)state->error_count;
    nd_mbase_mac_suspend_state(mbase_cb) = state->suspended;
    if (state->suspend_date_set) {
        nd_mbase_mac_suspend_date(mbase_cb)[0] = state->suspend_day;
        nd_mbase_mac_suspend_date(mbase_cb)[1] = state->suspend_month;
        nd_mbase_mac_suspend_date(mbase_cb)[2] = (uint8_t)(state->suspend_year - 1900);
    }
}

void mac_err_cnt_add(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    nd_mac_suspend_state_t state = {0};
    nd_datetime_t now;

    now_datetime(instance, &now);
    state_from_mbase(mbase_cb, &state);
    nd_mac_suspend_add(&state, 200, &now);
    state_to_mbase(mbase_cb, &state);
}


void mac_suspend_reset(struct instance_t* instance)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);

    nd_mbase_mac_suspend_state(mbase_cb) = false;
    nd_mbase_mac_suspend_date(mbase_cb)[0] = 0;
    nd_mbase_mac_suspend_date(mbase_cb)[1] = 0;
    nd_mbase_mac_suspend_date(mbase_cb)[2] = 0;
    nd_mbase_mac_err_cnt(mbase_cb) = 0;
    sec_state.error_count = 0;
    sec_state.suspended = 0;
    sec_state.suspend_date_set = 0;
}


void mac_suspend_sec_pro(struct instance_t* instance, struct pipeline_t* task)
{
    struct mbase_control_block_t* mbase_cb = (struct mbase_control_block_t*)get_pipeline_ctx(instance, pid_meter_base);
    nd_datetime_t now;

    now_datetime(instance, &now);
    state_from_mbase(mbase_cb, &sec_state);
    if (nd_mac_suspend_step(&sec_state, 200, &now)) {
        nd_mbase_password_err_cnt(mbase_cb) = 0;
    }
    state_to_mbase(mbase_cb, &sec_state);
}
