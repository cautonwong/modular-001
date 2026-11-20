#include "sim.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu_surport.h"
#include "targets/stategrid_meter/app/ram_budget.h"

sim_config_t g_sim = {
    .cycles = 10,
    .arena_size = STG_RAM_ARENA_SIZE,
    .eeprom_path = "backupfiles/firmware_eeprom.bin",
    .flash_path = "backupfiles/firmware_flash.bin",
    .trace_path = NULL,
    .verbose = 0,
};

static FILE *trace_file;
static unsigned cycle_index;
static unsigned malloc_calls;

void sim_trace_open(void)
{
    if (g_sim.trace_path == NULL) {
        trace_file = stdout;
        return;
    }
    trace_file = fopen(g_sim.trace_path, "w");
    if (trace_file == NULL) {
        perror(g_sim.trace_path);
        trace_file = stdout;
    }
}

void sim_trace_close(void)
{
    if (trace_file != NULL && trace_file != stdout) {
        fclose(trace_file);
    }
    trace_file = NULL;
}

void sim_trace_printf(const char *format, ...)
{
    va_list args;

    if (trace_file == NULL) {
        trace_file = stdout;
    }
    va_start(args, format);
    vfprintf(trace_file, format, args);
    va_end(args);
    fputc('\n', trace_file);
    fflush(trace_file);
}

int nd_sim_malloc_should_fail(void)
{
    malloc_calls++;
    if ((g_sim.fail_malloc_at != 0u) && (malloc_calls == g_sim.fail_malloc_at)) {
        sim_trace_printf("[sim] malloc fail call=%u", malloc_calls);
        return 1;
    }
    return 0;
}

unsigned nd_sim_malloc_count(void)
{
    return malloc_calls;
}

int nd_sim_should_stop(stategrid_context_t *instance)
{
    if (cycle_index >= g_sim.cycles) {
        return 1;
    }

    sim_scenario_apply(instance, cycle_index);

    if (cycle_index == 0 && sim_scenario_power_on()) {
        /* The firmware initializes the meter in the power-off state. Keep the
         * normal metering path selected so the second tick is delivered to
         * pl_meter_base instead of the low-power pipeline. */
        instance->ac_power_status = AC_POWER_ON;
    }

    /* The baremetal firmware has no ms timebase of its own: instance->ms is
     * initialized to zero and never advanced. The simulation supplies one ms
     * per second tick so DL698 timers see a monotonic clock. */
    instance->ms = cycle_index * 1000u;

    if (instance->pipe_rtc.buf_ptr != NULL) {
        fifoBuf_putByte(&instance->pipe_rtc, IRQ_RTC);
    }

    sim_trace_printf("[sim] cycle=%u time=%04d-%02d-%02d %02d:%02d:%02d ms=%u",
                     cycle_index,
                     instance->time.tm_year + 1900,
                     instance->time.tm_mon + 1,
                     instance->time.tm_mday,
                     instance->time.tm_hour,
                     instance->time.tm_min,
                     instance->time.tm_sec,
                     instance->ms);
    cycle_index++;
    return 0;
}

void nd_sim_after_run(stategrid_context_t *instance)
{
    (void)instance;
    sim_uart_drain();
}

void nd_sim_filter(const char *pipeline, uint8_t filter_id, int ret)
{
    if (!g_sim.verbose) {
        return;
    }
    sim_trace_printf("[sim] filter pipeline=%s id=0x%02x ret=%d",
                     pipeline != NULL ? pipeline : "?",
                     (unsigned)filter_id, ret);
}

void nd_sim_event(uint8_t from, uint8_t to, uint8_t ctrl, uint16_t len)
{
    if (!g_sim.verbose) {
        return;
    }
    sim_trace_printf("[sim] msg from=0x%02x to=0x%02x ctrl=0x%02x len=%u",
                     (unsigned)from, (unsigned)to, (unsigned)ctrl,
                     (unsigned)len);
}

void nd_sim_stage(const char *name)
{
    sim_scenario_apply_stage(name);
    if (!g_sim.verbose) {
        return;
    }
    sim_trace_printf("[sim] stage=%s", name);
}

void nd_sim_storage_layout(uint32_t used, uint32_t region)
{
    sim_trace_printf("[sim] eeprom-used=%u snapshot-region=%u margin=%u",
                     used, region, region - used);
}

void nd_sim_system_reset(void)
{
    sim_trace_printf("[sim] NVIC_SystemReset requested; stopping simulation");
    sim_trace_close();
    exit(3);
}
