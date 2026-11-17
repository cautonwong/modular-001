#include "targets/stategrid_meter/app/platform_adapter.h"

#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "targets/stategrid_meter/board/board_io.h"
#include "targets/stategrid_meter/board/board_eeprom.h"

#include "application/ports/platform.h"

static void platform_watchdog_kick(void *ctx)
{
    (void)ctx;
    v85xxp_clear_watchdog();
}

static void platform_event_output_set(void *ctx, bool enable)
{
    (void)ctx;

    if (enable)
    {
        nd_board_plc_event_set(1);
    }
    else
    {
        nd_board_plc_event_set(0);
    }
}

static bool platform_device_error(void *ctx)
{
    (void)ctx;
    return (DEVICE_ERR & (1u << 1)) != 0;
}

void stategrid_platform_bind(stategrid_context_t *instance)
{
    static nd_platform_port_t port;

    port.ctx = instance;
    port.watchdog_kick = platform_watchdog_kick;
    port.event_output_set = platform_event_output_set;
    port.device_error = platform_device_error;
    nd_platform_bind(&port);
}
