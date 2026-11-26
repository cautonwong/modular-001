#include "targets/stategrid_meter/persistence/meter_base_view.h"
#include "targets/stategrid_meter/app/resource_ids.h"

#include <stddef.h>

#include "targets/stategrid_meter/app/stategrid_context.h"
#include "application/ports/resource_access.h"
#include "application/adapters/dl698/fr_698_env.h"
#include "targets/stategrid_meter/app/resource_blocks.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/app/metering_energy_adapter.h"
#include "targets/stategrid_meter/persistence/meter_record.h"
#include "targets/stategrid_meter/app/metering/metering_applications.h"

static struct mbase_control_block_t *mb(void *instance)
{
    return ((stategrid_context_t *)instance)->mbase;
}

int nd_mb_valid(void *instance)
{
    return mb(instance) != NULL;
}

uint32_t *nd_mb_report_2015_state_ptr(void *instance)
{
    return nd_mbase_report_2015state(mb(instance));
}

uint32_t *nd_mb_report_2015_cache_ptr(void *instance)
{
    return &nd_mbase_report_2015state_cache(mb(instance));
}

uint16_t *nd_mb_active_report_cmdtype_ptr(void *instance)
{
    return &nd_mbase_active_report_CMDtype(mb(instance));
}

uint8_t *nd_mb_active_report_retry_cnt_ptr(void *instance)
{
    return nd_mbase_active_report_retry_cnt(mb(instance));
}

uint8_t *nd_mb_active_report_retry_dly_ptr(void *instance)
{
    return nd_mbase_active_report_retry_dly(mb(instance));
}

uint8_t *nd_mb_report_piid_ptr(void *instance)
{
    return &nd_mbase_report_piid(mb(instance));
}

uint8_t *nd_mb_report_random_data_ptr(void *instance)
{
    return nd_mbase_report_random_data(mb(instance));
}

uint8_t *nd_mb_switch_on_success_ptr(void *instance)
{
    return &nd_mbase_switch_on_succees(mb(instance));
}

uint8_t *nd_mb_switch_off_success_ptr(void *instance)
{
    return &nd_mbase_switch_off_succees(mb(instance));
}

uint8_t *nd_mb_esam_chip_abnormal_ptr(void *instance)
{
    return &nd_mbase_esam_chip_abnormal(mb(instance));
}

uint8_t *nd_mb_auth_on_ptr(void *instance)
{
    return &nd_mbase_AuthOn(mb(instance));
}

uint32_t *nd_mb_report_2015_mode_ptr(void *instance)
{
    return &nd_mbase_report_2015mode(mb(instance));
}

uint8_t *nd_mb_poweron_active_report_dly_ptr(void *instance)
{
    return &nd_mbase_poweron_active_report_dly(mb(instance));
}

uint32_t *nd_mb_power_on_sec_accum_ptr(void *instance)
{
    return &nd_mbase_power_on_sec_accum(mb(instance));
}

uint16_t *nd_mb_active_report_sec_limit_ptr(void *instance)
{
    return &nd_mbase_active_report_sec_limit(mb(instance));
}

uint8_t *nd_mb_key_state_ptr(void *instance)
{
    return &nd_mbase_key_state(mb(instance));
}

uint32_t *nd_mb_voltage_battery_ptr(void *instance)
{
    return &nd_mbase_voltage_battery(mb(instance));
}

uint8_t *nd_mb_tsa_ptr(void *instance)
{
    return nd_mbase_tsa(mb(instance));
}

uint32_t (*nd_mb_connect_timeout_ptr(void *instance))[2]
{
    return nd_mbase_connect_timeout(mb(instance));
}

uint32_t (*nd_mb_esam_connect_timeout_ptr(void *instance))[2]
{
    return nd_mbase_esam_connect_timeout(mb(instance));
}

uint8_t (*nd_mb_connect_status_ptr(void *instance))[2]
{
    return nd_mbase_connect_status(mb(instance));
}

uint32_t (*nd_mb_connect_timeout_cnt_ptr(void *instance))[2]
{
    return nd_mbase_connect_timeout_cnt(mb(instance));
}

uint8_t *nd_mb_current_connect_channel_ptr(void *instance)
{
    return nd_mbase_current_connect_channel(mb(instance));
}

uint8_t *nd_mb_clientaddr_ptr(void *instance)
{
    return nd_mbase_clientaddr(mb(instance));
}

uint32_t *nd_mb_term_auth_timeout_ptr(void *instance)
{
    return &nd_mbase_term_auth_timeout(mb(instance));
}

uint32_t *nd_mb_infrared_auth_timeout_ptr(void *instance)
{
    return &nd_mbase_infrared_auth_timeout(mb(instance));
}

uint8_t *nd_mb_mac_suspend_state_ptr(void *instance)
{
    return &nd_mbase_mac_suspend_state(mb(instance));
}

uint8_t *nd_mb_factor_mode_ptr(void *instance)
{
    return &nd_mbase_factor_mode(mb(instance));
}

uint8_t *nd_mb_lcd_autodis_list_num_ptr(void *instance)
{
    return &nd_mbase_lcd_autodis_list_num(mb(instance));
}

uint16_t *nd_mb_lcd_autodis_keep_sec_ptr(void *instance)
{
    return &nd_mbase_lcd_autodis_keep_sec(mb(instance));
}

uint8_t *nd_mb_commdisp_plc_ptr(void *instance)
{
    return &nd_mbase_commdisp_plc(mb(instance));
}

uint8_t *nd_mb_commdisp_ir485_ptr(void *instance)
{
    return &nd_mbase_commdisp_ir485(mb(instance));
}

uint16_t *nd_mb_backlight_tmr_ptr(void *instance)
{
    return &nd_mbase_BackLightTmr(mb(instance));
}

void nd_mb_save_report_2015_state(void *instance)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    struct mbase_control_block_t *mbase_cb = mb(instance);
    uint32_t addr = ((uint32_t)RES_DEV_EEPROM << 24)
                    + ((uint32_t)mbase_block_persistent_eprom << 16)
                    + nd_mbase_offset_report_2015state()
                    - nd_mbase_offset_persistent_section_start();

    nd_set_offset_port(port, addr,
                       (uint8_t *)&nd_mbase_report_2015state(mbase_cb)[0],
                       4 * 4);
}

void nd_mb_save_active_report_state(void *instance)
{
    const nd_resource_port_t *port = nd_fr698_resource_access(instance);

    struct mbase_control_block_t *mbase_cb = mb(instance);
    uint32_t addr = ((uint32_t)RES_DEV_EEPROM << 24)
                    + ((uint32_t)mbase_block_persistent_eprom << 16)
                    + nd_mbase_offset_active_report_CMDtype()
                    - nd_mbase_offset_persistent_section_start();
    uint32_t len = nd_mbase_offset_active_report_retry_cnt()
                   - nd_mbase_offset_active_report_CMDtype()
                   + sizeof(nd_mbase_active_report_retry_cnt(mbase_cb));

    nd_set_offset_port(port, addr,
                       (uint8_t *)&nd_mbase_active_report_CMDtype(mbase_cb),
                       len);
}

uint8_t *nd_mb_boardadjtime_ptr(void *instance)
{
    return &nd_mbase_boardadjtime(mb(instance));
}

uint8_t *nd_mb_flag_event_clear_ptr(void *instance)
{
    return &nd_mbase_flag_event_clear(mb(instance));
}

uint16_t *nd_mb_limit_t_max_ptr(void *instance)
{
    return &nd_mbase_Limit_T_Max(mb(instance));
}

uint16_t *nd_mb_limit_t_min_ptr(void *instance)
{
    return &nd_mbase_Limit_T_Min(mb(instance));
}

uint8_t *nd_mb_rtc_errs_status_ptr(void *instance)
{
    return &nd_mbase_RTCErrS_status(mb(instance));
}

uint32_t *nd_mb_battery_used_time_cnt_ptr(void *instance)
{
    return &nd_mbase_battery_used_time_cnt(mb(instance));
}

uint16_t *nd_mb_trigger_insfreeze_delay_sec_ptr(void *instance)
{
    return &nd_mbase_trigger_insfreeze_delay_sec(mb(instance));
}

uint8_t *nd_mb_fee_ptr(void *instance)
{
    return &nd_mbase_Fee(mb(instance));
}

uint8_t *nd_mb_active_energy_cal_word_ptr(void *instance)
{
    return &nd_mbase_active_energy_cal_word(mb(instance));
}

uint16_t *nd_mb_backlight_key_time_ptr(void *instance)
{
    return &nd_mbase_backlight_key_time(mb(instance));
}

uint8_t *nd_mb_energy_point_ptr(void *instance)
{
    return &nd_mbase_energy_point(mb(instance));
}

uint8_t *nd_mb_lcd_12_mean_ptr(void *instance)
{
    return &nd_mbase_lcd_12_mean(mb(instance));
}

uint16_t *nd_mb_lcd_checkdis_keep_sec_ptr(void *instance)
{
    return &nd_mbase_lcd_checkdis_keep_sec(mb(instance));
}

uint16_t *nd_mb_lcd_nopower_key_keep_sec_ptr(void *instance)
{
    return &nd_mbase_lcd_nopower_key_keep_sec(mb(instance));
}

uint8_t *nd_mb_lcd_poweron_all_dis_time_ptr(void *instance)
{
    return &nd_mbase_lcd_poweron_all_dis_time(mb(instance));
}

uint8_t *nd_mb_meter_no_ptr(void *instance)
{
    return nd_mbase_meterNo(mb(instance));
}

uint8_t *nd_mb_power_point_ptr(void *instance)
{
    return &nd_mbase_power_point(mb(instance));
}

uint8_t *nd_mb_first_mbill_day_flag_ptr(void *instance)
{
    return &nd_mbase_first_MbillDay_flag(mb(instance));
}

uint8_t *nd_mb_password_err_cnt_ptr(void *instance)
{
    return &nd_mbase_password_err_cnt(mb(instance));
}

uint8_t *nd_mb_cover_open_s_status_ptr(void *instance)
{
    return &nd_mbase_CoverOpenS_status(mb(instance));
}

uint8_t *nd_mb_lcd_autodis_list_maxnum_ptr(void *instance)
{
    return &nd_mbase_lcd_autodis_list_maxnum(mb(instance));
}

uint16_t *nd_mb_lcd_keydis_keep_sec_ptr(void *instance)
{
    return &nd_mbase_lcd_keydis_keep_sec(mb(instance));
}

uint8_t *nd_mb_lcd_keydis_list_maxnum_ptr(void *instance)
{
    return &nd_mbase_lcd_keydis_list_maxnum(mb(instance));
}

uint8_t *nd_mb_lcd_keydis_list_num_ptr(void *instance)
{
    return &nd_mbase_lcd_keydis_list_num(mb(instance));
}

uint16_t *nd_mb_auto_inv_ptr(void *instance)
{
    return &nd_mbase_AutoInv(mb(instance));
}

uint8_t *nd_mb_auto_ptr_ptr(void *instance)
{
    return &nd_mbase_AutoPtr(mb(instance));
}

uint8_t *nd_mb_baut_plc_ptr(void *instance)
{
    return &nd_mbase_Baut_plc(mb(instance));
}

uint8_t *nd_mb_baut_plc_neg_ptr(void *instance)
{
    return &nd_mbase_Baut_plc_neg(mb(instance));
}

uint8_t *nd_mb_baut_rs485_ptr(void *instance)
{
    return &nd_mbase_Baut_rs485(mb(instance));
}

uint8_t *nd_mb_key_ptr_ptr(void *instance)
{
    return &nd_mbase_KeyPtr(mb(instance));
}

uint8_t *nd_mb_mul_port_mode_ptr(void *instance)
{
    return &nd_mbase_MulPortMode(mb(instance));
}

uint16_t *nd_mb_page_id_ptr(void *instance)
{
    return &nd_mbase_Page_ID(mb(instance));
}

uint8_t *nd_mb_baud_chg_ptr(void *instance)
{
    return &nd_mbase_baud_chg(mb(instance));
}

uint8_t *nd_mb_esam_power_close_dly_ptr(void *instance)
{
    return &nd_mbase_esam_power_close_dly(mb(instance));
}

uint8_t *nd_mb_relay_ctl_mode_ptr(void *instance)
{
    return &nd_mbase_relay_ctl_mode(mb(instance));
}

uint32_t nd_mb_offset_limit_t_max(void)
{
    return nd_mbase_offset_Limit_T_Max();
}

uint32_t nd_mb_offset_limit_t_min(void)
{
    return nd_mbase_offset_Limit_T_Min();
}

uint32_t nd_mb_offset_active_energy_cal_word(void)
{
    return nd_mbase_offset_active_energy_cal_word();
}

uint32_t nd_mb_offset_lcd_autodis_list_num(void)
{
    return nd_mbase_offset_lcd_autodis_list_num();
}

uint32_t nd_mb_offset_lcd_poweron_all_dis_time(void)
{
    return nd_mbase_offset_lcd_poweron_all_dis_time();
}


uint32_t nd_mb_offset_baut_rs485(void)
{
    return nd_mbase_offset_Baut_rs485();
}

uint32_t nd_mb_offset_factor_mode(void)
{
    return nd_mbase_offset_factor_mode();
}

uint32_t nd_mb_offset_lcd_autodis_keep_sec(void)
{
    return nd_mbase_offset_lcd_autodis_keep_sec();
}

uint32_t nd_mb_offset_lcd_keydis_keep_sec(void)
{
    return nd_mbase_offset_lcd_keydis_keep_sec();
}

uint32_t nd_mb_offset_lcd_keydis_list_num(void)
{
    return nd_mbase_offset_lcd_keydis_list_num();
}

uint32_t nd_mb_offset_baut_plc(void)
{
    return nd_mbase_offset_Baut_plc();
}

uint32_t nd_mb_offset_meter_no(void)
{
    return nd_mbase_offset_meterNo();
}

uint32_t nd_mb_offset_relay_ctl_mode(void)
{
    return nd_mbase_offset_relay_ctl_mode();
}

uint32_t nd_mb_offset_term_auth_timeout(void)
{
    return nd_mbase_offset_term_auth_timeout();
}

uint32_t *nd_mb_i_prot_ptr(void *instance)
{
    return &nd_mbase_I_Prot(mb(instance));
}

uint16_t *nd_mb_prot_time_ptr(void *instance)
{
    return &nd_mbase_Prot_time(mb(instance));
}

uint32_t *nd_mb_alarmmin_ptr(void *instance)
{
    return &nd_mbase_alarmmin(mb(instance));
}

uint16_t *nd_mb_cmd_dostate_ptr(void *instance)
{
    return &nd_mbase_cmd_dostate(mb(instance));
}

uint16_t *nd_mb_cmd_errstate_ptr(void *instance)
{
    return &nd_mbase_cmd_errstate(mb(instance));
}

int32_t *nd_mb_current_l_ptr(void *instance)
{
    return &nd_mbase_current_l(mb(instance));
}

uint32_t *nd_mb_imaxmin_ptr(void *instance)
{
    return &nd_mbase_imaxmin(mb(instance));
}

uint32_t *nd_mb_limitmin_ptr(void *instance)
{
    return &nd_mbase_limitmin(mb(instance));
}

uint16_t *nd_mb_metertype_ptr(void *instance)
{
    return &nd_mbase_metertype(mb(instance));
}

uint8_t *nd_mb_relay_do_ptr(void *instance)
{
    return &nd_mbase_relay_do(mb(instance));
}

uint8_t *nd_mb_sta_ptr(void *instance)
{
    return &nd_mbase_sta(mb(instance));
}

uint32_t *nd_mb_over_current_threshold_ptr(void *instance)
{
    return &nd_mbase_OverCurrent_threshold(mb(instance));
}

uint8_t *nd_mb_zero_line_errs_duration_para_ptr(void *instance)
{
    return &nd_mbase_ZeroLineErrS_duration_para(mb(instance));
}

uint32_t nd_mb_offset_over_current_threshold(void)
{
    return nd_mbase_offset_OverCurrent_threshold();
}

uint32_t nd_mb_offset_zero_line_errs_duration_para(void)
{
    return nd_mbase_offset_ZeroLineErrS_duration_para();
}

uint32_t nd_mb_offset_report_2015mode(void)
{
    return nd_mbase_offset_report_2015mode();
}

uint32_t *nd_mb_month_energy_ptr(void *instance)
{
    return &nd_mbase_MonthEnegery(mb(instance));
}

int32_t *nd_mb_active_avg_power_ptr(void *instance)
{
    return &nd_mbase_active_avg_power(mb(instance));
}

int32_t *nd_mb_active_power_ptr(void *instance)
{
    return &nd_mbase_active_power(mb(instance));
}

uint32_t *nd_mb_apparent_power_ptr(void *instance)
{
    return &nd_mbase_apparent_power(mb(instance));
}

int32_t *nd_mb_current_n_ptr(void *instance)
{
    return &nd_mbase_current_n(mb(instance));
}

uint32_t *nd_mb_frequency_ptr(void *instance)
{
    return &nd_mbase_frequency(mb(instance));
}

int32_t *nd_mb_power_factor_ptr(void *instance)
{
    return &nd_mbase_power_factor(mb(instance));
}

int32_t *nd_mb_temperature_ptr(void *instance)
{
    return &nd_mbase_temperature(mb(instance));
}

uint32_t *nd_mb_voltage_ac_ptr(void *instance)
{
    return &nd_mbase_voltage_ac(mb(instance));
}

uint8_t *nd_mb_relay_true_sta_ptr(void *instance)
{
    return &nd_mbase_relay_true_sta(mb(instance));
}

uint16_t *nd_mb_run_state_ptr(void *instance)
{
    return nd_mbase_run_state(mb(instance));
}

uint8_t *nd_mb_handler_code_ptr(void *instance)
{
    return nd_mbase_handler_code(mb(instance));
}

int64_t nd_mb_cal_active_energy(void *instance, uint8_t fee, uint8_t type)
{
    return Get_Cal_Active_Energy_T(instance, fee, type);
}

int64_t nd_mb_active_energy_tabc(void *instance, uint8_t phase, uint8_t fee,
                                 uint8_t type, uint8_t pn)
{
    return Get_Active_Energy_TABC(instance, phase, fee, type, pn);
}

uint32_t *nd_mb_cover_open_s_acc_time_ptr(void *instance)
{
    return &nd_mbase_CoverOpenS_acc_time(mb(instance));
}

uint8_t *nd_mb_cover_open_s_duration_para_ptr(void *instance)
{
    return &nd_mbase_CoverOpenS_duration_para(mb(instance));
}

uint32_t *nd_mb_emu_err_s_acc_time_ptr(void *instance)
{
    return &nd_mbase_EmuErrS_acc_time(mb(instance));
}

uint8_t *nd_mb_emu_err_s_duration_para_ptr(void *instance)
{
    return &nd_mbase_EmuErrS_duration_para(mb(instance));
}

uint32_t *nd_mb_over_current_as_acc_time_ptr(void *instance)
{
    return &nd_mbase_OverCurrentAS_acc_time(mb(instance));
}

uint8_t *nd_mb_over_current_duration_para_ptr(void *instance)
{
    return &nd_mbase_OverCurrent_duration_para(mb(instance));
}

uint32_t *nd_mb_power_abnormal_s_acc_time_ptr(void *instance)
{
    return &nd_mbase_PowerAbnormalS_acc_time(mb(instance));
}

uint32_t *nd_mb_power_off_s_acc_time_ptr(void *instance)
{
    return &nd_mbase_PowerOffS_acc_time(mb(instance));
}

uint8_t *nd_mb_power_off_s_duration_para_ptr(void *instance)
{
    return &nd_mbase_PowerOffS_duration_para(mb(instance));
}

uint8_t *nd_mb_rtcerr_s_duration_para_ptr(void *instance)
{
    return &nd_mbase_RTCErrS_duration_para(mb(instance));
}

uint8_t *nd_mb_safe_en_ptr(void *instance)
{
    return &nd_mbase_SafeEn(mb(instance));
}

uint32_t *nd_mb_switch_err_s_acc_time_ptr(void *instance)
{
    return &nd_mbase_SwitchErrS_acc_time(mb(instance));
}

uint8_t *nd_mb_switch_err_s_duration_para_ptr(void *instance)
{
    return &nd_mbase_SwitchErrS_duration_para(mb(instance));
}

uint32_t *nd_mb_zero_line_err_s_current_threshold_ptr(void *instance)
{
    return &nd_mbase_ZeroLineErrS_Current_threshold(mb(instance));
}

uint32_t *nd_mb_zero_line_err_s_acc_time_ptr(void *instance)
{
    return &nd_mbase_ZeroLineErrS_acc_time(mb(instance));
}

uint16_t *nd_mb_zero_line_err_s_rate_threshold_ptr(void *instance)
{
    return &nd_mbase_ZeroLineErrS_rate_threshold(mb(instance));
}

uint32_t *nd_mb_auth_timeout_645_ptr(void *instance)
{
    return &nd_mbase_auth_timeout_645(mb(instance));
}

uint32_t *nd_mb_meter_const_ptr(void *instance)
{
    return &nd_mbase_meter_const(mb(instance));
}

uint16_t *nd_mb_plc_baut_tick_ptr(void *instance)
{
    return &nd_mbase_plc_baut_tick(mb(instance));
}

uint8_t *nd_mb_rand8_645_ptr(void *instance)
{
    return nd_mbase_rand8_645(mb(instance));
}

uint8_t *nd_mb_report_mode_645_ptr(void *instance)
{
    return nd_mbase_report_mode_645(mb(instance));
}

uint32_t nd_mb_offset_persistent_section_start(void)
{
    return nd_mbase_offset_persistent_section_start();
}

uint32_t nd_mb_offset_sta(void)
{
    return nd_mbase_offset_sta();
}

uint32_t nd_mb_offset_tsa(void)
{
    return nd_mbase_offset_tsa();
}

uint16_t *nd_mb_ec_save_diff_ptr(void *instance)
{
    return &nd_mbase_EC_SaveDiff(mb(instance));
}

uint32_t *nd_mb_upgrade_check_flag_ptr(void *instance)
{
    return &nd_mbase_UpGrade_CheckFlag(mb(instance));
}

uint32_t *nd_mb_upgrade_max_block_count_ptr(void *instance)
{
    return &nd_mbase_UpGrade_max_block_count(mb(instance));
}

uint32_t *nd_mb_blt_alarm_ptr(void *instance)
{
    return &nd_mbase_blt_alarm(mb(instance));
}

uint32_t *nd_mb_frz_min_start_dlysec_ptr(void *instance)
{
    return &nd_mbase_frz_min_start_dlysec(mb(instance));
}

uint8_t *nd_mb_no_filt_ptr(void *instance)
{
    return &nd_mbase_no_filt(mb(instance));
}

uint8_t *nd_mb_report_stat_645_ptr(void *instance)
{
    return nd_mbase_report_stat_645(mb(instance));
}

uint8_t *nd_mb_report_stat_already_read645_ptr(void *instance)
{
    return &nd_mbase_report_stat_already_read645(mb(instance));
}

uint8_t *nd_mb_report_stat_read_cache645_ptr(void *instance)
{
    return nd_mbase_report_stat_read_cache645(mb(instance));
}

struct tm *nd_mb_v_powerdown_tm_ptr(void *instance)
{
    return &nd_mbase_v_powerdown_tm(mb(instance));
}

uint32_t nd_mb_offset_no_filt(void)
{
    return nd_mbase_offset_no_filt();
}

uint32_t nd_mb_offset_report_mode_645(void)
{
    return nd_mbase_offset_report_mode_645();
}

uint32_t nd_mb_offset_energy_point(void)
{
    return nd_mbase_offset_energy_point();
}

uint32_t nd_mb_offset_over_current_duration_para(void)
{
    return nd_mbase_offset_OverCurrent_duration_para();
}

uint32_t nd_mb_offset_power_point(void)
{
    return nd_mbase_offset_power_point();
}

uint32_t nd_mb_offset_active_report_sec_limit(void)
{
    return nd_mbase_offset_active_report_sec_limit();
}

uint32_t nd_mb_offset_lcd_12_mean(void)
{
    return nd_mbase_offset_lcd_12_mean();
}

uint8_t *nd_mb_meter_wr_set_time_flag_ptr(void *instance)
{
    return &nd_mbase_MeterWr_SetTime_flag(mb(instance));
}

uint16_t *nd_mb_month_energery_dec_ptr(void *instance)
{
    return &nd_mbase_MonthEnegery_dec(mb(instance));
}

uint8_t *nd_mb_disp_borrow_ptr(void *instance)
{
    return &nd_mbase_disp_borrow(mb(instance));
}

uint8_t *nd_mb_meterclear_ptr(void *instance)
{
    return &nd_mbase_meterclear(mb(instance));
}

uint8_t *nd_mb_pd_cover_end_record_flag_ptr(void *instance)
{
    return &nd_mbase_pd_cover_EndRecord_flag(mb(instance));
}


uint8_t *nd_mb_active_flag_ptr(void *instance)
{
    return nd_mbase_active_flag(mb(instance));
}

uint32_t *nd_mb_minute_freeze_seq_ptr(void *instance)
{
    return nd_mbase_minute_freeze_seq(mb(instance));
}

uint32_t nd_mb_offset_active_flag(void)
{
    return nd_mbase_offset_active_flag();
}

uint32_t nd_mb_offset_minute_freeze_seq(void)
{
    return nd_mbase_offset_minute_freeze_seq();
}

uint32_t nd_mb_offset_power_on_sec_accum(void)
{
    return nd_mbase_offset_power_on_sec_accum();
}

uint32_t nd_mb_offset_v_powerdown_tm(void)
{
    return nd_mbase_offset_v_powerdown_tm();
}

uint8_t *nd_mb_cover_open_s_duration_ptr(void *instance)
{
    return &nd_mbase_CoverOpenS_duration(mb(instance));
}

uint32_t *nd_mb_cover_open_s_time_low_power_cnt_ptr(void *instance)
{
    return &nd_mbase_CoverOpenS_time_low_power_cnt(mb(instance));
}

uint8_t *nd_mb_emu_err_s_duration_ptr(void *instance)
{
    return &nd_mbase_EmuErrS_duration(mb(instance));
}

uint8_t *nd_mb_over_current_as_duration_ptr(void *instance)
{
    return &nd_mbase_OverCurrentAS_duration(mb(instance));
}

uint8_t *nd_mb_power_abnormal_s_duration_ptr(void *instance)
{
    return &nd_mbase_PowerAbnormalS_duration(mb(instance));
}

uint32_t *nd_mb_power_abnormal_s_time_low_power_cnt_ptr(void *instance)
{
    return &nd_mbase_PowerAbnormalS_time_low_power_cnt(mb(instance));
}

uint8_t *nd_mb_power_off_s_duration_ptr(void *instance)
{
    return &nd_mbase_PowerOffS_duration(mb(instance));
}

uint32_t *nd_mb_power_off_s_time_low_power_cnt_ptr(void *instance)
{
    return &nd_mbase_PowerOffS_time_low_power_cnt(mb(instance));
}

uint8_t *nd_mb_rtcerr_s_duration_ptr(void *instance)
{
    return &nd_mbase_RTCErrS_duration(mb(instance));
}

uint8_t *nd_mb_switch_err_s_duration_ptr(void *instance)
{
    return &nd_mbase_SwitchErrS_duration(mb(instance));
}

uint8_t *nd_mb_zero_line_err_s_duration_ptr(void *instance)
{
    return &nd_mbase_ZeroLineErrS_duration(mb(instance));
}

uint8_t *nd_mb_pd_active_flag_update_ptr(void *instance)
{
    return &nd_mbase_pd_active_flag_update(mb(instance));
}

uint8_t *nd_mb_record_flag_ptr(void *instance)
{
    return nd_mbase_record_flag(mb(instance));
}

struct tm *nd_mb_powerdown_tm_ptr(void *instance)
{
    return &nd_mbase_powerdown_tm(mb(instance));
}

uint16_t *nd_mb_report_reset_sec_cnt_645_ptr(void *instance)
{
    return &nd_mbase_report_reset_sec_cnt_645(mb(instance));
}

uint8_t *nd_mb_disp12_ptr(void *instance)
{
    return &nd_mbase_Disp12(mb(instance));
}

uint8_t *nd_mb_key_byte_ptr(void *instance)
{
    return &nd_mbase_Key(mb(instance));
}

uint8_t *nd_mb_key_inv_ptr(void *instance)
{
    return &nd_mbase_KeyInv(mb(instance));
}

uint8_t *nd_mb_tou_no_ptr(void *instance)
{
    return &nd_mbase_TOUNo(mb(instance));
}

uint8_t *nd_mb_tou_price_table_ptr(void *instance)
{
    return &nd_mbase_TouPriceTable(mb(instance));
}



uint8_t *nd_mb_power_dir_ptr(void *instance)
{
    return &nd_mbase_power_dir(mb(instance));
}

uint8_t *nd_mb_rtcbat_under_v_state_ptr(void *instance)
{
    return &nd_mbase_RtcBat_UnderV_state(mb(instance));
}

uint8_t *nd_mb_day_table_no_ptr(void *instance)
{
    return &nd_mbase_DayTableNo(mb(instance));
}

uint8_t *nd_mb_port_tou_cut_ptr(void *instance)
{
    return &nd_mbase_PortTouCut(mb(instance));
}

uint8_t *nd_mb_tou_bef_no_ptr(void *instance)
{
    return &nd_mbase_TouBefNo(mb(instance));
}

uint32_t *nd_mb_energy_chksum_ptr(void *instance)
{
    return &nd_mbase_Energy_chksum(mb(instance));
}

uint32_t *nd_mb_use_energy_chksum_ptr(void *instance)
{
    return &nd_mbase_UseEnergy_chksum(mb(instance));
}

uint16_t *nd_mb_ec_save_success_ptr(void *instance)
{
    return &nd_mbase_ec_save_success(mb(instance));
}

uint8_t *nd_mb_n_pluse_ptr(void *instance)
{
    return &nd_mbase_n_pluse(mb(instance));
}

uint8_t *nd_mb_p_pluse_ptr(void *instance)
{
    return &nd_mbase_p_pluse(mb(instance));
}

uint8_t *nd_mb_record_rollback_flag_ptr(void *instance)
{
    return &nd_mbase_record_rollback_flag(mb(instance));
}

uint8_t *nd_mb_emu_err_s_status_ptr(void *instance)
{
    return &nd_mbase_EmuErrS_status(mb(instance));
}

uint8_t *nd_mb_avg_cal_start_flag_ptr(void *instance)
{
    return &nd_mbase_avg_cal_start_flag(mb(instance));
}

uint32_t *nd_mb_curt_33_ptr(void *instance)
{
    return &nd_mbase_curt_33(mb(instance));
}

uint8_t *nd_mb_meter_start_cnt_ptr(void *instance)
{
    return &nd_mbase_meter_start_cnt(mb(instance));
}

uint32_t *nd_mb_power_33_ptr(void *instance)
{
    return &nd_mbase_power_33(mb(instance));
}

uint32_t *nd_mb_volt_120_ptr(void *instance)
{
    return &nd_mbase_volt_120(mb(instance));
}

uint32_t *nd_mb_volt_60_ptr(void *instance)
{
    return &nd_mbase_volt_60(mb(instance));
}

uint32_t *nd_mb_volt_78_ptr(void *instance)
{
    return &nd_mbase_volt_78(mb(instance));
}

uint32_t *nd_mb_active_n_ptr(void *instance)
{
    return nd_mbase_active_n(mb(instance));
}

uint16_t *nd_mb_active_n_dec_ptr(void *instance)
{
    return nd_mbase_active_n_dec(mb(instance));
}

uint32_t *nd_mb_active_p_ptr(void *instance)
{
    return nd_mbase_active_p(mb(instance));
}

uint16_t *nd_mb_active_p_dec_ptr(void *instance)
{
    return nd_mbase_active_p_dec(mb(instance));
}

nd_energy_ledger_t *nd_mb_energy_ledger(void *instance)
{
    return metering_energy_ledger_from_mbase(mb(instance));
}

uint32_t nd_mb_offset_record_rollback_flag(void)
{
    return nd_mbase_offset_record_rollback_flag();
}

uint8_t *nd_mb_frz_replenish_end_ptr(void *instance)
{
    return &nd_mbase_frz_replenish_end(mb(instance));
}
