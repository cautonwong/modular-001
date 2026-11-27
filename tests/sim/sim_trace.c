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

/* Budget counters. The stage counter runs even without --verbose so a plain
 * run still measures the per-pass work. */
static unsigned stage_count_this_cycle;
static unsigned stage_count_max;
static unsigned stage_count_total;
static int power_off_pending;
static unsigned power_off_cycle;
static unsigned power_off_entry_passes;
static int power_off_measured;
static int wake_pending;
static unsigned wake_cycle;
static unsigned wake_latency_passes;
static int wake_measured;
static unsigned watchdog_kicks;
static unsigned watchdog_max_gap;
static unsigned watchdog_last_cycle;
static int watchdog_seen;
static unsigned stack_high_water_bytes;

static unsigned stats_current_cycle(void)
{
    return cycle_index > 0 ? cycle_index - 1 : 0;
}

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

    /* Finalize the pass that just ended before this pass's scenario applies. */
    if (stage_count_this_cycle > stage_count_max) {
        stage_count_max = stage_count_this_cycle;
    }
    if (power_off_pending && (cycle_index > 0) && (stage_count_this_cycle == 0)) {
        power_off_entry_passes = cycle_index - 1 - power_off_cycle;
        power_off_measured = 1;
        power_off_pending = 0;
    }
    stage_count_this_cycle = 0;

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
    stage_count_total++;
    stage_count_this_cycle++;
    if (wake_pending && (stage_count_max > 0)
        && (stage_count_this_cycle >= stage_count_max)) {
        wake_latency_passes = stats_current_cycle() - wake_cycle;
        wake_measured = 1;
        wake_pending = 0;
    }
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

void sim_stats_power(int on, unsigned cycle)
{
    if (on) {
        wake_pending = 1;
        wake_cycle = cycle;
        wake_measured = 0;
        return;
    }
    wake_pending = 0;
    if (!power_off_pending) {
        power_off_pending = 1;
        power_off_cycle = cycle;
        power_off_measured = 0;
    }
}

void sim_stats_watchdog_kick(void)
{
    unsigned cycle = stats_current_cycle();
    unsigned gap;

    watchdog_kicks++;
    if (watchdog_seen) {
        gap = cycle - watchdog_last_cycle;
        if (gap > watchdog_max_gap) {
            watchdog_max_gap = gap;
        }
    }
    watchdog_last_cycle = cycle;
    watchdog_seen = 1;
}

void sim_stats_stack_high_water(unsigned bytes)
{
    stack_high_water_bytes = bytes;
}

void sim_stats_report(void)
{
    char wake[16];
    char power_off[16];

    if (stage_count_this_cycle > stage_count_max) {
        stage_count_max = stage_count_this_cycle;
    }
    if (wake_measured) {
        snprintf(wake, sizeof(wake), "%u", wake_latency_passes);
    } else {
        snprintf(wake, sizeof(wake), "none");
    }
    if (power_off_measured) {
        snprintf(power_off, sizeof(power_off), "%u", power_off_entry_passes);
    } else {
        snprintf(power_off, sizeof(power_off), "none");
    }
    sim_trace_printf("[sim] stats cycles=%u stages=%u stage-max=%u "
                     "wake-latency-passes=%s power-off-entry-passes=%s "
                     "watchdog-kicks=%u watchdog-max-gap-passes=%u "
                     "stack-high-water-bytes=%u",
                     cycle_index, stage_count_total, stage_count_max,
                     wake, power_off, watchdog_kicks, watchdog_max_gap,
                     stack_high_water_bytes);
}
