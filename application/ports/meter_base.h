#ifndef ND_APPLICATION_PORTS_METER_BASE_H
#define ND_APPLICATION_PORTS_METER_BASE_H

#include <stddef.h>
#include <stdint.h>

/* Consumer-defined meter-base port. One typed entry per nd_mb_* access used
 * by the 698 adapters. Entries whose field is written return the storage
 * pointer so lvalue writes need no setters; status and power return semantic
 * values (ND_STATUS_*, ND_POWER_*) rather than wire codes. The product
 * context is opaque here. */
typedef uint8_t (*nd_mb_connect_status_fn(void *ctx))[2];
typedef uint32_t (*nd_mb_connect_timeout_fn(void *ctx))[2];
typedef uint32_t (*nd_mb_connect_timeout_cnt_fn(void *ctx))[2];
typedef uint32_t (*nd_mb_esam_connect_timeout_fn(void *ctx))[2];

typedef struct {
    void *ctx;
    uint16_t *(*active_report_cmdtype)(void *ctx);
    uint8_t *(*active_report_retry_cnt)(void *ctx);
    uint8_t *(*active_report_retry_dly)(void *ctx);
    uint16_t *(*active_report_sec_limit)(void *ctx);
    uint8_t *(*auth_on)(void *ctx);
    uint16_t *(*backlight_tmr)(void *ctx);
    uint8_t *(*clientaddr)(void *ctx);
    uint8_t *(*commdisp_ir485)(void *ctx);
    uint8_t *(*commdisp_plc)(void *ctx);
    nd_mb_connect_status_fn *connect_status;
    nd_mb_connect_timeout_fn *connect_timeout;
    nd_mb_connect_timeout_cnt_fn *connect_timeout_cnt;
    uint8_t *(*current_connect_channel)(void *ctx);
    int32_t (*esam_chip_abnormal)(void *ctx);
    nd_mb_esam_connect_timeout_fn *esam_connect_timeout;
    uint8_t *(*factor_mode)(void *ctx);
    uint32_t *(*infrared_auth_timeout)(void *ctx);
    uint8_t *(*key_state)(void *ctx);
    uint16_t *(*lcd_autodis_keep_sec)(void *ctx);
    uint8_t *(*lcd_autodis_list_num)(void *ctx);
    uint8_t *(*mac_suspend_state)(void *ctx);
    uint32_t *(*power_on_sec_accum)(void *ctx);
    uint8_t *(*poweron_active_report_dly)(void *ctx);
    uint32_t *(*report_2015_cache)(void *ctx);
    uint32_t *(*report_2015_mode)(void *ctx);
    uint32_t *(*report_2015_state)(void *ctx);
    uint8_t *(*report_piid)(void *ctx);
    uint8_t *(*report_random_data)(void *ctx);
    void (*save_active_report_state)(void *ctx);
    void (*save_report_2015_state)(void *ctx);
    uint32_t *(*term_auth_timeout)(void *ctx);
    uint8_t *(*tsa)(void *ctx);
    int (*valid)(void *ctx);
    uint32_t *(*voltage_battery)(void *ctx);
    int32_t (*power_state)(void *ctx);
} nd_meter_base_port_t;

const nd_meter_base_port_t *nd_meter_base_get(void *instance);

int nd_mb_get_valid(void *instance);
int32_t nd_mb_get_voltage_battery(void *instance);
int32_t nd_mb_get_esam_chip_abnormal(void *instance);
int32_t nd_mb_get_auth_on(void *instance);
int32_t nd_mb_get_term_auth_timeout(void *instance);
void nd_mb_set_term_auth_timeout(void *instance, int32_t value);
int32_t nd_mb_get_infrared_auth_timeout(void *instance);
int32_t nd_mb_get_factor_mode(void *instance);
int32_t nd_mb_get_mac_suspend_state(void *instance);
int32_t nd_mb_get_key_state(void *instance);
void nd_mb_set_backlight_tmr(void *instance, int32_t value);
void nd_mb_set_commdisp_ir485(void *instance, int32_t value);
void nd_mb_set_commdisp_plc(void *instance, int32_t value);
int32_t nd_mb_get_lcd_autodis_keep_sec(void *instance);
int32_t nd_mb_get_lcd_autodis_list_num(void *instance);
int32_t nd_mb_get_power_on_sec_accum(void *instance);
int32_t nd_mb_get_poweron_active_report_dly(void *instance);
int32_t nd_mb_get_active_report_sec_limit(void *instance);
int32_t nd_mb_get_active_report_cmdtype(void *instance);
void nd_mb_set_active_report_cmdtype(void *instance, int32_t value);
int32_t nd_mb_get_report_2015_cache(void *instance);
void nd_mb_set_report_2015_cache(void *instance, int32_t value);
int32_t nd_mb_get_report_2015_mode(void *instance);
int32_t nd_mb_get_report_piid(void *instance);
void nd_mb_set_report_piid(void *instance, int32_t value);
int32_t nd_mb_get_power_state(void *instance);
int32_t nd_mb_get_connect_timeout(void *instance, uint16_t channel, uint16_t client);
void nd_mb_set_connect_timeout(void *instance, uint16_t channel, uint16_t client, int32_t value);
int32_t nd_mb_get_esam_connect_timeout(void *instance, uint16_t channel, uint16_t client);
void nd_mb_set_esam_connect_timeout(void *instance, uint16_t channel, uint16_t client, int32_t value);
int32_t nd_mb_get_connect_status(void *instance, uint16_t channel, uint16_t client);
void nd_mb_set_connect_status(void *instance, uint16_t channel, uint16_t client, int32_t value);
int32_t nd_mb_get_connect_timeout_cnt(void *instance, uint16_t channel, uint16_t client);
void nd_mb_set_connect_timeout_cnt(void *instance, uint16_t channel, uint16_t client, int32_t value);
int32_t nd_mb_get_current_connect_channel(void *instance, uint16_t client);
void nd_mb_set_current_connect_channel(void *instance, uint16_t client, int32_t value);
int32_t nd_mb_get_clientaddr(void *instance, uint16_t channel);
void nd_mb_set_clientaddr(void *instance, uint16_t channel, int32_t value);
int32_t nd_mb_get_report_2015_state(void *instance, uint16_t channel);
void nd_mb_set_report_2015_state(void *instance, uint16_t channel, int32_t value);
int32_t nd_mb_get_active_report_retry_cnt(void *instance, uint16_t type);
void nd_mb_set_active_report_retry_cnt(void *instance, uint16_t type, int32_t value);
int32_t nd_mb_get_active_report_retry_dly(void *instance, uint16_t type);
void nd_mb_set_active_report_retry_dly(void *instance, uint16_t type, int32_t value);
void nd_mb_get_tsa(void *instance, uint8_t *buf, uint16_t length);
void nd_mb_get_report_random_data(void *instance, uint8_t *buf, uint16_t length);
void nd_mb_do_save_active_report_state(void *instance);
void nd_mb_do_save_report_2015_state(void *instance);

#endif
