#include "targets/stategrid_meter/app/platform_adapter.h"

#include "infra/soc/v85xxp/v8500/v85xxp_system.h"
#include "targets/stategrid_meter/board/board_io.h"
#include "targets/stategrid_meter/board/board_eeprom.h"
#include "targets/stategrid_meter/app/metering_energy_adapter.h"

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
    stategrid_context_t *instance = ctx;

    if (instance == NULL) {
        return false;
    }
    return nd_board_device_error(instance->board) != 0
        || instance->storage_fault != 0;
}

static void platform_device_error_reported(void *ctx)
{
    metering_energy_storage_fault_reported(ctx);
}

void stategrid_platform_attach(stategrid_context_t *instance)
{
    instance->platform.ctx = instance;
    instance->platform.watchdog_kick = platform_watchdog_kick;
    instance->platform.event_output_set = platform_event_output_set;
    instance->platform.device_error = platform_device_error;
    instance->platform.device_error_reported = platform_device_error_reported;
}
