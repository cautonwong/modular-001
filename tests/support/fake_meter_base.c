#include "fake_meter_base.h"

#include <string.h>

#include "application/ports/meter_base.h"

#define FAKE_CONNECT_ROWS 4
#define FAKE_BYTES 16

static int valid_value;
static uint8_t mac_suspend_state;
static uint8_t tsa[8];
static uint8_t commdisp_ir485;
static uint8_t clientaddr[FAKE_CONNECT_ROWS];
static uint8_t connect_status[FAKE_CONNECT_ROWS][2];

static uint16_t *active_report_cmdtype(void *ctx)
{
    static uint16_t value;
    (void)ctx;
    return &value;
}

static uint8_t *active_report_retry_cnt(void *ctx)
{
    static uint8_t value[8];
    (void)ctx;
    return value;
}

static uint8_t *active_report_retry_dly(void *ctx)
{
    static uint8_t value[8];
    (void)ctx;
    return value;
}

static uint16_t *active_report_sec_limit(void *ctx)
{
    static uint16_t value;
    (void)ctx;
    return &value;
}

static uint8_t *auth_on(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint16_t *backlight_tmr(void *ctx)
{
    static uint16_t value;
    (void)ctx;
    return &value;
}

static uint32_t (*connect_timeout(void *ctx))[2]
{
    static uint32_t value[FAKE_CONNECT_ROWS][2];
    (void)ctx;
    return value;
}

static uint8_t *current_connect_channel(void *ctx)
{
    static uint8_t value[2];
    (void)ctx;
    return value;
}

static int32_t esam_chip_abnormal(void *ctx)
{
    (void)ctx;
    return 0;
}

static uint32_t (*esam_connect_timeout(void *ctx))[2]
{
    static uint32_t value[FAKE_CONNECT_ROWS][2];
    (void)ctx;
    return value;
}

static uint8_t *factor_mode(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint32_t *infrared_auth_timeout(void *ctx)
{
    static uint32_t value;
    (void)ctx;
    return &value;
}

static uint8_t *key_state(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint16_t *lcd_autodis_keep_sec(void *ctx)
{
    static uint16_t value;
    (void)ctx;
    return &value;
}

static uint8_t *lcd_autodis_list_num(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint8_t *mac_suspend(void *ctx)
{
    (void)ctx;
    return &mac_suspend_state;
}

static uint32_t *power_on_sec_accum(void *ctx)
{
    static uint32_t value;
    (void)ctx;
    return &value;
}

static uint8_t *poweron_active_report_dly(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint32_t *report_2015_cache(void *ctx)
{
    static uint32_t value;
    (void)ctx;
    return &value;
}

static uint32_t *report_2015_mode(void *ctx)
{
    static uint32_t value;
    (void)ctx;
    return &value;
}

static uint32_t *report_2015_state(void *ctx)
{
    static uint32_t value[4];
    (void)ctx;
    return value;
}

static uint8_t *report_piid(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint8_t *report_random_data(void *ctx)
{
    static uint8_t value[12];
    (void)ctx;
    return value;
}

static void save_active_report_state(void *ctx)
{
    (void)ctx;
}

static void save_report_2015_state(void *ctx)
{
    (void)ctx;
}

static uint32_t *term_auth_timeout(void *ctx)
{
    static uint32_t value;
    (void)ctx;
    return &value;
}

static uint8_t *tsa_ptr(void *ctx)
{
    (void)ctx;
    return tsa;
}

static int valid(void *ctx)
{
    (void)ctx;
    return valid_value;
}

static uint32_t *voltage_battery(void *ctx)
{
    static uint32_t value;
    (void)ctx;
    return &value;
}

static int32_t power_state(void *ctx)
{
    (void)ctx;
    return 0;
}

static uint32_t (*connect_timeout_cnt(void *ctx))[2]
{
    static uint32_t value[FAKE_CONNECT_ROWS][2];
    (void)ctx;
    return value;
}

static uint8_t *commdisp_plc(void *ctx)
{
    static uint8_t value;
    (void)ctx;
    return &value;
}

static uint8_t *commdisp_ir485_ptr(void *ctx)
{
    (void)ctx;
    return &commdisp_ir485;
}

static uint8_t (*connect_status_ptr(void *ctx))[2]
{
    (void)ctx;
    return connect_status;
}

static uint8_t *clientaddr_ptr(void *ctx)
{
    (void)ctx;
    return clientaddr;
}

static const nd_meter_base_port_t fake_port = {
    NULL,
    active_report_cmdtype,
    active_report_retry_cnt,
    active_report_retry_dly,
    active_report_sec_limit,
    auth_on,
    backlight_tmr,
    clientaddr_ptr,
    commdisp_ir485_ptr,
    commdisp_plc,
    connect_status_ptr,
    connect_timeout,
    connect_timeout_cnt,
    current_connect_channel,
    esam_chip_abnormal,
    esam_connect_timeout,
    factor_mode,
    infrared_auth_timeout,
    key_state,
    lcd_autodis_keep_sec,
    lcd_autodis_list_num,
    mac_suspend,
    power_on_sec_accum,
    poweron_active_report_dly,
    report_2015_cache,
    report_2015_mode,
    report_2015_state,
    report_piid,
    report_random_data,
    save_active_report_state,
    save_report_2015_state,
    term_auth_timeout,
    tsa_ptr,
    valid,
    voltage_battery,
    power_state
};

__attribute__((weak)) const nd_meter_base_port_t *nd_meter_base_get(void *instance)
{
    (void)instance;
    return &fake_port;
}

void test_meter_base_reset(void)
{
    memset(tsa, 0, sizeof(tsa));
    memset(clientaddr, 0, sizeof(clientaddr));
    memset(connect_status, 0, sizeof(connect_status));
    valid_value = 0;
    mac_suspend_state = 0;
    commdisp_ir485 = 0;
}

void test_meter_base_set_valid(int value)
{
    valid_value = value;
}

uint8_t *test_meter_base_tsa(void)
{
    return tsa;
}

uint8_t *test_meter_base_commdisp_ir485(void)
{
    return &commdisp_ir485;
}

uint8_t *test_meter_base_clientaddr(void)
{
    return clientaddr;
}

uint8_t *test_meter_base_connect_status(void)
{
    return &connect_status[0][0];
}

void test_meter_base_set_mac_suspend_state(int value)
{
    mac_suspend_state = (uint8_t)value;
}
