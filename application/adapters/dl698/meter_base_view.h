#ifndef ND_APPLICATION_ADAPTERS_DL698_METER_BASE_VIEW_H
#define ND_APPLICATION_ADAPTERS_DL698_METER_BASE_VIEW_H

#include <stdint.h>

struct instance_t;

int nd_mb_valid(struct instance_t *instance);

uint32_t *nd_mb_report_2015_state_ptr(struct instance_t *instance);
#define nd_mb_report_2015_state(i) (nd_mb_report_2015_state_ptr(i))

uint32_t *nd_mb_report_2015_cache_ptr(struct instance_t *instance);
#define nd_mb_report_2015_cache(i) (*nd_mb_report_2015_cache_ptr(i))

uint16_t *nd_mb_active_report_cmdtype_ptr(struct instance_t *instance);
#define nd_mb_active_report_cmdtype(i) (*nd_mb_active_report_cmdtype_ptr(i))

uint8_t *nd_mb_active_report_retry_cnt_ptr(struct instance_t *instance);
#define nd_mb_active_report_retry_cnt(i) (nd_mb_active_report_retry_cnt_ptr(i))

uint8_t *nd_mb_active_report_retry_dly_ptr(struct instance_t *instance);
#define nd_mb_active_report_retry_dly(i) (nd_mb_active_report_retry_dly_ptr(i))

uint8_t *nd_mb_report_piid_ptr(struct instance_t *instance);
#define nd_mb_report_piid(i) (*nd_mb_report_piid_ptr(i))

uint8_t *nd_mb_report_random_data_ptr(struct instance_t *instance);
#define nd_mb_report_random_data(i) (nd_mb_report_random_data_ptr(i))

uint8_t *nd_mb_switch_on_success_ptr(struct instance_t *instance);
#define nd_mb_switch_on_success(i) (*nd_mb_switch_on_success_ptr(i))

uint8_t *nd_mb_switch_off_success_ptr(struct instance_t *instance);
#define nd_mb_switch_off_success(i) (*nd_mb_switch_off_success_ptr(i))

uint8_t *nd_mb_esam_chip_abnormal_ptr(struct instance_t *instance);
#define nd_mb_esam_chip_abnormal(i) (*nd_mb_esam_chip_abnormal_ptr(i))

uint8_t *nd_mb_auth_on_ptr(struct instance_t *instance);
#define nd_mb_auth_on(i) (*nd_mb_auth_on_ptr(i))

uint32_t *nd_mb_report_2015_mode_ptr(struct instance_t *instance);
#define nd_mb_report_2015_mode(i) (*nd_mb_report_2015_mode_ptr(i))

uint8_t *nd_mb_poweron_active_report_dly_ptr(struct instance_t *instance);
#define nd_mb_poweron_active_report_dly(i) (*nd_mb_poweron_active_report_dly_ptr(i))

uint32_t *nd_mb_power_on_sec_accum_ptr(struct instance_t *instance);
#define nd_mb_power_on_sec_accum(i) (*nd_mb_power_on_sec_accum_ptr(i))

uint16_t *nd_mb_active_report_sec_limit_ptr(struct instance_t *instance);
#define nd_mb_active_report_sec_limit(i) (*nd_mb_active_report_sec_limit_ptr(i))

uint8_t *nd_mb_key_state_ptr(struct instance_t *instance);
#define nd_mb_key_state(i) (*nd_mb_key_state_ptr(i))

uint32_t *nd_mb_voltage_battery_ptr(struct instance_t *instance);
#define nd_mb_voltage_battery(i) (*nd_mb_voltage_battery_ptr(i))

uint8_t *nd_mb_tsa_ptr(struct instance_t *instance);
#define nd_mb_tsa(i) (nd_mb_tsa_ptr(i))

uint32_t (*nd_mb_connect_timeout_ptr(struct instance_t *instance))[2];
#define nd_mb_connect_timeout(i) (nd_mb_connect_timeout_ptr(i))

uint32_t (*nd_mb_esam_connect_timeout_ptr(struct instance_t *instance))[2];
#define nd_mb_esam_connect_timeout(i) (nd_mb_esam_connect_timeout_ptr(i))

uint8_t (*nd_mb_connect_status_ptr(struct instance_t *instance))[2];
#define nd_mb_connect_status(i) (nd_mb_connect_status_ptr(i))

uint32_t (*nd_mb_connect_timeout_cnt_ptr(struct instance_t *instance))[2];
#define nd_mb_connect_timeout_cnt(i) (nd_mb_connect_timeout_cnt_ptr(i))

uint8_t *nd_mb_current_connect_channel_ptr(struct instance_t *instance);
#define nd_mb_current_connect_channel(i) (nd_mb_current_connect_channel_ptr(i))

uint8_t *nd_mb_clientaddr_ptr(struct instance_t *instance);
#define nd_mb_clientaddr(i) (nd_mb_clientaddr_ptr(i))

uint32_t *nd_mb_term_auth_timeout_ptr(struct instance_t *instance);
#define nd_mb_term_auth_timeout(i) (*nd_mb_term_auth_timeout_ptr(i))

uint32_t *nd_mb_infrared_auth_timeout_ptr(struct instance_t *instance);
#define nd_mb_infrared_auth_timeout(i) (*nd_mb_infrared_auth_timeout_ptr(i))

uint8_t *nd_mb_mac_suspend_state_ptr(struct instance_t *instance);
#define nd_mb_mac_suspend_state(i) (*nd_mb_mac_suspend_state_ptr(i))

uint8_t *nd_mb_factor_mode_ptr(struct instance_t *instance);
#define nd_mb_factor_mode(i) (*nd_mb_factor_mode_ptr(i))

uint8_t *nd_mb_lcd_autodis_list_num_ptr(struct instance_t *instance);
#define nd_mb_lcd_autodis_list_num(i) (*nd_mb_lcd_autodis_list_num_ptr(i))

uint16_t *nd_mb_lcd_autodis_keep_sec_ptr(struct instance_t *instance);
#define nd_mb_lcd_autodis_keep_sec(i) (*nd_mb_lcd_autodis_keep_sec_ptr(i))

uint8_t *nd_mb_commdisp_plc_ptr(struct instance_t *instance);
#define nd_mb_commdisp_plc(i) (*nd_mb_commdisp_plc_ptr(i))

uint8_t *nd_mb_commdisp_ir485_ptr(struct instance_t *instance);
#define nd_mb_commdisp_ir485(i) (*nd_mb_commdisp_ir485_ptr(i))

uint16_t *nd_mb_backlight_tmr_ptr(struct instance_t *instance);
#define nd_mb_backlight_tmr(i) (*nd_mb_backlight_tmr_ptr(i))

void nd_mb_save_report_2015_state(struct instance_t *instance);
void nd_mb_save_active_report_state(struct instance_t *instance);

#endif
