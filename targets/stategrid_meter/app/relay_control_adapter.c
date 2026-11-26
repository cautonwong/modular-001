#include "targets/stategrid_meter/app/relay_control_adapter.h"
#include "targets/stategrid_meter/app/resource_ids.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "application/adapters/dl698/fr_698_env.h"

#include "fr_645_report.h"
#include "fr_698.h"
#include "targets/stategrid_meter/app/events/gs_record.h"
#include "targets/stategrid_meter/board/board_io.h"

#include "domain/load_control/load_control.h"
#include "targets/stategrid_meter/app/relay_control_ui.h"
#include "infra/memory/arena.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "targets/stategrid_meter/app/events/events_applications.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"
#include "targets/stategrid_meter/board/relay_config.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"

uint16_t relay_off_chk, relay_on_chk;

void save_relay_state(stategrid_context_t *instance)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    struct mbase_control_block_t *mbase_cb = ((stategrid_context_t *)instance)->mbase;
    uint32_t s_addr = ((uint32_t)(RES_DEV_EEPROM << 24)
                       + ((uint32_t)mbase_block_persistent_eprom << 16)
                       + nd_mbase_offset_cmd()
                       - nd_mbase_offset_persistent_section_start());
    uint32_t len = nd_mbase_offset_sta() - nd_mbase_offset_cmd()
                   + sizeof(nd_mbase_sta(mbase_cb));
    nd_set_offset_port(port, s_addr, (uint8_t *)&nd_mbase_cmd(mbase_cb), len);
}

/* shortcut: one relay state per firmware. Move it into the target
 * composition root when the target owns instance state. */
static uint8_t state_buffer[128];
static nd_arena_t state_arena;
static load_control_state_t *control;

static load_control_state_t *relay_state(void)
{
    if (control == NULL) {
        nd_arena_init(&state_arena, state_buffer, sizeof(state_buffer));
        control = load_control_create(nd_arena_allocator(&state_arena));
    }
    return control;
}

void relay_control_second(void *instance)
{
    struct mbase_control_block_t *mbase_cb = ((stategrid_context_t *)instance)->mbase;
    load_control_state_t *state = relay_state();
    nd_load_inputs_t inputs;
    nd_load_actions_t actions;
    uint8_t voltage_blocked;
    uint8_t command;
    uint8_t voltage_ok;

    if (state == NULL) {
        return;
    }

    inputs.current = nd_mbase_current_l(mbase_cb);
    inputs.voltage = nd_mbase_voltage_ac(mbase_cb);
    inputs.volt_low = nd_mbase_volt_78(mbase_cb);
    inputs.volt_high = nd_mbase_volt_120(mbase_cb);
    inputs.protection_current = nd_mbase_I_Prot(mbase_cb);
    inputs.protection_minutes = nd_mbase_Prot_time(mbase_cb);
    inputs.factory_mode = nd_mbase_factor_mode(mbase_cb);

    load_control_set_timers(state, nd_mbase_alarmmin(mbase_cb), nd_mbase_imaxmin(mbase_cb),
                            nd_mbase_limitmin(mbase_cb));
    command = nd_mbase_sta(mbase_cb);
    voltage_ok = (uint8_t)(inputs.factory_mode
                           || (inputs.voltage >= inputs.volt_low
                               && inputs.voltage <= inputs.volt_high));
    relay_control_ui_key_tick(&((stategrid_context_t *)instance)->relay_ui,
                              &command, (uint8_t)nd_board_key_pressed(),
                              voltage_ok);
    load_control_set_command(state, command);
    load_control_set_meter_clear(state,
                                 (uint8_t)(nd_mbase_meterclear(mbase_cb) == 0x5A));
    if (nd_mbase_relay_do(mbase_cb) == 0x5A) {
        load_control_request_repeat(state);
    }

    load_control_second(state, &inputs, &actions);

    nd_mbase_alarmmin(mbase_cb) = load_control_alarm_minutes(state);
    nd_mbase_imaxmin(mbase_cb) = load_control_current_limit_minutes(state);
    nd_mbase_limitmin(mbase_cb) = load_control_limit_minutes(state);
    nd_mbase_sta(mbase_cb) = load_control_command(state);

    if (actions.connect) {
        Magnetic_hold_relay_connect();
        relay_off_chk = 0;
        relay_on_chk = 0;
    }
    if (actions.disconnect) {
        Magnetic_hold_relay_disconnect();
        relay_off_chk = 0;
        relay_on_chk = 0;
    }
    if (actions.event_switch_on) {
        send_event_mesg_pro(instance, NULL, SwitchOnS, NULL, 1);
        nd_mbase_switch_on_succees(mbase_cb) = 0x5A;
    }
    if (actions.event_switch_off) {
        send_event_mesg_pro(instance, NULL, SwitchOffS, NULL, 1);
        nd_mbase_switch_off_succees(mbase_cb) = 0x5A;
    }
    if (actions.transition) {
        nd_mbase_relayerr_record(mbase_cb) = 0x5A;
        nd_mbase_relay_do(mbase_cb) = 0;
    } else if (nd_mbase_relay_do(mbase_cb) == 0x5A && (actions.connect || actions.disconnect)) {
        nd_mbase_relay_do(mbase_cb) = 0;
    }

    voltage_blocked = (uint8_t)(!inputs.factory_mode
                                && (inputs.voltage < inputs.volt_low
                                    || inputs.voltage > inputs.volt_high));
    if (!voltage_blocked) {
        nd_mbase_meterclear(mbase_cb) = 0;
    }
}

void relay_control_led(void *instance)
{
    struct mbase_control_block_t *mbase_cb = ((stategrid_context_t *)instance)->mbase;
    load_control_state_t *state = relay_state();
    nd_load_feedback_inputs_t inputs;
    nd_load_feedback_t feedback;
    relay_control_led_t led;

    if (nd_mbase_Page_ID(mbase_cb) == 0xFFFF) {
        nd_board_relay_led_set(1);
        return;
    }
    if (state == NULL) {
        return;
    }

    inputs.active_power = nd_mbase_active_power(mbase_cb);
    inputs.current = nd_mbase_current_l(mbase_cb);
    inputs.relay_off_elapsed = relay_off_chk;
    inputs.relay_on_elapsed = relay_on_chk;
    inputs.feedback_threshold = Relay_Ck_Time;
    inputs.min_current = MIN_CURRENT;
    inputs.suppress_feedback =
        (uint8_t)(((stategrid_context_t *)instance)->relay_ui.led_flash > 0);

    load_control_feedback(state, &inputs, &feedback);

    led = relay_control_ui_led(&((stategrid_context_t *)instance)->relay_ui,
                               load_control_command(state),
                               load_control_relay_true(state),
                               ((stategrid_context_t *)instance)->time.tm_sec);
    nd_mbase_relay_true_sta(mbase_cb) = load_control_relay_true(state);
    nd_board_relay_led_set(led.led_on);
    if (led.led_blink) {
        return;
    }

    nd_mbase_SwitchErrS_status(mbase_cb) = feedback.misoperation;

    if (feedback.event_switch_off && nd_mbase_switch_off_succees(mbase_cb) == 0x5A) {
        nd_mbase_switch_off_succees(mbase_cb) = 0;
        report_645state_judge(instance, RPT_2015_SWITCH_OFF_SUCCEES);
        report_2015state_update(instance, RPT_2015_SWITCH_OFF_SUCCEES);
    }
    if (feedback.event_switch_on && nd_mbase_switch_on_succees(mbase_cb) == 0x5A) {
        nd_mbase_switch_on_succees(mbase_cb) = 0;
        report_645state_judge(instance, RPT_2015_SWITCH_ON_SUCCEES);
        report_2015state_update(instance, RPT_2015_SWITCH_ON_SUCCEES);
    }
}
