#ifndef ND_APPLICATION_ADAPTERS_DL698_METER_BASE_VIEW_H
#define ND_APPLICATION_ADAPTERS_DL698_METER_BASE_VIEW_H

#include <stdint.h>

int nd_mb_valid(void *instance);

uint32_t *nd_mb_report_2015_state_ptr(void *instance);
#define nd_mb_report_2015_state(i) (nd_mb_report_2015_state_ptr(i))

uint32_t *nd_mb_report_2015_cache_ptr(void *instance);
#define nd_mb_report_2015_cache(i) (*nd_mb_report_2015_cache_ptr(i))

uint16_t *nd_mb_active_report_cmdtype_ptr(void *instance);
#define nd_mb_active_report_cmdtype(i) (*nd_mb_active_report_cmdtype_ptr(i))

uint8_t *nd_mb_active_report_retry_cnt_ptr(void *instance);
#define nd_mb_active_report_retry_cnt(i) (nd_mb_active_report_retry_cnt_ptr(i))

uint8_t *nd_mb_active_report_retry_dly_ptr(void *instance);
#define nd_mb_active_report_retry_dly(i) (nd_mb_active_report_retry_dly_ptr(i))

uint8_t *nd_mb_report_piid_ptr(void *instance);
#define nd_mb_report_piid(i) (*nd_mb_report_piid_ptr(i))

uint8_t *nd_mb_report_random_data_ptr(void *instance);
#define nd_mb_report_random_data(i) (nd_mb_report_random_data_ptr(i))

uint8_t *nd_mb_switch_on_success_ptr(void *instance);
#define nd_mb_switch_on_success(i) (*nd_mb_switch_on_success_ptr(i))

uint8_t *nd_mb_switch_off_success_ptr(void *instance);
#define nd_mb_switch_off_success(i) (*nd_mb_switch_off_success_ptr(i))

uint8_t *nd_mb_esam_chip_abnormal_ptr(void *instance);
#define nd_mb_esam_chip_abnormal(i) (*nd_mb_esam_chip_abnormal_ptr(i))

uint8_t *nd_mb_auth_on_ptr(void *instance);
#define nd_mb_auth_on(i) (*nd_mb_auth_on_ptr(i))

uint32_t *nd_mb_report_2015_mode_ptr(void *instance);
#define nd_mb_report_2015_mode(i) (*nd_mb_report_2015_mode_ptr(i))

uint8_t *nd_mb_poweron_active_report_dly_ptr(void *instance);
#define nd_mb_poweron_active_report_dly(i) (*nd_mb_poweron_active_report_dly_ptr(i))

uint32_t *nd_mb_power_on_sec_accum_ptr(void *instance);
#define nd_mb_power_on_sec_accum(i) (*nd_mb_power_on_sec_accum_ptr(i))

uint16_t *nd_mb_active_report_sec_limit_ptr(void *instance);
#define nd_mb_active_report_sec_limit(i) (*nd_mb_active_report_sec_limit_ptr(i))

uint8_t *nd_mb_key_state_ptr(void *instance);
#define nd_mb_key_state(i) (*nd_mb_key_state_ptr(i))

uint32_t *nd_mb_voltage_battery_ptr(void *instance);
#define nd_mb_voltage_battery(i) (*nd_mb_voltage_battery_ptr(i))

uint8_t *nd_mb_tsa_ptr(void *instance);
#define nd_mb_tsa(i) (nd_mb_tsa_ptr(i))

uint32_t (*nd_mb_connect_timeout_ptr(void *instance))[2];
#define nd_mb_connect_timeout(i) (nd_mb_connect_timeout_ptr(i))

uint32_t (*nd_mb_esam_connect_timeout_ptr(void *instance))[2];
#define nd_mb_esam_connect_timeout(i) (nd_mb_esam_connect_timeout_ptr(i))

uint8_t (*nd_mb_connect_status_ptr(void *instance))[2];
#define nd_mb_connect_status(i) (nd_mb_connect_status_ptr(i))

uint32_t (*nd_mb_connect_timeout_cnt_ptr(void *instance))[2];
#define nd_mb_connect_timeout_cnt(i) (nd_mb_connect_timeout_cnt_ptr(i))

uint8_t *nd_mb_current_connect_channel_ptr(void *instance);
#define nd_mb_current_connect_channel(i) (nd_mb_current_connect_channel_ptr(i))

uint8_t *nd_mb_clientaddr_ptr(void *instance);
#define nd_mb_clientaddr(i) (nd_mb_clientaddr_ptr(i))

uint32_t *nd_mb_term_auth_timeout_ptr(void *instance);
#define nd_mb_term_auth_timeout(i) (*nd_mb_term_auth_timeout_ptr(i))

uint32_t *nd_mb_infrared_auth_timeout_ptr(void *instance);
#define nd_mb_infrared_auth_timeout(i) (*nd_mb_infrared_auth_timeout_ptr(i))

uint8_t *nd_mb_mac_suspend_state_ptr(void *instance);
#define nd_mb_mac_suspend_state(i) (*nd_mb_mac_suspend_state_ptr(i))

uint8_t *nd_mb_factor_mode_ptr(void *instance);
#define nd_mb_factor_mode(i) (*nd_mb_factor_mode_ptr(i))

uint8_t *nd_mb_lcd_autodis_list_num_ptr(void *instance);
#define nd_mb_lcd_autodis_list_num(i) (*nd_mb_lcd_autodis_list_num_ptr(i))

uint16_t *nd_mb_lcd_autodis_keep_sec_ptr(void *instance);
#define nd_mb_lcd_autodis_keep_sec(i) (*nd_mb_lcd_autodis_keep_sec_ptr(i))

uint8_t *nd_mb_commdisp_plc_ptr(void *instance);
#define nd_mb_commdisp_plc(i) (*nd_mb_commdisp_plc_ptr(i))

uint8_t *nd_mb_commdisp_ir485_ptr(void *instance);
#define nd_mb_commdisp_ir485(i) (*nd_mb_commdisp_ir485_ptr(i))

uint16_t *nd_mb_backlight_tmr_ptr(void *instance);
#define nd_mb_backlight_tmr(i) (*nd_mb_backlight_tmr_ptr(i))

void nd_mb_save_report_2015_state(void *instance);
void nd_mb_save_active_report_state(void *instance);

uint8_t *nd_mb_boardadjtime_ptr(void *instance);
#define nd_mb_boardadjtime(i) (*nd_mb_boardadjtime_ptr(i))

uint8_t *nd_mb_flag_event_clear_ptr(void *instance);
#define nd_mb_flag_event_clear(i) (*nd_mb_flag_event_clear_ptr(i))

uint16_t *nd_mb_limit_t_max_ptr(void *instance);
#define nd_mb_limit_t_max(i) (*nd_mb_limit_t_max_ptr(i))

uint16_t *nd_mb_limit_t_min_ptr(void *instance);
#define nd_mb_limit_t_min(i) (*nd_mb_limit_t_min_ptr(i))

uint8_t *nd_mb_rtc_errs_status_ptr(void *instance);
#define nd_mb_rtc_errs_status(i) (*nd_mb_rtc_errs_status_ptr(i))

uint16_t *nd_mb_trigger_insfreeze_delay_sec_ptr(void *instance);
#define nd_mb_trigger_insfreeze_delay_sec(i) (*nd_mb_trigger_insfreeze_delay_sec_ptr(i))

uint8_t *nd_mb_fee_ptr(void *instance);
#define nd_mb_fee(i) (*nd_mb_fee_ptr(i))

uint8_t *nd_mb_active_energy_cal_word_ptr(void *instance);
#define nd_mb_active_energy_cal_word(i) (*nd_mb_active_energy_cal_word_ptr(i))

uint16_t *nd_mb_backlight_key_time_ptr(void *instance);
#define nd_mb_backlight_key_time(i) (*nd_mb_backlight_key_time_ptr(i))

uint8_t *nd_mb_energy_point_ptr(void *instance);
#define nd_mb_energy_point(i) (*nd_mb_energy_point_ptr(i))

uint8_t *nd_mb_lcd_12_mean_ptr(void *instance);
#define nd_mb_lcd_12_mean(i) (*nd_mb_lcd_12_mean_ptr(i))

uint16_t *nd_mb_lcd_checkdis_keep_sec_ptr(void *instance);
#define nd_mb_lcd_checkdis_keep_sec(i) (*nd_mb_lcd_checkdis_keep_sec_ptr(i))

uint16_t *nd_mb_lcd_nopower_key_keep_sec_ptr(void *instance);
#define nd_mb_lcd_nopower_key_keep_sec(i) (*nd_mb_lcd_nopower_key_keep_sec_ptr(i))

uint8_t *nd_mb_lcd_poweron_all_dis_time_ptr(void *instance);
#define nd_mb_lcd_poweron_all_dis_time(i) (*nd_mb_lcd_poweron_all_dis_time_ptr(i))

uint8_t *nd_mb_meter_no_ptr(void *instance);
#define nd_mb_meter_no(i) (nd_mb_meter_no_ptr(i))

uint8_t *nd_mb_power_point_ptr(void *instance);
#define nd_mb_power_point(i) (*nd_mb_power_point_ptr(i))

uint8_t *nd_mb_first_mbill_day_flag_ptr(void *instance);
#define nd_mb_first_mbill_day_flag(i) (*nd_mb_first_mbill_day_flag_ptr(i))

uint8_t *nd_mb_password_err_cnt_ptr(void *instance);
#define nd_mb_password_err_cnt(i) (*nd_mb_password_err_cnt_ptr(i))

uint8_t *nd_mb_cover_open_s_status_ptr(void *instance);
#define nd_mb_cover_open_s_status(i) (*nd_mb_cover_open_s_status_ptr(i))

uint8_t *nd_mb_lcd_autodis_list_maxnum_ptr(void *instance);
#define nd_mb_lcd_autodis_list_maxnum(i) (*nd_mb_lcd_autodis_list_maxnum_ptr(i))

uint16_t *nd_mb_lcd_keydis_keep_sec_ptr(void *instance);
#define nd_mb_lcd_keydis_keep_sec(i) (*nd_mb_lcd_keydis_keep_sec_ptr(i))

uint8_t *nd_mb_lcd_keydis_list_maxnum_ptr(void *instance);
#define nd_mb_lcd_keydis_list_maxnum(i) (*nd_mb_lcd_keydis_list_maxnum_ptr(i))

uint8_t *nd_mb_lcd_keydis_list_num_ptr(void *instance);
#define nd_mb_lcd_keydis_list_num(i) (*nd_mb_lcd_keydis_list_num_ptr(i))

uint16_t *nd_mb_auto_inv_ptr(void *instance);
#define nd_mb_auto_inv(i) (*nd_mb_auto_inv_ptr(i))

uint8_t *nd_mb_auto_ptr_ptr(void *instance);
#define nd_mb_auto_ptr(i) (*nd_mb_auto_ptr_ptr(i))

uint8_t *nd_mb_baut_plc_ptr(void *instance);
#define nd_mb_baut_plc(i) (*nd_mb_baut_plc_ptr(i))

uint8_t *nd_mb_baut_plc_neg_ptr(void *instance);
#define nd_mb_baut_plc_neg(i) (*nd_mb_baut_plc_neg_ptr(i))

uint8_t *nd_mb_baut_rs485_ptr(void *instance);
#define nd_mb_baut_rs485(i) (*nd_mb_baut_rs485_ptr(i))

uint8_t *nd_mb_key_byte_ptr(void *instance);
#define nd_mb_key_byte(i) (*nd_mb_key_byte_ptr(i))

uint8_t *nd_mb_mul_port_mode_ptr(void *instance);
#define nd_mb_mul_port_mode(i) (*nd_mb_mul_port_mode_ptr(i))

uint16_t *nd_mb_page_id_ptr(void *instance);
#define nd_mb_page_id(i) (*nd_mb_page_id_ptr(i))

uint8_t *nd_mb_baud_chg_ptr(void *instance);
#define nd_mb_baud_chg(i) (*nd_mb_baud_chg_ptr(i))

uint8_t *nd_mb_esam_power_close_dly_ptr(void *instance);
#define nd_mb_esam_power_close_dly(i) (*nd_mb_esam_power_close_dly_ptr(i))

uint8_t *nd_mb_relay_ctl_mode_ptr(void *instance);
#define nd_mb_relay_ctl_mode(i) (*nd_mb_relay_ctl_mode_ptr(i))

uint32_t nd_mb_offset_limit_t_max(void);

uint32_t nd_mb_offset_limit_t_min(void);

uint32_t nd_mb_offset_active_energy_cal_word(void);

uint32_t nd_mb_offset_lcd_autodis_list_num(void);

uint32_t nd_mb_offset_lcd_poweron_all_dis_time(void);


uint32_t nd_mb_offset_baut_rs485(void);

uint32_t nd_mb_offset_factor_mode(void);

uint32_t nd_mb_offset_lcd_autodis_keep_sec(void);

uint32_t nd_mb_offset_lcd_keydis_keep_sec(void);

uint32_t nd_mb_offset_lcd_keydis_list_num(void);

uint32_t nd_mb_offset_baut_plc(void);

uint32_t nd_mb_offset_meter_no(void);

uint32_t nd_mb_offset_relay_ctl_mode(void);
uint32_t nd_mb_offset_term_auth_timeout(void);

#define ND_MB_FLAG_PORT_SEC 0
#define ND_MB_FLAG_PORT_TOU_CUT 2

uint32_t *nd_mb_i_prot_ptr(void *instance);
#define nd_mb_i_prot(i) (*nd_mb_i_prot_ptr(i))

uint16_t *nd_mb_prot_time_ptr(void *instance);
#define nd_mb_prot_time(i) (*nd_mb_prot_time_ptr(i))

uint32_t *nd_mb_alarmmin_ptr(void *instance);
#define nd_mb_alarmmin(i) (*nd_mb_alarmmin_ptr(i))

uint16_t *nd_mb_cmd_dostate_ptr(void *instance);
#define nd_mb_cmd_dostate(i) (*nd_mb_cmd_dostate_ptr(i))

uint16_t *nd_mb_cmd_errstate_ptr(void *instance);
#define nd_mb_cmd_errstate(i) (*nd_mb_cmd_errstate_ptr(i))

int32_t *nd_mb_current_l_ptr(void *instance);
#define nd_mb_current_l(i) (*nd_mb_current_l_ptr(i))

uint32_t *nd_mb_imaxmin_ptr(void *instance);
#define nd_mb_imaxmin(i) (*nd_mb_imaxmin_ptr(i))

uint32_t *nd_mb_limitmin_ptr(void *instance);
#define nd_mb_limitmin(i) (*nd_mb_limitmin_ptr(i))

uint16_t *nd_mb_metertype_ptr(void *instance);
#define nd_mb_metertype(i) (*nd_mb_metertype_ptr(i))

uint8_t *nd_mb_relay_do_ptr(void *instance);
#define nd_mb_relay_do(i) (*nd_mb_relay_do_ptr(i))

uint8_t *nd_mb_sta_ptr(void *instance);
#define nd_mb_sta(i) (*nd_mb_sta_ptr(i))

uint32_t *nd_mb_over_current_threshold_ptr(void *instance);
#define nd_mb_over_current_threshold(i) (*nd_mb_over_current_threshold_ptr(i))

uint8_t *nd_mb_zero_line_errs_duration_para_ptr(void *instance);
#define nd_mb_zero_line_errs_duration_para(i) (*nd_mb_zero_line_errs_duration_para_ptr(i))

uint32_t nd_mb_offset_over_current_threshold(void);

uint32_t nd_mb_offset_zero_line_errs_duration_para(void);

uint32_t nd_mb_offset_report_2015mode(void);

uint32_t *nd_mb_month_energy_ptr(void *instance);
#define nd_mb_month_energy(i) (*nd_mb_month_energy_ptr(i))

int32_t *nd_mb_active_avg_power_ptr(void *instance);
#define nd_mb_active_avg_power(i) (*nd_mb_active_avg_power_ptr(i))

int32_t *nd_mb_active_power_ptr(void *instance);
#define nd_mb_active_power(i) (*nd_mb_active_power_ptr(i))

uint32_t *nd_mb_apparent_power_ptr(void *instance);
#define nd_mb_apparent_power(i) (*nd_mb_apparent_power_ptr(i))

int32_t *nd_mb_current_n_ptr(void *instance);
#define nd_mb_current_n(i) (*nd_mb_current_n_ptr(i))

uint32_t *nd_mb_frequency_ptr(void *instance);
#define nd_mb_frequency(i) (*nd_mb_frequency_ptr(i))

int32_t *nd_mb_power_factor_ptr(void *instance);
#define nd_mb_power_factor(i) (*nd_mb_power_factor_ptr(i))

int32_t *nd_mb_temperature_ptr(void *instance);
#define nd_mb_temperature(i) (*nd_mb_temperature_ptr(i))

uint32_t *nd_mb_voltage_ac_ptr(void *instance);
#define nd_mb_voltage_ac(i) (*nd_mb_voltage_ac_ptr(i))

uint8_t *nd_mb_relay_true_sta_ptr(void *instance);
#define nd_mb_relay_true_sta(i) (*nd_mb_relay_true_sta_ptr(i))

uint16_t *nd_mb_run_state_ptr(void *instance);
#define nd_mb_run_state(i) (nd_mb_run_state_ptr(i))

uint8_t *nd_mb_handler_code_ptr(void *instance);
#define nd_mb_handler_code(i) (nd_mb_handler_code_ptr(i))

int64_t nd_mb_cal_active_energy(void *instance, uint8_t fee, uint8_t type);
int64_t nd_mb_active_energy_tabc(void *instance, uint8_t phase, uint8_t fee,
                                 uint8_t type, uint8_t pn);

uint32_t *nd_mb_cover_open_s_acc_time_ptr(void *instance);
#define nd_mb_cover_open_s_acc_time(i) (*nd_mb_cover_open_s_acc_time_ptr(i))

uint8_t *nd_mb_cover_open_s_duration_para_ptr(void *instance);
#define nd_mb_cover_open_s_duration_para(i) (*nd_mb_cover_open_s_duration_para_ptr(i))

uint32_t *nd_mb_emu_err_s_acc_time_ptr(void *instance);
#define nd_mb_emu_err_s_acc_time(i) (*nd_mb_emu_err_s_acc_time_ptr(i))

uint8_t *nd_mb_emu_err_s_duration_para_ptr(void *instance);
#define nd_mb_emu_err_s_duration_para(i) (*nd_mb_emu_err_s_duration_para_ptr(i))

uint32_t *nd_mb_over_current_as_acc_time_ptr(void *instance);
#define nd_mb_over_current_as_acc_time(i) (*nd_mb_over_current_as_acc_time_ptr(i))

uint8_t *nd_mb_over_current_duration_para_ptr(void *instance);
#define nd_mb_over_current_duration_para(i) (*nd_mb_over_current_duration_para_ptr(i))

uint32_t *nd_mb_power_abnormal_s_acc_time_ptr(void *instance);
#define nd_mb_power_abnormal_s_acc_time(i) (*nd_mb_power_abnormal_s_acc_time_ptr(i))

uint32_t *nd_mb_power_off_s_acc_time_ptr(void *instance);
#define nd_mb_power_off_s_acc_time(i) (*nd_mb_power_off_s_acc_time_ptr(i))

uint8_t *nd_mb_power_off_s_duration_para_ptr(void *instance);
#define nd_mb_power_off_s_duration_para(i) (*nd_mb_power_off_s_duration_para_ptr(i))

uint8_t *nd_mb_rtcerr_s_duration_para_ptr(void *instance);
#define nd_mb_rtcerr_s_duration_para(i) (*nd_mb_rtcerr_s_duration_para_ptr(i))

uint8_t *nd_mb_safe_en_ptr(void *instance);
#define nd_mb_safe_en(i) (*nd_mb_safe_en_ptr(i))

uint32_t *nd_mb_switch_err_s_acc_time_ptr(void *instance);
#define nd_mb_switch_err_s_acc_time(i) (*nd_mb_switch_err_s_acc_time_ptr(i))

uint8_t *nd_mb_switch_err_s_duration_para_ptr(void *instance);
#define nd_mb_switch_err_s_duration_para(i) (*nd_mb_switch_err_s_duration_para_ptr(i))

uint32_t *nd_mb_zero_line_err_s_current_threshold_ptr(void *instance);
#define nd_mb_zero_line_err_s_current_threshold(i) (*nd_mb_zero_line_err_s_current_threshold_ptr(i))

uint32_t *nd_mb_zero_line_err_s_acc_time_ptr(void *instance);
#define nd_mb_zero_line_err_s_acc_time(i) (*nd_mb_zero_line_err_s_acc_time_ptr(i))

uint16_t *nd_mb_zero_line_err_s_rate_threshold_ptr(void *instance);
#define nd_mb_zero_line_err_s_rate_threshold(i) (*nd_mb_zero_line_err_s_rate_threshold_ptr(i))

uint32_t *nd_mb_auth_timeout_645_ptr(void *instance);
#define nd_mb_auth_timeout_645(i) (*nd_mb_auth_timeout_645_ptr(i))

uint32_t *nd_mb_meter_const_ptr(void *instance);
#define nd_mb_meter_const(i) (*nd_mb_meter_const_ptr(i))

uint16_t *nd_mb_plc_baut_tick_ptr(void *instance);
#define nd_mb_plc_baut_tick(i) (*nd_mb_plc_baut_tick_ptr(i))

uint8_t *nd_mb_rand8_645_ptr(void *instance);
#define nd_mb_rand8_645(i) (nd_mb_rand8_645_ptr(i))

uint8_t *nd_mb_report_mode_645_ptr(void *instance);
#define nd_mb_report_mode_645(i) (nd_mb_report_mode_645_ptr(i))

uint32_t nd_mb_offset_persistent_section_start(void);

uint32_t nd_mb_offset_sta(void);

uint32_t nd_mb_offset_tsa(void);

uint16_t *nd_mb_ec_save_diff_ptr(void *instance);
#define nd_mb_ec_save_diff(i) (*nd_mb_ec_save_diff_ptr(i))

uint32_t *nd_mb_upgrade_check_flag_ptr(void *instance);
#define nd_mb_upgrade_check_flag(i) (*nd_mb_upgrade_check_flag_ptr(i))

uint32_t *nd_mb_upgrade_max_block_count_ptr(void *instance);
#define nd_mb_upgrade_max_block_count(i) (*nd_mb_upgrade_max_block_count_ptr(i))

uint32_t *nd_mb_blt_alarm_ptr(void *instance);
#define nd_mb_blt_alarm(i) (*nd_mb_blt_alarm_ptr(i))

uint32_t *nd_mb_frz_min_start_dlysec_ptr(void *instance);
#define nd_mb_frz_min_start_dlysec(i) (*nd_mb_frz_min_start_dlysec_ptr(i))

uint8_t *nd_mb_no_filt_ptr(void *instance);
#define nd_mb_no_filt(i) (*nd_mb_no_filt_ptr(i))

uint8_t *nd_mb_report_stat_645_ptr(void *instance);
#define nd_mb_report_stat_645(i) (nd_mb_report_stat_645_ptr(i))

uint8_t *nd_mb_report_stat_already_read645_ptr(void *instance);
#define nd_mb_report_stat_already_read645(i) (*nd_mb_report_stat_already_read645_ptr(i))

uint8_t *nd_mb_report_stat_read_cache645_ptr(void *instance);
#define nd_mb_report_stat_read_cache645(i) (nd_mb_report_stat_read_cache645_ptr(i))

struct tm;
struct tm *nd_mb_v_powerdown_tm_ptr(void *instance);
#define nd_mb_v_powerdown_tm(i) (*nd_mb_v_powerdown_tm_ptr(i))

uint32_t nd_mb_offset_no_filt(void);
uint32_t nd_mb_offset_report_mode_645(void);

uint32_t nd_mb_offset_energy_point(void);
uint32_t nd_mb_offset_over_current_duration_para(void);
uint32_t nd_mb_offset_power_point(void);
uint32_t nd_mb_offset_active_report_sec_limit(void);
uint32_t nd_mb_offset_lcd_12_mean(void);

uint8_t *nd_mb_meter_wr_set_time_flag_ptr(void *instance);
#define nd_mb_meter_wr_set_time_flag(i) (*nd_mb_meter_wr_set_time_flag_ptr(i))

uint16_t *nd_mb_month_energery_dec_ptr(void *instance);
#define nd_mb_month_energery_dec(i) (*nd_mb_month_energery_dec_ptr(i))

uint8_t *nd_mb_disp_borrow_ptr(void *instance);
#define nd_mb_disp_borrow(i) (*nd_mb_disp_borrow_ptr(i))

uint8_t *nd_mb_meterclear_ptr(void *instance);
#define nd_mb_meterclear(i) (*nd_mb_meterclear_ptr(i))

uint8_t *nd_mb_pd_cover_end_record_flag_ptr(void *instance);
#define nd_mb_pd_cover_end_record_flag(i) (*nd_mb_pd_cover_end_record_flag_ptr(i))


uint8_t *nd_mb_active_flag_ptr(void *instance);
#define nd_mb_active_flag(i) (nd_mb_active_flag_ptr(i))

uint32_t *nd_mb_minute_freeze_seq_ptr(void *instance);
#define nd_mb_minute_freeze_seq(i) (nd_mb_minute_freeze_seq_ptr(i))

uint32_t nd_mb_offset_active_flag(void);

uint32_t nd_mb_offset_minute_freeze_seq(void);

uint32_t nd_mb_offset_power_on_sec_accum(void);

uint32_t nd_mb_offset_v_powerdown_tm(void);

uint8_t *nd_mb_cover_open_s_duration_ptr(void *instance);
#define nd_mb_cover_open_s_duration(i) (*nd_mb_cover_open_s_duration_ptr(i))

uint32_t *nd_mb_cover_open_s_time_low_power_cnt_ptr(void *instance);
#define nd_mb_cover_open_s_time_low_power_cnt(i) (*nd_mb_cover_open_s_time_low_power_cnt_ptr(i))

uint8_t *nd_mb_emu_err_s_duration_ptr(void *instance);
#define nd_mb_emu_err_s_duration(i) (*nd_mb_emu_err_s_duration_ptr(i))

uint8_t *nd_mb_over_current_as_duration_ptr(void *instance);
#define nd_mb_over_current_as_duration(i) (*nd_mb_over_current_as_duration_ptr(i))

uint8_t *nd_mb_power_abnormal_s_duration_ptr(void *instance);
#define nd_mb_power_abnormal_s_duration(i) (*nd_mb_power_abnormal_s_duration_ptr(i))

uint32_t *nd_mb_power_abnormal_s_time_low_power_cnt_ptr(void *instance);
#define nd_mb_power_abnormal_s_time_low_power_cnt(i) (*nd_mb_power_abnormal_s_time_low_power_cnt_ptr(i))

uint8_t *nd_mb_power_off_s_duration_ptr(void *instance);
#define nd_mb_power_off_s_duration(i) (*nd_mb_power_off_s_duration_ptr(i))

uint32_t *nd_mb_power_off_s_time_low_power_cnt_ptr(void *instance);
#define nd_mb_power_off_s_time_low_power_cnt(i) (*nd_mb_power_off_s_time_low_power_cnt_ptr(i))

uint8_t *nd_mb_rtcerr_s_duration_ptr(void *instance);
#define nd_mb_rtcerr_s_duration(i) (*nd_mb_rtcerr_s_duration_ptr(i))

uint8_t *nd_mb_switch_err_s_duration_ptr(void *instance);
#define nd_mb_switch_err_s_duration(i) (*nd_mb_switch_err_s_duration_ptr(i))

uint8_t *nd_mb_zero_line_err_s_duration_ptr(void *instance);
#define nd_mb_zero_line_err_s_duration(i) (*nd_mb_zero_line_err_s_duration_ptr(i))

uint8_t *nd_mb_pd_active_flag_update_ptr(void *instance);
#define nd_mb_pd_active_flag_update(i) (*nd_mb_pd_active_flag_update_ptr(i))

uint8_t *nd_mb_record_flag_ptr(void *instance);
#define nd_mb_record_flag(i) (nd_mb_record_flag_ptr(i))

struct tm *nd_mb_powerdown_tm_ptr(void *instance);
#define nd_mb_powerdown_tm(i) (*nd_mb_powerdown_tm_ptr(i))

uint16_t *nd_mb_report_reset_sec_cnt_645_ptr(void *instance);
#define nd_mb_report_reset_sec_cnt_645(i) (*nd_mb_report_reset_sec_cnt_645_ptr(i))

uint8_t *nd_mb_disp12_ptr(void *instance);
#define nd_mb_disp12(i) (*nd_mb_disp12_ptr(i))

uint8_t *nd_mb_key_ptr_ptr(void *instance);
#define nd_mb_key(i) (*nd_mb_key_ptr_ptr(i))

uint8_t *nd_mb_key_inv_ptr(void *instance);
#define nd_mb_key_inv(i) (*nd_mb_key_inv_ptr(i))

uint8_t *nd_mb_tou_no_ptr(void *instance);
#define nd_mb_tou_no(i) (*nd_mb_tou_no_ptr(i))

uint8_t *nd_mb_tou_price_table_ptr(void *instance);
#define nd_mb_tou_price_table(i) (*nd_mb_tou_price_table_ptr(i))



uint8_t *nd_mb_power_dir_ptr(void *instance);
#define nd_mb_power_dir(i) (*nd_mb_power_dir_ptr(i))

uint8_t *nd_mb_rtcbat_under_v_state_ptr(void *instance);
#define nd_mb_rtcbat_under_v_state(i) (*nd_mb_rtcbat_under_v_state_ptr(i))

uint8_t *nd_mb_day_table_no_ptr(void *instance);
#define nd_mb_day_table_no(i) (*nd_mb_day_table_no_ptr(i))

uint8_t *nd_mb_port_tou_cut_ptr(void *instance);
#define nd_mb_port_tou_cut(i) (*nd_mb_port_tou_cut_ptr(i))

uint8_t *nd_mb_tou_bef_no_ptr(void *instance);
#define nd_mb_tou_bef_no(i) (*nd_mb_tou_bef_no_ptr(i))

uint32_t *nd_mb_energy_chksum_ptr(void *instance);
#define nd_mb_energy_chksum(i) (*nd_mb_energy_chksum_ptr(i))

uint32_t *nd_mb_use_energy_chksum_ptr(void *instance);
#define nd_mb_use_energy_chksum(i) (*nd_mb_use_energy_chksum_ptr(i))

uint16_t *nd_mb_ec_save_success_ptr(void *instance);
#define nd_mb_ec_save_success(i) (*nd_mb_ec_save_success_ptr(i))

uint8_t *nd_mb_n_pluse_ptr(void *instance);
#define nd_mb_n_pluse(i) (*nd_mb_n_pluse_ptr(i))

uint8_t *nd_mb_p_pluse_ptr(void *instance);
#define nd_mb_p_pluse(i) (*nd_mb_p_pluse_ptr(i))

uint8_t *nd_mb_record_rollback_flag_ptr(void *instance);
#define nd_mb_record_rollback_flag(i) (*nd_mb_record_rollback_flag_ptr(i))

uint8_t *nd_mb_emu_err_s_status_ptr(void *instance);
#define nd_mb_emu_err_s_status(i) (*nd_mb_emu_err_s_status_ptr(i))

uint8_t *nd_mb_avg_cal_start_flag_ptr(void *instance);
#define nd_mb_avg_cal_start_flag(i) (*nd_mb_avg_cal_start_flag_ptr(i))

uint32_t *nd_mb_curt_33_ptr(void *instance);
#define nd_mb_curt_33(i) (*nd_mb_curt_33_ptr(i))

uint8_t *nd_mb_meter_start_cnt_ptr(void *instance);
#define nd_mb_meter_start_cnt(i) (*nd_mb_meter_start_cnt_ptr(i))

uint32_t *nd_mb_power_33_ptr(void *instance);
#define nd_mb_power_33(i) (*nd_mb_power_33_ptr(i))

uint32_t *nd_mb_volt_120_ptr(void *instance);
#define nd_mb_volt_120(i) (*nd_mb_volt_120_ptr(i))

uint32_t *nd_mb_volt_60_ptr(void *instance);
#define nd_mb_volt_60(i) (*nd_mb_volt_60_ptr(i))

uint32_t *nd_mb_volt_78_ptr(void *instance);
#define nd_mb_volt_78(i) (*nd_mb_volt_78_ptr(i))

uint32_t *nd_mb_active_n_ptr(void *instance);
#define nd_mb_active_n(i) (nd_mb_active_n_ptr(i))

uint16_t *nd_mb_active_n_dec_ptr(void *instance);
#define nd_mb_active_n_dec(i) (nd_mb_active_n_dec_ptr(i))

uint32_t *nd_mb_active_p_ptr(void *instance);
#define nd_mb_active_p(i) (nd_mb_active_p_ptr(i))

uint16_t *nd_mb_active_p_dec_ptr(void *instance);
#define nd_mb_active_p_dec(i) (nd_mb_active_p_dec_ptr(i))

struct nd_energy_ledger;
struct nd_energy_ledger *nd_mb_energy_ledger(void *instance);

uint32_t nd_mb_offset_record_rollback_flag(void);

uint8_t *nd_mb_frz_replenish_end_ptr(void *instance);
#define nd_mb_frz_replenish_end(i) (*nd_mb_frz_replenish_end_ptr(i))

#endif
