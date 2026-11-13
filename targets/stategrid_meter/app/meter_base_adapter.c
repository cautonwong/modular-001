#include "application/adapters/dl698/meter_base_view.h"

#include <stddef.h>

#include "nidavellir.h"
#include "kernel/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

static struct mbase_control_block_t *mb(struct instance_t *instance)
{
    return (struct mbase_control_block_t *)get_pipeline_ctx(instance,
                                                             pid_meter_base);
}

int nd_mb_valid(struct instance_t *instance)
{
    return mb(instance) != NULL;
}

uint32_t *nd_mb_report_2015_state_ptr(struct instance_t *instance)
{
    return nd_mbase_report_2015state(mb(instance));
}

uint32_t *nd_mb_report_2015_cache_ptr(struct instance_t *instance)
{
    return &nd_mbase_report_2015state_cache(mb(instance));
}

uint16_t *nd_mb_active_report_cmdtype_ptr(struct instance_t *instance)
{
    return &nd_mbase_active_report_CMDtype(mb(instance));
}

uint8_t *nd_mb_active_report_retry_cnt_ptr(struct instance_t *instance)
{
    return nd_mbase_active_report_retry_cnt(mb(instance));
}

uint8_t *nd_mb_active_report_retry_dly_ptr(struct instance_t *instance)
{
    return nd_mbase_active_report_retry_dly(mb(instance));
}

uint8_t *nd_mb_report_piid_ptr(struct instance_t *instance)
{
    return &nd_mbase_report_piid(mb(instance));
}

uint8_t *nd_mb_report_random_data_ptr(struct instance_t *instance)
{
    return nd_mbase_report_random_data(mb(instance));
}

uint8_t *nd_mb_switch_on_success_ptr(struct instance_t *instance)
{
    return &nd_mbase_switch_on_succees(mb(instance));
}

uint8_t *nd_mb_switch_off_success_ptr(struct instance_t *instance)
{
    return &nd_mbase_switch_off_succees(mb(instance));
}

uint8_t *nd_mb_esam_chip_abnormal_ptr(struct instance_t *instance)
{
    return &nd_mbase_esam_chip_abnormal(mb(instance));
}

uint8_t *nd_mb_auth_on_ptr(struct instance_t *instance)
{
    return &nd_mbase_AuthOn(mb(instance));
}

uint32_t *nd_mb_report_2015_mode_ptr(struct instance_t *instance)
{
    return &nd_mbase_report_2015mode(mb(instance));
}

uint8_t *nd_mb_poweron_active_report_dly_ptr(struct instance_t *instance)
{
    return &nd_mbase_poweron_active_report_dly(mb(instance));
}

uint32_t *nd_mb_power_on_sec_accum_ptr(struct instance_t *instance)
{
    return &nd_mbase_power_on_sec_accum(mb(instance));
}

uint16_t *nd_mb_active_report_sec_limit_ptr(struct instance_t *instance)
{
    return &nd_mbase_active_report_sec_limit(mb(instance));
}

uint8_t *nd_mb_key_state_ptr(struct instance_t *instance)
{
    return &nd_mbase_key_state(mb(instance));
}

uint32_t *nd_mb_voltage_battery_ptr(struct instance_t *instance)
{
    return &nd_mbase_voltage_battery(mb(instance));
}

uint8_t *nd_mb_tsa_ptr(struct instance_t *instance)
{
    return nd_mbase_tsa(mb(instance));
}

uint32_t (*nd_mb_connect_timeout_ptr(struct instance_t *instance))[2]
{
    return nd_mbase_connect_timeout(mb(instance));
}

uint32_t (*nd_mb_esam_connect_timeout_ptr(struct instance_t *instance))[2]
{
    return nd_mbase_esam_connect_timeout(mb(instance));
}

uint8_t (*nd_mb_connect_status_ptr(struct instance_t *instance))[2]
{
    return nd_mbase_connect_status(mb(instance));
}

uint32_t (*nd_mb_connect_timeout_cnt_ptr(struct instance_t *instance))[2]
{
    return nd_mbase_connect_timeout_cnt(mb(instance));
}

uint8_t *nd_mb_current_connect_channel_ptr(struct instance_t *instance)
{
    return nd_mbase_current_connect_channel(mb(instance));
}

uint8_t *nd_mb_clientaddr_ptr(struct instance_t *instance)
{
    return nd_mbase_clientaddr(mb(instance));
}

uint32_t *nd_mb_term_auth_timeout_ptr(struct instance_t *instance)
{
    return &nd_mbase_term_auth_timeout(mb(instance));
}

uint32_t *nd_mb_infrared_auth_timeout_ptr(struct instance_t *instance)
{
    return &nd_mbase_infrared_auth_timeout(mb(instance));
}

uint8_t *nd_mb_mac_suspend_state_ptr(struct instance_t *instance)
{
    return &nd_mbase_mac_suspend_state(mb(instance));
}

uint8_t *nd_mb_factor_mode_ptr(struct instance_t *instance)
{
    return &nd_mbase_factor_mode(mb(instance));
}

uint8_t *nd_mb_lcd_autodis_list_num_ptr(struct instance_t *instance)
{
    return &nd_mbase_lcd_autodis_list_num(mb(instance));
}

uint16_t *nd_mb_lcd_autodis_keep_sec_ptr(struct instance_t *instance)
{
    return &nd_mbase_lcd_autodis_keep_sec(mb(instance));
}

uint8_t *nd_mb_commdisp_plc_ptr(struct instance_t *instance)
{
    return &nd_mbase_commdisp_plc(mb(instance));
}

uint8_t *nd_mb_commdisp_ir485_ptr(struct instance_t *instance)
{
    return &nd_mbase_commdisp_ir485(mb(instance));
}

uint16_t *nd_mb_backlight_tmr_ptr(struct instance_t *instance)
{
    return &nd_mbase_BackLightTmr(mb(instance));
}

void nd_mb_save_report_2015_state(struct instance_t *instance)
{
    struct mbase_control_block_t *mbase_cb = mb(instance);
    uint32_t addr = ((uint32_t)RES_DEV_EEPROM << 24)
                    + ((uint32_t)mbase_block_persistent_eprom << 16)
                    + nd_mbase_offset_report_2015state()
                    - nd_mbase_offset_persistent_section_start();

    nd_set_offset(instance, addr, (uint8_t *)&nd_mbase_report_2015state(mbase_cb)[0],
                  4 * 4);
}

void nd_mb_save_active_report_state(struct instance_t *instance)
{
    struct mbase_control_block_t *mbase_cb = mb(instance);
    uint32_t addr = ((uint32_t)RES_DEV_EEPROM << 24)
                    + ((uint32_t)mbase_block_persistent_eprom << 16)
                    + nd_mbase_offset_active_report_CMDtype()
                    - nd_mbase_offset_persistent_section_start();
    uint32_t len = nd_mbase_offset_active_report_retry_cnt()
                   - nd_mbase_offset_active_report_CMDtype()
                   + sizeof(nd_mbase_active_report_retry_cnt(mbase_cb));

    nd_set_offset(instance, addr, (uint8_t *)&nd_mbase_active_report_CMDtype(mbase_cb),
                  len);
}
