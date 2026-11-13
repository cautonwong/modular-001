#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "infra/memory/arena.h"
#include "infra/memory/byte_store.h"
#include "infra/memory/file_store.h"
#include "infra/memory/parameter_store.h"
#include "targets/host/composition.h"

#define EEPROM_SIZE (64u * 1024u)
#define FLASH_SIZE (4u * 1024u * 1024u)

static uint8_t parameters[5] = {1, 1, 1, 1, 0};
static uint8_t weekend[1] = {0xFF};
static uint8_t weekend_fee[1] = {1};
static uint8_t season0[3] = {1, 1, 1};
static uint8_t day0[3] = {0, 0, 0x01};
static nd_param_record_t records[5] = {
    {916, parameters, sizeof(parameters)},
    {922, weekend, sizeof(weekend)},
    {923, weekend_fee, sizeof(weekend_fee)},
    {924, season0, sizeof(season0)},
    {952, day0, sizeof(day0)},
};

static void usage(const char *name)
{
    printf("usage: %s [--backup-files DIR] [--seconds N]\n", name);
    printf("  --backup-files DIR  persist EEPROM/flash to DIR/eeprom.bin and DIR/flash.bin\n");
    printf("  --seconds N         run N one-second ticks (default 60)\n");
}

static host_tick_t base_tick(void)
{
    host_tick_t tick;
    memset(&tick, 0, sizeof(tick));
    tick.direction = ND_ENERGY_FORWARD;
    tick.pulses = 20;
    tick.power = 100;
    tick.load.factory_mode = 1;
    tick.load.voltage = 220;
    tick.load.volt_low = 100;
    tick.load.volt_high = 300;
    tick.load.protection_current = 600;
    tick.load.protection_minutes = 5;
    return tick;
}

static void run_seconds(host_composition_t *composition, unsigned seconds)
{
    host_tick_t tick = base_tick();
    for (unsigned second = 0; second < seconds; second++) {
        tick.time.second = (uint8_t)(second % 60);
        tick.time.minute = (uint8_t)((second / 60) % 60);
        tick.time.hour = 0;
        tick.time.day = 1;
        tick.time.month = 1;
        tick.time.year = 2024;
        host_composition_second(composition, &tick);
    }
}

int main(int argc, char **argv)
{
    const char *backup_dir = NULL;
    unsigned seconds = 60;
    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--backup-files") == 0 && index + 1 < argc) {
            backup_dir = argv[++index];
        } else if (strcmp(argv[index], "--seconds") == 0 && index + 1 < argc) {
            seconds = (unsigned)strtoul(argv[++index], NULL, 10);
        } else if (strcmp(argv[index], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    static uint8_t arena_memory[8192];
    static uint8_t energy_memory[4096];
    nd_arena_t arena;
    nd_arena_init(&arena, arena_memory, sizeof(arena_memory));

    nd_byte_store_t memory_store;
    nd_file_store_t eeprom;
    nd_file_store_t flash;
    nd_store_t *energy_store;
    char eeprom_path[512];
    char flash_path[512];

    if (backup_dir != NULL) {
        snprintf(eeprom_path, sizeof(eeprom_path), "%s/eeprom.bin", backup_dir);
        snprintf(flash_path, sizeof(flash_path), "%s/flash.bin", backup_dir);
        if (nd_file_store_open(&eeprom, eeprom_path, EEPROM_SIZE) != 0) {
            fprintf(stderr, "cannot open %s\n", eeprom_path);
            return 1;
        }
        if (nd_file_store_open(&flash, flash_path, FLASH_SIZE) != 0) {
            fprintf(stderr, "cannot open %s\n", flash_path);
            nd_file_store_close(&eeprom);
            return 1;
        }
        energy_store = nd_file_store_port(&eeprom);
        printf("backup: %s, %s\n", eeprom_path, flash_path);
    } else {
        nd_byte_store_init(&memory_store, energy_memory, sizeof(energy_memory));
        energy_store = nd_byte_store_port(&memory_store);
        printf("backup: RAM only\n");
    }

    nd_param_table_t parameter_table;
    nd_param_table_init(&parameter_table, records, 5);

    host_config_t config;
    memset(&config, 0, sizeof(config));
    config.energy_store = energy_store;
    config.parameters = nd_param_table_port(&parameter_table);
    config.metering.meter_const = 2000;
    config.metering.cal_word = ND_CAL_ADD_POS | ND_CAL_ADD_NEG;
    config.metering.save_step = 200;
    config.demand.max_slide = 3;
    config.demand.sliding = 1;
    config.demand.slide_num = 3;
    config.demand.mode = 0;

    host_composition_t *composition =
        host_composition_create(nd_arena_allocator(&arena), &config);
    if (composition == NULL || host_composition_boot(composition) != 0) {
        fprintf(stderr, "host composition boot failed\n");
        return 1;
    }

    run_seconds(composition, seconds);
    if (host_composition_save(composition) != 0) {
        fprintf(stderr, "host composition save failed\n");
        return 1;
    }

    host_status_t status;
    host_composition_status(composition, &status);
    printf("fee=%u day_table=%u relay=%u demand=%" PRIu32 " energy_0_01_kwh=%" PRId64 "\n",
           status.fee, status.day_table, status.relay_connected, status.demand,
           status.energy_0_01_kwh);

    host_composition_destroy(composition, nd_arena_allocator(&arena));
    if (backup_dir != NULL) {
        nd_file_store_close(&eeprom);
        nd_file_store_close(&flash);
    }
    return 0;
}
