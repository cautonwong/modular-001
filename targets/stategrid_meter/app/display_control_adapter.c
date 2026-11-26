#include "targets/stategrid_meter/app/message.h"
#include "targets/stategrid_meter/app/meter_flags.h"
#include "targets/stategrid_meter/app/display_control_adapter.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

#include "targets/stategrid_meter/app/app_config.h"
#include "targets/stategrid_meter/app/display/backlight.h"
#include "targets/stategrid_meter/app/display/backlight_control.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/app/stategrid_context.h"

static nd_rtc_battery_state_t battery_state;

void check_RtcBat_volt(stategrid_context_t *instance)
{
    struct mbase_control_block_t *mbase_cb =
        ((stategrid_context_t *)instance)->mbase;
    battery_state.undervoltage = nd_mbase_RtcBat_UnderV_state(mbase_cb);
    nd_rtc_battery_tick(nd_mbase_voltage_battery(mbase_cb),
                        RtcBat_Low_Level_Dwn, RtcBat_Low_Level_Up,
                        &battery_state);
    nd_mbase_RtcBat_UnderV_state(mbase_cb) = battery_state.undervoltage;
}

int backlight_pro(stategrid_context_t *instance, void *task,
                  struct message_t *mesg)
{
    (void)task;
    (void)mesg;
    struct mbase_control_block_t *mbase_cb =
        ((stategrid_context_t *)instance)->mbase;
    nd_backlight_inputs_t inputs = {
        .page_id = nd_mbase_Page_ID(mbase_cb),
        .remote_alarm = (nd_mbase_sta(mbase_cb) & F_RemoteAlarm) != 0,
        .alarm_minutes_active = nd_mbase_alarmmin(mbase_cb) > 0,
        .current_limit_active = nd_mbase_imaxmin(mbase_cb) > 0,
        .alarm_word = nd_mbase_blt_alarm(mbase_cb),
        .rtc_battery_undervoltage =
            nd_mbase_RtcBat_UnderV_state(mbase_cb) != 0,
    };
    uint16_t timer = nd_mbase_BackLightTmr(mbase_cb);
    int on = nd_backlight_tick(&inputs, &timer);
    nd_mbase_BackLightTmr(mbase_cb) = timer;
    nd_backlight_set(stategrid_backlight_port(instance), on);
    return on;
}
