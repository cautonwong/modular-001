#include "sim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "nidavellir.h"
#include "resource_tool.h"
#include "infra/kernel/resource_blocks.h"
#include "targets/stategrid_meter/app/dl698_parameter_adapter.h"
#include "targets/stategrid_meter/app/platform_adapter.h"
#include "targets/stategrid_meter/app/backlight_adapter.h"
#include "targets/stategrid_meter/app/relay_status_adapter.h"
#include "targets/stategrid_meter/app/metering_input_adapter.h"
#include "targets/stategrid_meter/app/runtime_adapter.h"
#include "targets/stategrid_meter/app/timer_adapter.h"

#ifndef MAP_32BIT
#define MAP_32BIT 0
#endif
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0
#endif

#define SIM_ARENA_SIZE (24u * 1024u)
#define SIM_RESOURCE_BLOCK_SIZE (2u * 1024u)

/* The firmware writes MCU control registers directly, for example GPIOC in
 * PowerOff_Save_data(). Give those addresses a real page so a scenario power
 * cut runs the normal power-off path instead of faulting. */
#define SIM_PERIPH_BASE ((void *)0x40000000u)
#define SIM_PERIPH_SIZE (1024u * 1024u)

extern int baremetal_instance_entry_main(struct instance_t *instance, int argc,
                                         char **argv);

static int sim_arena_clear_of_peripherals(void *arena, size_t size)
{
    uintptr_t start = (uintptr_t)arena;
    uintptr_t end = start + size;
    uintptr_t periph_start = (uintptr_t)SIM_PERIPH_BASE;
    uintptr_t periph_end = periph_start + SIM_PERIPH_SIZE;

    return (end <= periph_start) || (start >= periph_end);
}

static void *sim_arena_map(size_t size)
{
    void *arena = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);

    if (arena == MAP_FAILED) {
        return NULL;
    }
    if ((uintptr_t)arena > UINT32_MAX) {
        fprintf(stderr,
                "firmware_sim: arena at %p does not fit the firmware's 32-bit "
                "resource addresses\n",
                arena);
        munmap(arena, size);
        return NULL;
    }
    if (!sim_arena_clear_of_peripherals(arena, size)) {
        fprintf(stderr,
                "firmware_sim: arena at %p overlaps the peripheral register "
                "block\n",
                arena);
        munmap(arena, size);
        return NULL;
    }
    return arena;
}

static void sim_arena_unmap(void *arena, size_t size)
{
    if (arena != NULL) {
        munmap(arena, size);
    }
}

static unsigned char sim_peripheral_block[SIM_PERIPH_SIZE]
    __attribute__((section(".sim_periph"), aligned(4096), used));

static void *sim_peripheral_map(size_t size)
{
    if ((uintptr_t)sim_peripheral_block == (uintptr_t)SIM_PERIPH_BASE) {
        return sim_peripheral_block;
    }
    /*
     * MAP_FIXED would silently replace a mapping the loader happened to place
     * at the peripheral window. MAP_FIXED_NOREPLACE fails instead, and the
     * plain-hint fallback maps anywhere and verifies the address afterwards.
     */
#if MAP_FIXED_NOREPLACE
    void *base = mmap(SIM_PERIPH_BASE, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
#else
    void *base = mmap(SIM_PERIPH_BASE, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

    if (base == MAP_FAILED) {
        return NULL;
    }
    if (base != SIM_PERIPH_BASE) {
        munmap(base, size);
        return NULL;
    }
    return base;
}

static void usage(const char *program)
{
    printf("usage: %s [--cycles N] [--eeprom PATH] [--flash PATH] "
           "[--trace PATH] [--scenario PATH] [--start-time "
           "'YYYY-MM-DD HH:MM:SS'] [--uart-tx PATH] [--uart-tx-hex PATH] "
           "[--relay-log PATH] [--backlight-log PATH] [--lcd-log PATH] "
           "[--arena-size BYTES] [--verbose]\n",
           program);
}

static int parse_start_time(const char *text, struct tm *value)
{
    int year, month, day, hour, minute, second;
    char extra;

    if (sscanf(text, "%d-%d-%d %d:%d:%d%c", &year, &month, &day, &hour,
               &minute, &second, &extra) != 6) {
        return -1;
    }
    if ((year < 1970) || (month < 1) || (month > 12) || (day < 1) || (day > 31)
        || (hour < 0) || (hour > 23) || (minute < 0) || (minute > 59)
        || (second < 0) || (second > 59)) {
        return -1;
    }
    memset(value, 0, sizeof(*value));
    value->tm_year = year - 1900;
    value->tm_mon = month - 1;
    value->tm_mday = day;
    value->tm_hour = hour;
    value->tm_min = minute;
    value->tm_sec = second;
    return 0;
}

static int parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cycles") == 0 && i + 1 < argc) {
            g_sim.cycles = (unsigned)strtoul(argv[++i], NULL, 0);
            if (g_sim.cycles == 0) {
                g_sim.cycles = 1;
            }
        } else if (strcmp(argv[i], "--eeprom") == 0 && i + 1 < argc) {
            g_sim.eeprom_path = argv[++i];
        } else if (strcmp(argv[i], "--flash") == 0 && i + 1 < argc) {
            g_sim.flash_path = argv[++i];
        } else if (strcmp(argv[i], "--trace") == 0 && i + 1 < argc) {
            g_sim.trace_path = argv[++i];
        } else if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            g_sim.scenario_path = argv[++i];
        } else if (strcmp(argv[i], "--start-time") == 0 && i + 1 < argc) {
            g_sim.start_time = argv[++i];
        } else if (strcmp(argv[i], "--uart-tx") == 0 && i + 1 < argc) {
            g_sim.uart_tx_path = argv[++i];
        } else if (strcmp(argv[i], "--uart-tx-hex") == 0 && i + 1 < argc) {
            g_sim.uart_tx_hex_path = argv[++i];
        } else if (strcmp(argv[i], "--relay-log") == 0 && i + 1 < argc) {
            g_sim.relay_log_path = argv[++i];
        } else if (strcmp(argv[i], "--backlight-log") == 0 && i + 1 < argc) {
            g_sim.backlight_log_path = argv[++i];
        } else if (strcmp(argv[i], "--lcd-log") == 0 && i + 1 < argc) {
            g_sim.lcd_log_path = argv[++i];
        } else if (strcmp(argv[i], "--arena-size") == 0 && i + 1 < argc) {
            g_sim.arena_size = (unsigned)strtoul(argv[++i], NULL, 0);
            if (g_sim.arena_size == 0) {
                fprintf(stderr, "firmware_sim: --arena-size must be positive\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--verbose") == 0) {
            g_sim.verbose = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "firmware_sim: unknown argument %s\n", argv[i]);
            usage(argv[0]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct instance_t *instance;

    struct tm start_time;
    void *arena;
    int start_time_set = 0;

    if (sim_peripheral_map(SIM_PERIPH_SIZE) == NULL) {
        fprintf(stderr, "firmware_sim: cannot map the peripheral register block\n");
        return 2;
    }
    if (parse_args(argc, argv) != 0) {
        return 2;
    }
    if (g_sim.start_time != NULL) {
        if (parse_start_time(g_sim.start_time, &start_time) != 0) {
            fprintf(stderr,
                    "firmware_sim: invalid --start-time %s; expected "
                    "'YYYY-MM-DD HH:MM:SS'\n",
                    g_sim.start_time);
            return 2;
        }
        start_time_set = 1;
    }
    if (sim_scenario_load(g_sim.scenario_path) != 0) {
        return 2;
    }
    if (sim_script_open() != 0) {
        return 2;
    }
    sim_trace_open();

    if (sim_devices_init() != 0) {
        sim_script_close();
        sim_trace_close();
        return 2;
    }
    arena = sim_arena_map(g_sim.arena_size);
    if (arena == NULL) {
        fprintf(stderr, "firmware_sim: cannot map the low arena\n");
        sim_devices_close();
        sim_script_close();
        sim_trace_close();
        return 2;
    }

    instance = create_instance(RES_DEV_RAM, (uint32_t)(uintptr_t)arena,
                               g_sim.arena_size, SIM_RESOURCE_BLOCK_SIZE,
                               resource_block_name);
    if (instance == NULL) {
        fprintf(stderr, "firmware_sim: create_instance failed\n");
        sim_arena_unmap(arena, g_sim.arena_size);
        sim_devices_close();
        sim_script_close();
        sim_trace_close();
        return 2;
    }

    if (start_time_set) {
        instance->time = start_time;
    } else {
        /* A fixed, plausible initial time. read_mcu_rtc() advances it at
         * second 20, and Rtc_Prossecc advances it one second per cycle. */
        memset(&instance->time, 0, sizeof(instance->time));
        instance->time.tm_year = 2021 - 1900;
        instance->time.tm_mon = 3;
        instance->time.tm_mday = 30;
        instance->time.tm_hour = 23;
        instance->time.tm_min = 59;
        instance->time.tm_sec = 50;
    }

    dl698_parameter_bind(instance);
    stategrid_platform_bind(instance);
    stategrid_backlight_bind();
    stategrid_relay_status_bind();
    stategrid_metering_input_bind();
    stategrid_runtime_bind();
    stategrid_timer_bind();

    sim_trace_printf("[sim] boot arena=%p instance=%p cycles=%u eeprom=%s "
                     "flash=%s",
                     arena, (void *)instance, g_sim.cycles, g_sim.eeprom_path,
                     g_sim.flash_path);

    int ret = baremetal_instance_entry_main(instance, 0, NULL);

    if (ret != 0) {
        sim_trace_printf("[sim] init failed ret=%d", ret);
    } else {
        sim_trace_printf("[sim] done cycles=%u", g_sim.cycles);
    }
    sim_devices_close();
    sim_script_close();
    sim_trace_close();
    sim_arena_unmap(arena, g_sim.arena_size);
    return ret;
}
