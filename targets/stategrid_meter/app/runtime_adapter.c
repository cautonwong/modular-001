#include "targets/stategrid_meter/app/runtime_adapter.h"
#include "domain/common/runtime.h"
#include "cpu_surport.h"
#include <stddef.h>

static void board_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    mcu_delay_us((uint16_t)us);
}

static void board_watchdog_clear(void *ctx)
{
    (void)ctx;
    mcu_clear_watchdog();
}

static const nd_runtime_port_t runtime_port = {
    .ctx = NULL,
    .delay_us = board_delay_us,
    .watchdog_clear = board_watchdog_clear,
};

void stategrid_runtime_bind(void)
{
    nd_runtime_bind(&runtime_port);
}
