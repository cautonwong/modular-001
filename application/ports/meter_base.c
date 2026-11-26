#include "application/ports/meter_base.h"

int nd_mb_get_valid(void *instance)
{
    return nd_meter_base_get(instance)->valid(instance);
}

int32_t nd_mb_get_voltage_battery(void *instance)
{
    return (int32_t)*nd_meter_base_get(instance)->voltage_battery(instance);
}

int32_t nd_mb_get_esam_chip_abnormal(void *instance)
{
    return nd_meter_base_get(instance)->esam_chip_abnormal(instance);
}

int32_t nd_mb_get_auth_on(void *instance)
{
    return *nd_meter_base_get(instance)->auth_on(instance);
}

int32_t nd_mb_get_term_auth_timeout(void *instance)
{
    return (int32_t)*nd_meter_base_get(instance)->term_auth_timeout(instance);
}

void nd_mb_set_term_auth_timeout(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->term_auth_timeout(instance) = (uint32_t)value;
}

int32_t nd_mb_get_infrared_auth_timeout(void *instance)
{
    return (int32_t)*nd_meter_base_get(instance)->infrared_auth_timeout(instance);
}

int32_t nd_mb_get_factor_mode(void *instance)
{
    return *nd_meter_base_get(instance)->factor_mode(instance);
}

int32_t nd_mb_get_mac_suspend_state(void *instance)
{
    return *nd_meter_base_get(instance)->mac_suspend_state(instance);
}

int32_t nd_mb_get_key_state(void *instance)
{
    return *nd_meter_base_get(instance)->key_state(instance);
}

void nd_mb_set_backlight_tmr(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->backlight_tmr(instance) = (uint16_t)value;
}

void nd_mb_set_commdisp_ir485(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->commdisp_ir485(instance) = (uint8_t)value;
}

void nd_mb_set_commdisp_plc(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->commdisp_plc(instance) = (uint8_t)value;
}

int32_t nd_mb_get_lcd_autodis_keep_sec(void *instance)
{
    return *nd_meter_base_get(instance)->lcd_autodis_keep_sec(instance);
}

int32_t nd_mb_get_lcd_autodis_list_num(void *instance)
{
    return *nd_meter_base_get(instance)->lcd_autodis_list_num(instance);
}

int32_t nd_mb_get_power_on_sec_accum(void *instance)
{
    return (int32_t)*nd_meter_base_get(instance)->power_on_sec_accum(instance);
}

int32_t nd_mb_get_poweron_active_report_dly(void *instance)
{
    return *nd_meter_base_get(instance)->poweron_active_report_dly(instance);
}

int32_t nd_mb_get_active_report_sec_limit(void *instance)
{
    return *nd_meter_base_get(instance)->active_report_sec_limit(instance);
}

int32_t nd_mb_get_active_report_cmdtype(void *instance)
{
    return *nd_meter_base_get(instance)->active_report_cmdtype(instance);
}

void nd_mb_set_active_report_cmdtype(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->active_report_cmdtype(instance) = (uint16_t)value;
}

int32_t nd_mb_get_report_2015_cache(void *instance)
{
    return (int32_t)*nd_meter_base_get(instance)->report_2015_cache(instance);
}

void nd_mb_set_report_2015_cache(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->report_2015_cache(instance) = (uint32_t)value;
}

int32_t nd_mb_get_report_2015_mode(void *instance)
{
    return (int32_t)*nd_meter_base_get(instance)->report_2015_mode(instance);
}

int32_t nd_mb_get_report_piid(void *instance)
{
    return *nd_meter_base_get(instance)->report_piid(instance);
}

void nd_mb_set_report_piid(void *instance, int32_t value)
{
        *nd_meter_base_get(instance)->report_piid(instance) = (uint8_t)value;
}

int32_t nd_mb_get_power_state(void *instance)
{
    return nd_meter_base_get(instance)->power_state(instance);
}

int32_t nd_mb_get_connect_timeout(void *instance, uint16_t channel, uint16_t client)
{
    return (int32_t)nd_meter_base_get(instance)->connect_timeout(instance)[channel][client];
}

void nd_mb_set_connect_timeout(void *instance, uint16_t channel, uint16_t client, int32_t value)
{
    nd_meter_base_get(instance)->connect_timeout(instance)[channel][client] = (uint32_t)value;
}

int32_t nd_mb_get_esam_connect_timeout(void *instance, uint16_t channel, uint16_t client)
{
    return (int32_t)nd_meter_base_get(instance)->esam_connect_timeout(instance)[channel][client];
}

void nd_mb_set_esam_connect_timeout(void *instance, uint16_t channel, uint16_t client, int32_t value)
{
    nd_meter_base_get(instance)->esam_connect_timeout(instance)[channel][client] =
            (uint32_t)value;
}

int32_t nd_mb_get_connect_status(void *instance, uint16_t channel, uint16_t client)
{
    return nd_meter_base_get(instance)->connect_status(instance)[channel][client];
}

void nd_mb_set_connect_status(void *instance, uint16_t channel, uint16_t client, int32_t value)
{
    nd_meter_base_get(instance)->connect_status(instance)[channel][client] = (uint8_t)value;
}

int32_t nd_mb_get_connect_timeout_cnt(void *instance, uint16_t channel, uint16_t client)
{
    return (int32_t)nd_meter_base_get(instance)->connect_timeout_cnt(instance)[channel][client];
}

void nd_mb_set_connect_timeout_cnt(void *instance, uint16_t channel, uint16_t client, int32_t value)
{
    nd_meter_base_get(instance)->connect_timeout_cnt(instance)[channel][client] =
            (uint32_t)value;
}

int32_t nd_mb_get_current_connect_channel(void *instance, uint16_t client)
{
    return nd_meter_base_get(instance)->current_connect_channel(instance)[client];
}

void nd_mb_set_current_connect_channel(void *instance, uint16_t client, int32_t value)
{
    nd_meter_base_get(instance)->current_connect_channel(instance)[client] = (uint8_t)value;
}

int32_t nd_mb_get_clientaddr(void *instance, uint16_t channel)
{
    return nd_meter_base_get(instance)->clientaddr(instance)[channel];
}

void nd_mb_set_clientaddr(void *instance, uint16_t channel, int32_t value)
{
    nd_meter_base_get(instance)->clientaddr(instance)[channel] = (uint8_t)value;
}

int32_t nd_mb_get_report_2015_state(void *instance, uint16_t channel)
{
    return (int32_t)nd_meter_base_get(instance)->report_2015_state(instance)[channel];
}

void nd_mb_set_report_2015_state(void *instance, uint16_t channel, int32_t value)
{
    nd_meter_base_get(instance)->report_2015_state(instance)[channel] = (uint32_t)value;
}

int32_t nd_mb_get_active_report_retry_cnt(void *instance, uint16_t type)
{
    return nd_meter_base_get(instance)->active_report_retry_cnt(instance)[type];
}

void nd_mb_set_active_report_retry_cnt(void *instance, uint16_t type, int32_t value)
{
    nd_meter_base_get(instance)->active_report_retry_cnt(instance)[type] = (uint8_t)value;
}

int32_t nd_mb_get_active_report_retry_dly(void *instance, uint16_t type)
{
    return nd_meter_base_get(instance)->active_report_retry_dly(instance)[type];
}

void nd_mb_set_active_report_retry_dly(void *instance, uint16_t type, int32_t value)
{
    nd_meter_base_get(instance)->active_report_retry_dly(instance)[type] = (uint8_t)value;
}

void nd_mb_get_tsa(void *instance, uint8_t *buf, uint16_t length)
{
    const uint8_t *src = nd_meter_base_get(instance)->tsa(instance);
    uint16_t i;

    for (i = 0; i < length; i++) {
        buf[i] = src[i];
    }
}

void nd_mb_get_report_random_data(void *instance, uint8_t *buf, uint16_t length)
{
    const uint8_t *src = nd_meter_base_get(instance)->report_random_data(instance);
    uint16_t i;

    for (i = 0; i < length; i++) {
        buf[i] = src[i];
    }
}

void nd_mb_do_save_active_report_state(void *instance)
{
    nd_meter_base_get(instance)->save_active_report_state(instance);
}

void nd_mb_do_save_report_2015_state(void *instance)
{
    nd_meter_base_get(instance)->save_report_2015_state(instance);
}
