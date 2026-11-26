#include "targets/stategrid_meter/app/meter_base_port.h"

#include "targets/stategrid_meter/app/meter_wire_status.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/persistence/meter_base_view.h"

static int32_t esam_chip_abnormal(void *ctx)
{
    return nd_wire_status_to_semantic(*nd_mb_esam_chip_abnormal_ptr(ctx));
}

static int32_t power_state(void *ctx)
{
    return nd_wire_power_to_semantic(
        ((stategrid_context_t *)ctx)->ac_power_status);
}

static const nd_meter_base_port_t meter_base_port = {
    NULL,
    nd_mb_active_report_cmdtype_ptr,
    nd_mb_active_report_retry_cnt_ptr,
    nd_mb_active_report_retry_dly_ptr,
    nd_mb_active_report_sec_limit_ptr,
    nd_mb_auth_on_ptr,
    nd_mb_backlight_tmr_ptr,
    nd_mb_clientaddr_ptr,
    nd_mb_commdisp_ir485_ptr,
    nd_mb_commdisp_plc_ptr,
    nd_mb_connect_status_ptr,
    nd_mb_connect_timeout_ptr,
    nd_mb_connect_timeout_cnt_ptr,
    nd_mb_current_connect_channel_ptr,
    esam_chip_abnormal,
    nd_mb_esam_connect_timeout_ptr,
    nd_mb_factor_mode_ptr,
    nd_mb_infrared_auth_timeout_ptr,
    nd_mb_key_state_ptr,
    nd_mb_lcd_autodis_keep_sec_ptr,
    nd_mb_lcd_autodis_list_num_ptr,
    nd_mb_mac_suspend_state_ptr,
    nd_mb_power_on_sec_accum_ptr,
    nd_mb_poweron_active_report_dly_ptr,
    nd_mb_report_2015_cache_ptr,
    nd_mb_report_2015_mode_ptr,
    nd_mb_report_2015_state_ptr,
    nd_mb_report_piid_ptr,
    nd_mb_report_random_data_ptr,
    nd_mb_save_active_report_state,
    nd_mb_save_report_2015_state,
    nd_mb_term_auth_timeout_ptr,
    nd_mb_tsa_ptr,
    nd_mb_valid,
    nd_mb_voltage_battery_ptr,
    power_state
};

void stategrid_meter_base_attach(stategrid_context_t *instance)
{
    if (instance != NULL) {
        instance->meter_base = &meter_base_port;
    }
}

const nd_meter_base_port_t *nd_meter_base_get(void *instance)
{
    return stategrid_meter_base_port(instance);
}
