#include "targets/stategrid_meter/app/platform_adapter.h"

#include "nidavellir.h"
#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "infra/boards/stategrid_single_phase/board_io.h"

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

void stategrid_platform_bind(struct instance_t *instance)
{
    static nd_platform_port_t port;

    port.ctx = instance;
    port.watchdog_kick = platform_watchdog_kick;
    port.event_output_set = platform_event_output_set;
    nd_platform_bind(&port);
}
