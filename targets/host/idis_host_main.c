#include <inttypes.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "infra/memory/file_store.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_data.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profile_table.h"
#include "targets/idis_ip_meter/cosem/nd_idis_scheduler.h"
#include "targets/idis_ip_meter/cosem/nd_measurement_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event_map.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "tests/gen/obis_audit_table.h"
#include "tests/gen/obis_audit_methods.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define EEPROM_SIZE (64u * 1024u)
#define FLASH_SIZE (4u * 1024u * 1024u)
#define EEPROM_EVENT 0u
#define EEPROM_DEMAND 1024u
#define EEPROM_DISCONNECT 2048u
#define EEPROM_MAX_DEMAND 3072u
#define EEPROM_ENERGY 4096u
#define EEPROM_CLOCK 5120u
#define EEPROM_PQ 5632u
#define EEPROM_CALENDAR 6144u
#define EEPROM_RELAYS 6656u
#define EEPROM_MBUS 7168u
#define EEPROM_PUSH 9216u
#define EEPROM_SCRIPT 15360u
#define EEPROM_SINGLE_ACTION 22528u
#define EEPROM_POWER 23552u
#define EEPROM_CALENDAR_EXTRA 32768u
#define EEPROM_MONITOR 40960u
#define EEPROM_LIMITER 42496u
#define EEPROM_IMAGE 43008u
#define EEPROM_SETUP 45056u

typedef struct {
    nd_dlms_datetime_t now;
} host_idis_clock_t;

static int host_clock_read(void *ctx, nd_dlms_datetime_t *now)
{
    host_idis_clock_t *clock = ctx;
    *now = clock->now;
    return 0;
}

static int host_clock_write(void *ctx, const nd_dlms_datetime_t *now)
{
    host_idis_clock_t *clock = ctx;
    clock->now = *now;
    return 0;
}

static void host_clock_tick(host_idis_clock_t *clock)
{
    nd_datetime_t time = {
        .second = clock->now.second,
        .minute = clock->now.minute,
        .hour = clock->now.hour,
        .day = clock->now.day,
        .month = clock->now.month,
        .year = clock->now.year,
    };
    if (nd_calendar_add_seconds(&time, 1, &time) != 0) {
        return;
    }
    clock->now.second = time.second;
    clock->now.minute = time.minute;
    clock->now.hour = time.hour;
    clock->now.day = time.day;
    clock->now.month = time.month;
    clock->now.year = time.year;
    uint8_t weekday = nd_calendar_weekday(time.day, time.month, time.year);
    clock->now.day_of_week = weekday == 0 ? 7 : weekday;
}

static nd_clock_value_t host_clock_value(const host_idis_clock_t *clock)
{
    return (nd_clock_value_t){
        .time = {
            .second = clock->now.second,
            .minute = clock->now.minute,
            .hour = clock->now.hour,
            .day = clock->now.day,
            .month = clock->now.month,
            .year = clock->now.year,
        },
        .day_of_week = clock->now.day_of_week,
        .deviation = clock->now.deviation,
        .status = clock->now.status,
    };
}

static int host_measurement_read(void *ctx, nd_measure_quantity_t quantity,
                                 uint8_t phase,
                                 nd_measurement_value_t *out)
{
    (void)ctx;
    int64_t value = 2300 + phase * 10;
    int8_t scaler = -1;
    nd_measurement_unit_t unit = ND_MEASUREMENT_UNIT_VOLT;
    if (quantity == ND_MEASURE_FREQUENCY) {
        value = 5001;
        scaler = -2;
        unit = ND_MEASUREMENT_UNIT_HERTZ;
    } else if (quantity == ND_MEASURE_CURRENT) {
        value = 100 * phase;
        scaler = -2;
        unit = ND_MEASUREMENT_UNIT_AMPERE;
    } else if (quantity == ND_MEASURE_ACTIVE_POWER) {
        value = 5000;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_POWER_FACTOR
               || quantity == ND_MEASURE_POWER_FACTOR_MIN
               || quantity == ND_MEASURE_POWER_FACTOR_AVG) {
        value = quantity == ND_MEASURE_POWER_FACTOR ? 950 : 900;
        scaler = -3;
        unit = ND_MEASUREMENT_UNIT_NONE;
    } else if (quantity == ND_MEASURE_ACTIVE_POWER_EXPORT) {
        value = 1000;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_REACTIVE_POWER_IMPORT
               || quantity == ND_MEASURE_REACTIVE_POWER_EXPORT) {
        value = 2000;
        unit = ND_MEASUREMENT_UNIT_VAR;
    } else if (quantity == ND_MEASURE_APPARENT_POWER_IMPORT
               || quantity == ND_MEASURE_APPARENT_POWER_EXPORT) {
        value = 3000;
        unit = ND_MEASUREMENT_UNIT_VA;
    } else if (quantity == ND_MEASURE_TOTAL_ACTIVE_POWER) {
        value = 6000;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_ACTIVE_POWER_NET) {
        value = 4000;
        unit = ND_MEASUREMENT_UNIT_WATT;
    } else if (quantity == ND_MEASURE_CURRENT_SUM) {
        value = 600;
        scaler = -2;
        unit = ND_MEASUREMENT_UNIT_AMPERE;
    }
    *out = (nd_measurement_value_t){
        .value = value,
        .scaler = scaler,
        .unit = unit,
        .phase = phase,
    };
    return 0;
}

static int host_mbus_read(void *ctx, uint8_t channel, nd_mbus_readout_t *out)
{
    (void)ctx;
    *out = (nd_mbus_readout_t){
        .identity = {
            .identification_number = 10000000u + channel,
            .manufacturer_id = 0x1234,
            .version = 1,
            .device_type = 3,
        },
        .time = {.year = 2026, .month = 11, .day = 11},
        .access_number = (uint8_t)(10 + channel),
        .value_count = 4,
    };
    for (uint8_t index = 0; index < out->value_count; index++) {
        out->values[index] = (nd_mbus_value_t){
            .value = 1000 + channel * 100 + index,
            .scaler = -1,
            .unit = 35, /* V; VIF mapping is exercised by the real driver */
        };
    }
    return 0;
}

static int host_mbus_write(void *ctx, uint8_t channel, const uint8_t *data,
                           size_t length)
{
    (void)ctx;
    (void)channel;
    (void)data;
    (void)length;
    return 0;
}

static int host_mbus_command(void *ctx, uint8_t channel,
                             nd_mbus_command_t command, uint8_t argument)
{
    (void)ctx;
    (void)channel;
    (void)command;
    (void)argument;
    return 0;
}

static int host_push_send(void *ctx, uint8_t setup_index,
                          const nd_push_setup_t *setup)
{
    (void)ctx;
    (void)setup_index;
    if (setup->destination.destination_length == 0
        || setup->destination.transport_service > 1) {
        return -1;
    }
    /* host socket adapter: the DataNotification APDU belongs to the wire layer. */
    char address[ND_PUSH_DESTINATION_MAX + 1] = {0};
    memcpy(address, setup->destination.destination,
           setup->destination.destination_length);
    int stream = setup->destination.transport_service == 0;
    int fd = socket(AF_INET, stream ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in peer = {.sin_family = AF_INET, .sin_port = htons(9999)};
    static const uint8_t notification[] = {0x0F, 0x00};
    int result = -1;
    if (inet_pton(AF_INET, address, &peer.sin_addr) == 1) {
        if (stream) {
            result = connect(fd, (const struct sockaddr *)&peer,
                             sizeof(peer)) < 0
                         ? -1
                         : (send(fd, notification, sizeof(notification), 0)
                                < 0 ? -1 : 0);
        } else {
            result = sendto(fd, notification, sizeof(notification), 0,
                            (const struct sockaddr *)&peer, sizeof(peer)) < 0
                         ? -1
                         : 0;
        }
    }
    close(fd);
    return result;
}

static uint8_t host_script_count(nd_idis_objects_t *objects,
                                 const nd_obis_t *ln)
{
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    if (nd_cosem_port_get(objects, 16, ND_IC_SCRIPT_TABLE, ln, 2, NULL, &out)
        != ND_OK) {
        return 0;
    }
    return buffer[1];
}

static int host_install_script_override(nd_idis_objects_t *objects)
{
    uint8_t buffer[64];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 9); /* script id */
    nd_axdr_put_array_header(&out, 1);
    nd_axdr_put_structure_header(&out, 5);
    nd_axdr_put_tag(&out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&out, 2); /* ACTION */
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, ND_IC_DISCONNECT_CONTROL);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)&ND_OBIS_DISCONNECT_CONTROL, 6);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 1); /* remote_disconnect */
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 0);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    return nd_cosem_port_set(objects, 16, ND_IC_SCRIPT_TABLE,
                             &ND_OBIS_SCRIPT_DISCONNECT, 2, &in);
}

static int parse_obis(const char *text, nd_obis_t *ln)
{
    unsigned a, b, c, d, e, f;
    if (sscanf(text, "%u-%u:%u.%u.%u.%u", &a, &b, &c, &d, &e, &f) != 6) {
        return -1;
    }
    *ln = (nd_obis_t){(uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d,
                      (uint8_t)e, (uint8_t)f};
    return 0;
}

static void usage(const char *name)
{
    printf("usage: %s [--backup-files DIR] [--seconds N] [--audit] [--no-final-save] [--start-time YYYY-MM-DDTHH:MM:SS]\n", name);
    printf("  --backup-files DIR  persist EEPROM/flash to DIR/eeprom.bin and DIR/flash.bin\n");
    printf("  --seconds N         run N one-second ticks (default 60)\n");
    printf("  --audit             GET every attribute listed in the OBIS table and exit\n");
    printf("  --no-final-save     leave without the end-of-run state save (power-loss test)\n");
    printf("  --start-time TIME   start the simulated clock at TIME\n");
}

static const char *obis_text(const nd_obis_t *ln, char *buffer,
                             size_t length)
{
    snprintf(buffer, length, "%u-%u:%u.%u.%u.%u", ln->a, ln->b, ln->c, ln->d,
             ln->e, ln->f);
    return buffer;
}

static void host_configure_push(nd_idis_objects_t *objects,
                                const nd_obis_t *ln)
{
    uint8_t push_buffer[64];
    nd_axdr_writer_t push_out;
    nd_axdr_reader_t push_in;
    nd_axdr_writer_init(&push_out, push_buffer, sizeof(push_buffer));
    nd_axdr_put_array_header(&push_out, 1);
    nd_axdr_put_structure_header(&push_out, 5);
    nd_axdr_put_tag(&push_out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&push_out, ND_IC_REGISTER);
    nd_axdr_put_tag(&push_out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&push_out, 6);
    nd_axdr_put_octets(&push_out,
                       (const uint8_t *)&ND_OBIS_ACTIVE_ENERGY_IMPORT, 6);
    nd_axdr_put_tag(&push_out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(&push_out, 2);
    nd_axdr_put_tag(&push_out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&push_out, 0);
    nd_axdr_put_structure_header(&push_out, 2);
    nd_axdr_put_tag(&push_out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&push_out, 0);
    nd_axdr_put_tag(&push_out, ND_AXDR_NULL);
    nd_axdr_reader_init(&push_in, push_buffer, push_out.len);
    (void)nd_cosem_port_set(objects, 16, ND_IC_PUSH_SETUP, ln, 2, &push_in);

    nd_axdr_writer_init(&push_out, push_buffer, sizeof(push_buffer));
    nd_axdr_put_structure_header(&push_out, 3);
    nd_axdr_put_tag(&push_out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&push_out, 1); /* UDP */
    nd_axdr_put_tag(&push_out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&push_out, 9);
    nd_axdr_put_octets(&push_out, (const uint8_t *)"192.0.2.1", 9);
    nd_axdr_put_tag(&push_out, ND_AXDR_ENUM);
    nd_axdr_put_u8(&push_out, 4); /* A-XDR encoded APDU */
    nd_axdr_reader_init(&push_in, push_buffer, push_out.len);
    (void)nd_cosem_port_set(objects, 16, ND_IC_PUSH_SETUP, ln, 3, &push_in);
}

static size_t build_method_param(uint8_t kind, uint8_t *buffer,
                                 size_t capacity)
{
    (void)capacity;
    switch (kind) {
    case 0:
        return 0;
    case 2:
    case 5:
    case 12:
        buffer[0] = ND_AXDR_UNSIGNED;
        buffer[1] = 1;
        return 2;
    case 3:
        buffer[0] = ND_AXDR_INTEGER;
        buffer[1] = 1;
        buffer[2] = ND_AXDR_INTEGER;
        buffer[3] = 1;
        return 4;
    case 4:
        buffer[0] = ND_AXDR_INTEGER;
        buffer[1] = 1;
        return 2;
    case 8:
    case 9:
    case 19: {
        buffer[0] = ND_AXDR_OCTET_STRING;
        buffer[1] = 16;
        memset(buffer + 2, 0, 16);
        return 18;
    }
    case 10:
        buffer[0] = ND_AXDR_STRUCTURE;
        buffer[1] = 4;
        buffer[2] = ND_AXDR_LONG_UNSIGNED;
        buffer[3] = 0;
        buffer[4] = ND_IC_REGISTER;
        buffer[5] = ND_AXDR_OCTET_STRING;
        buffer[6] = 6;
        memcpy(buffer + 7, &ND_OBIS_ACTIVE_ENERGY_IMPORT, 6);
        buffer[13] = ND_AXDR_INTEGER;
        buffer[14] = 2;
        buffer[15] = ND_AXDR_LONG_UNSIGNED;
        buffer[16] = 0;
        buffer[17] = 0;
        return 18;
    case 11:
        buffer[0] = ND_AXDR_STRUCTURE;
        buffer[1] = 2;
        buffer[2] = ND_AXDR_UNSIGNED;
        buffer[3] = 1;
        buffer[4] = ND_AXDR_OCTET_STRING;
        buffer[5] = 4;
        memcpy(buffer + 6, "USER", 4);
        return 10;
    case 14:
        buffer[0] = ND_AXDR_STRUCTURE;
        buffer[1] = 2;
        buffer[2] = ND_AXDR_OCTET_STRING;
        buffer[3] = 4;
        memcpy(buffer + 4, "IMG1", 4);
        buffer[8] = ND_AXDR_DOUBLE_LONG_UNSIGNED;
        buffer[9] = 0;
        buffer[10] = 0;
        buffer[11] = 2;
        buffer[12] = 0;
        return 13;
    case 15:
        buffer[0] = ND_AXDR_STRUCTURE;
        buffer[1] = 2;
        buffer[2] = ND_AXDR_DOUBLE_LONG_UNSIGNED;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = 1;
        buffer[7] = ND_AXDR_OCTET_STRING;
        buffer[8] = 4;
        memcpy(buffer + 9, "DATA", 4);
        return 13;
    case 16:
        buffer[0] = ND_AXDR_STRUCTURE;
        buffer[1] = 2;
        buffer[2] = ND_AXDR_ENUM;
        buffer[3] = 0;
        buffer[4] = ND_AXDR_OCTET_STRING;
        buffer[5] = 16;
        memset(buffer + 6, 0, 16);
        return 22;
    case 17:
        buffer[0] = ND_AXDR_OCTET_STRING;
        buffer[1] = 8;
        memset(buffer + 2, 0, 8);
        return 10;
    case 18:
        buffer[0] = ND_AXDR_ARRAY;
        buffer[1] = 0;
        return 2;
    case 20:
    case 21: {
        const char *name = kind == 20 ? "LPCAP_1" : "LPCAP_M1";
        uint8_t name_len = kind == 20 ? 7 : 8;
        buffer[0] = ND_AXDR_ARRAY;
        buffer[1] = 1;
        buffer[2] = ND_AXDR_STRUCTURE;
        buffer[3] = 2;
        buffer[4] = ND_AXDR_VISIBLE_STRING;
        buffer[5] = name_len;
        memcpy(buffer + 6, name, name_len);
        buffer[6 + name_len] = ND_AXDR_BOOLEAN;
        buffer[7 + name_len] = 1;
        return (size_t)(8 + name_len);
    }
    default:
        buffer[0] = ND_AXDR_INTEGER;
        buffer[1] = 0;
        return 2;
    }
}


static int parse_start_time(const char *text, nd_dlms_datetime_t *out)
{
    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    if (sscanf(text, "%u-%u-%uT%u:%u:%u", &year, &month, &day, &hour, &minute,
               &second) != 6) {
        return -1;
    }
    nd_datetime_t value = {
        .year = (uint16_t)year,
        .month = (uint8_t)month,
        .day = (uint8_t)day,
        .hour = (uint8_t)hour,
        .minute = (uint8_t)minute,
        .second = (uint8_t)second,
    };
    if (!nd_calendar_date_valid(&value) || !nd_calendar_time_valid(&value)) {
        return -1;
    }
    *out = (nd_dlms_datetime_t){
        .year = value.year,
        .month = value.month,
        .day = value.day,
        .hour = value.hour,
        .minute = value.minute,
        .second = value.second,
        .day_of_week = nd_calendar_weekday(value.day, value.month, value.year),
    };
    return 0;
}

int main(int argc, char **argv)
{
    const char *backup_dir = NULL;
    unsigned seconds = 60;
    int audit = 0;
    int final_save = 1;
    nd_dlms_datetime_t start_time = {0};
    int have_start_time = 0;
    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--backup-files") == 0 && index + 1 < argc) {
            backup_dir = argv[++index];
        } else if (strcmp(argv[index], "--seconds") == 0 && index + 1 < argc) {
            seconds = (unsigned)strtoul(argv[++index], NULL, 10);
        } else if (strcmp(argv[index], "--audit") == 0) {
            audit = 1;
        } else if (strcmp(argv[index], "--no-final-save") == 0) {
            final_save = 0;
        } else if (strcmp(argv[index], "--start-time") == 0
                   && index + 1 < argc) {
            if (parse_start_time(argv[++index], &start_time) != 0) {
                usage(argv[0]);
                return 2;
            }
            have_start_time = 1;
        } else if (strcmp(argv[index], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    nd_file_store_t eeprom;
    nd_file_store_t flash;
    char eeprom_path[512];
    char flash_path[512];
    if (backup_dir != NULL) {
        snprintf(eeprom_path, sizeof(eeprom_path), "%s/eeprom.bin", backup_dir);
        snprintf(flash_path, sizeof(flash_path), "%s/flash.bin", backup_dir);
        if (nd_file_store_open(&eeprom, eeprom_path, EEPROM_SIZE) != 0
            || nd_file_store_open(&flash, flash_path, FLASH_SIZE) != 0) {
            fprintf(stderr, "cannot open backup files in %s\n", backup_dir);
            return 1;
        }
        printf("backup: %s, %s\n", eeprom_path, flash_path);
    } else {
        fprintf(stderr, "--backup-files DIR is required for the IDIS host\n");
        return 2;
    }

    host_idis_clock_t clock_state = {
        .now = {.year = 2026, .month = 11, .day = 11, .hour = 0, .minute = 0,
                .second = 0},
    };
    if (have_start_time) {
        clock_state.now = start_time;
    }
    nd_clock_port_t clock_port = {
        .ctx = &clock_state,
        .read = host_clock_read,
        .write = host_clock_write,
    };
    nd_measurement_port_t measurement = {
        .ctx = NULL,
        .read = host_measurement_read,
    };

    nd_idis_objects_t objects = {0};
    nd_idis_clock_init(&objects.clock, &clock_port);
    objects.reg.measurement = &measurement;
    objects.reg.energy = &objects.energy;
    objects.reg.average = &objects.average;
    nd_energy_set_init(&objects.energy, 2000);
    nd_idis_average_init(&objects.average, &measurement);
    nd_pq_config_t pq_config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 50,
        .debounce_seconds = 1,
        .phase_count = 3,
    };
    nd_idis_pq_init(&objects.pq, &pq_config);
    nd_power_failure_init(&objects.power_failure, 60);
    (void)nd_energy_set_add_pulses(&objects.energy, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                   1, 4000);
    objects.demand[0].current = (nd_demand_snapshot_t){
        .current_value = 100,
        .current_fee_value = 200,
        .period_seconds = 900,
        .periods = 15,
    };
    objects.demand[0].history = (nd_demand_history_t){
        .last_value = 90,
        .last_fee_value = 180,
        .status = 0,
        .last_capture_time = {.year = 2026, .month = 11, .day = 11},
        .period_start = {.year = 2026, .month = 11, .day = 11},
    };
    for (uint8_t group = 1; group < ND_IDIS_DEMAND_MAX; group++) {
        objects.demand[group].current = (nd_demand_snapshot_t){
            .current_value = 100 + group * 10,
            .current_fee_value = 200 + group * 10,
            .period_seconds = 900,
            .periods = 15,
        };
        objects.demand[group].history = (nd_demand_history_t){
            .last_value = 90 + group * 10,
            .last_fee_value = 180,
            .status = 0,
            .last_capture_time = {.year = 2026, .month = 11, .day = 11},
            .period_start = {.year = 2026, .month = 11, .day = 11},
        };
    }
    objects.max_demand[0] = (nd_idis_extended_state_t){
        .value = 110,
        .status = 0,
        .capture_time = {.year = 2026, .month = 11, .day = 11},
    };
    nd_disconnect_init(&objects.disconnect, 2, NULL);
    nd_disconnect_init(&objects.load_control_1, 2, NULL);
    nd_disconnect_init(&objects.load_control_2, 2, NULL);
    for (uint8_t relay = 0; relay < 4; relay++) {
        nd_disconnect_init(&objects.mbus_disconnect[relay], 2, NULL);
    }
    nd_ic_register_activation_init(&objects.activation);
    nd_ic_function_control_init(&objects.functions);
    nd_idis_push_init(&objects.push);
    nd_push_port_t push_port = {.ctx = NULL, .send = host_push_send};
    nd_push_bind_port(&objects.push.state, &push_port);
    nd_idis_mbus_init(&objects.mbus);
    nd_mbus_port_t mbus_port = {
        .ctx = NULL,
        .read = host_mbus_read,
        .write = host_mbus_write,
        .command = host_mbus_command,
    };
    nd_mbus_bind_port(&objects.mbus.state, &mbus_port);
    nd_idis_calendar_init(&objects.calendar);
    nd_idis_calendar_apply_defaults(&objects.calendar, &ND_OBIS_SCRIPT_TARIFF, 1);
    for (uint8_t index = 0; index < 3; index++) {
        nd_idis_calendar_init(&objects.calendar_extra[index]);
    }
    (void)nd_ic_demand_load(objects.demand, nd_file_store_port(&eeprom),
                            EEPROM_DEMAND);
    (void)nd_ic_disconnect_load(&objects.disconnect,
                                nd_file_store_port(&eeprom),
                                EEPROM_DISCONNECT);
    (void)nd_ic_extended_load(objects.max_demand, nd_file_store_port(&eeprom),
                              EEPROM_MAX_DEMAND);
    (void)nd_idis_energy_load(&objects.energy, nd_file_store_port(&eeprom),
                              EEPROM_ENERGY);
    (void)nd_idis_clock_load(&objects.clock, nd_file_store_port(&eeprom),
                             EEPROM_CLOCK);
    (void)nd_idis_pq_load(&objects.pq, nd_file_store_port(&eeprom), EEPROM_PQ);
    (void)nd_idis_power_load(&objects.power_failure,
                             nd_file_store_port(&eeprom), EEPROM_POWER);
    (void)nd_idis_calendar_load(&objects.calendar, nd_file_store_port(&eeprom),
                                EEPROM_CALENDAR);
    for (uint8_t index = 0; index < 3; index++) {
        (void)nd_idis_calendar_load(&objects.calendar_extra[index],
                                    nd_file_store_port(&eeprom),
                                    EEPROM_CALENDAR_EXTRA + index * 2048u);
    }
    (void)nd_idis_mbus_load(&objects.mbus, nd_file_store_port(&eeprom),
                            EEPROM_MBUS);
    if (objects.ldn_len == 0) {
        memcpy(objects.ldn, "0000000000000001", 16);
        objects.ldn_len = 16;
    }
    for (uint8_t index = 0; index < 6; index++) {
        if (objects.device_id_len[index] == 0) {
            snprintf((char *)objects.device_id[index],
                     sizeof(objects.device_id[index]), "DEV%05u", index + 1);
            objects.device_id_len[index] = 8;
        }
    }
    for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
        for (uint8_t id = 0; id < 2; id++) {
            if (objects.mbus.device_id_len[channel][id] == 0) {
                snprintf((char *)objects.mbus.device_id[channel][id],
                         sizeof(objects.mbus.device_id[channel][id]),
                         "MB%u-%u", channel + 1, id);
                objects.mbus.device_id_len[channel][id] = 4;
            }
        }
    }
    (void)nd_idis_push_load(&objects.push, nd_file_store_port(&eeprom),
                            EEPROM_PUSH);
    /* Live demand values do not come from the backup. */
    for (uint8_t group = 0; group < ND_IDIS_DEMAND_MAX; group++) {
        objects.demand[group].current.current_value = 100 + group * 10;
        objects.demand[group].current.current_fee_value = 200 + group * 10;
        objects.demand[group].current.period_seconds = 900;
        objects.demand[group].current.periods = 15;
    }
    nd_disconnect_snapshot_t relays[6] = {
        objects.load_control_1, objects.load_control_2,
        objects.mbus_disconnect[0], objects.mbus_disconnect[1],
        objects.mbus_disconnect[2], objects.mbus_disconnect[3],
    };
    (void)nd_ic_relays_load(relays, nd_file_store_port(&eeprom), EEPROM_RELAYS);
    objects.load_control_1 = relays[0];
    objects.load_control_2 = relays[1];
    for (uint8_t relay = 0; relay < 4; relay++) {
        objects.mbus_disconnect[relay] = relays[2 + relay];
    }

    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    int profile_count = nd_idis_profile_table_build(
        configs, ND_IDIS_PROFILE_TABLE_COUNT, nd_file_store_port(&flash),
        &clock_port);
    if (profile_count < 0
        || nd_idis_profiles_init(&objects.profiles, configs,
                                 (uint8_t)profile_count) != 0) {
        fprintf(stderr, "profile init failed\n");
        return 1;
    }

    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects.profiles, &port);
    nd_idis_single_action_init(&objects.single_action);
    nd_idis_monitors_init(&objects.monitors);
    nd_idis_limiter_init(&objects.limiter);
    nd_idis_image_init(&objects.image);
    nd_idis_setup_init(&objects.setup);
    nd_idis_association_init(&objects.association);
    for (uint8_t index = 0; index < 3; index++) {
        snprintf((char *)objects.firmware_id[index],
                 sizeof(objects.firmware_id[index]), "FW-%u.0.0", index + 1);
        objects.firmware_id_len[index] = 8;
        snprintf((char *)objects.firmware_sig[index],
                 sizeof(objects.firmware_sig[index]), "SIG-%u", index + 1);
        objects.firmware_sig_len[index] = 5;
    }
    nd_idis_script_init(&objects.scripts);
    nd_idis_script_bind_port(&objects.scripts, &port);
    nd_idis_script_bind_store(&objects.scripts, nd_file_store_port(&eeprom),
                              EEPROM_SCRIPT);
    (void)nd_idis_script_load(&objects.scripts, nd_file_store_port(&eeprom),
                              EEPROM_SCRIPT);
    (void)nd_idis_single_action_load(&objects.single_action,
                                     nd_file_store_port(&eeprom),
                                     EEPROM_SINGLE_ACTION);
    nd_idis_single_action_apply_defaults(&objects.single_action);
    (void)nd_idis_monitor_load(&objects.monitors, nd_file_store_port(&eeprom),
                               EEPROM_MONITOR);
    (void)nd_idis_limiter_load(&objects.limiter, nd_file_store_port(&eeprom),
                               EEPROM_LIMITER);
    (void)nd_idis_image_load(&objects.image, nd_file_store_port(&eeprom),
                             EEPROM_IMAGE);
    nd_idis_image_bind_store(&objects.image, nd_file_store_port(&flash),
                             1048576u);
    (void)nd_idis_setup_load(&objects.setup, nd_file_store_port(&eeprom),
                             EEPROM_SETUP);
    /* Install one script override on first boot; later runs load it. */
    if (host_script_count(&objects, &ND_OBIS_SCRIPT_DISCONNECT) != 1) {
        (void)host_install_script_override(&objects);
    }
    nd_idis_scheduler_init(&objects.scheduler);
    objects.reg.clock = &objects.clock;
    objects.reg.pq = &objects.pq;
    objects.reg.power = &objects.power_failure;

    nd_idis_event_init(&objects.event, nd_idis_event_rules,
                       nd_idis_event_rule_count);
    (void)nd_idis_event_load(&objects.event, nd_file_store_port(&eeprom),
                             EEPROM_EVENT);
    nd_idis_event_bind_log(
        &objects.event,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_STANDARD_EVENT_LOG),
        &port, &clock_port, 16);
    nd_idis_event_bind_category_log(
        &objects.event, ND_IDIS_EVENT_LOG_FRAUD,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_FRAUD_LOG));
    nd_idis_event_bind_category_log(
        &objects.event, ND_IDIS_EVENT_LOG_DISCONNECTOR,
        nd_idis_profiles_find(&objects.profiles,
                              &ND_OBIS_DISCONNECTOR_LOG));
    nd_idis_event_bind_category_log(
        &objects.event, ND_IDIS_EVENT_LOG_MBUS,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_MBUS_EVENT_LOG));
    nd_idis_event_bind_category_log(
        &objects.event, ND_IDIS_EVENT_LOG_PQ,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_PQ_LOG));
    nd_idis_event_bind_category_log(
        &objects.event, ND_IDIS_EVENT_LOG_COMMUNICATION,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_COMM_LOG));
    nd_idis_event_bind_category_log(
        &objects.event, ND_IDIS_EVENT_LOG_POWER_FAILURE,
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_POWER_FAILURE_LOG));
    nd_idis_event_bind_profiles(&objects.event, &objects.profiles);
    static const nd_obis_t demand_objects[ND_IDIS_DEMAND_MAX] = {
        {1, 0, 1, 4, 0, 255}, {1, 0, 2, 4, 0, 255},
        {1, 0, 3, 4, 0, 255}, {1, 0, 4, 4, 0, 255},
        {1, 0, 9, 4, 0, 255}, {1, 0, 10, 4, 0, 255},
    };
    static const nd_measure_quantity_t demand_quantities[ND_IDIS_DEMAND_MAX] = {
        ND_MEASURE_ACTIVE_POWER,          ND_MEASURE_ACTIVE_POWER_EXPORT,
        ND_MEASURE_REACTIVE_POWER_IMPORT, ND_MEASURE_REACTIVE_POWER_EXPORT,
        ND_MEASURE_APPARENT_POWER_IMPORT, ND_MEASURE_APPARENT_POWER_EXPORT,
    };
    static const nd_obis_t push_setups[8] = {
        ND_OBIS_PUSH_0, ND_OBIS_PUSH_1, ND_OBIS_PUSH_2, ND_OBIS_PUSH_3,
        ND_OBIS_PUSH_4, ND_OBIS_PUSH_5, ND_OBIS_PUSH_6, ND_OBIS_PUSH_7};
    for (uint8_t index = 0; index < 8; index++) {
        host_configure_push(&objects, &push_setups[index]);
    }

    for (unsigned second = 0; second < seconds; second++) {
        host_clock_tick(&clock_state);
        nd_clock_value_t now = host_clock_value(&clock_state);
        (void)nd_idis_average_on_second(&objects.average);
        (void)nd_idis_pq_on_second(&objects.pq, &measurement, &objects.event,
                                   &now.time);
        (void)nd_idis_calendar_tick(&objects.calendar, &now.time);
        for (uint8_t index = 0; index < 3; index++) {
            (void)nd_idis_calendar_tick(&objects.calendar_extra[index],
                                        &now.time);
        }
        const nd_idis_calendar_state_t *calendars[4] = {
            &objects.calendar, &objects.calendar_extra[0],
            &objects.calendar_extra[1], &objects.calendar_extra[2]};
        (void)nd_idis_scheduler_on_second_multi(
            &objects.scheduler, calendars, 4, &objects.scripts, &port, 16,
            &now.time);
        (void)nd_push_on_second(&objects.push.state, &now.time);
        (void)nd_idis_single_action_on_second(&objects.single_action, &port,
                                              &objects.scripts, 16,
                                              &now.time);
        (void)nd_idis_monitor_on_second(&objects.monitors, &port,
                                        &objects.scripts, 16);
        (void)nd_idis_limiter_on_second(&objects.limiter, &port,
                                        &objects.scripts, 16);
        for (uint8_t group = 0; group < ND_IDIS_DEMAND_MAX; group++) {
            nd_measurement_value_t power;
            if (measurement.read(measurement.ctx, demand_quantities[group], 0,
                                 &power) == 0) {
                nd_idis_demand_accumulate(&objects.demand_acc[group],
                                          (int32_t)power.value);
            }
        }
        /* Simulated meter input: one active-import pulse on the active tariff. */
        (void)nd_energy_set_add_pulses(
            &objects.energy, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
            nd_ic_register_activation_tariff(&objects.activation), 1);
        if (second == 30) {
            (void)nd_idis_script_execute(&objects.scripts, &port, 16,
                                         &ND_OBIS_SCRIPT_TARIFF, 2);
        }
        for (uint8_t channel = 1; channel <= ND_MBUS_CHANNELS; channel++) {
            (void)nd_idis_mbus_on_second(&objects.mbus, channel);
        }
        if (nd_idis_profiles_on_second(&objects.profiles, &port, 16, &now) < 0) {
            fprintf(stderr, "profile capture failed\n");
            return 1;
        }
        if (second == 5) {
            (void)nd_idis_event_raise(&objects.event, 7);
            (void)nd_power_failure_off(&objects.power_failure, &now.time);
        }
        if (second == 10) {
            (void)nd_power_failure_on(&objects.power_failure, &now.time);
        }
    }

    /* shortcut: close the demand period at end of run instead of 900 s. */
    /* Close the demand period and capture all groups. */
    for (uint8_t group = 0; group < ND_IDIS_DEMAND_MAX; group++) {
        (void)nd_cosem_port_action(&objects, 16, ND_IC_DEMAND_REGISTER,
                                   &demand_objects[group], 2, NULL, NULL);
    }

    nd_idis_profile_state_t *billing =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_BILLING_PROFILE);
    nd_clock_value_t now = host_clock_value(&clock_state);
    if (billing != NULL) {
        (void)nd_idis_profile_capture(billing, &port, 16, &now);
    }

    /* Install the four M-Bus slaves (once), capture their values through the
     * IC72 method and store one M-Bus load profile record per channel. */
    static const nd_obis_t mbus_clients[ND_MBUS_CHANNELS] = {
        {0, 1, 24, 1, 0, 255},
        {0, 2, 24, 1, 0, 255},
        {0, 3, 24, 1, 0, 255},
        {0, 4, 24, 1, 0, 255},
    };
    static const nd_obis_t mbus_profiles[ND_MBUS_CHANNELS] = {
        {0, 1, 24, 3, 0, 255},
        {0, 2, 24, 3, 0, 255},
        {0, 3, 24, 3, 0, 255},
        {0, 4, 24, 3, 0, 255},
    };
    uint8_t capture_parameter[2] = {ND_AXDR_INTEGER, 0};
    for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
        if (objects.mbus.state.clients[channel].primary_address == 0) {
            (void)nd_mbus_install(&objects.mbus.state, (uint8_t)(channel + 1),
                                  (uint8_t)(channel + 1));
        }
        nd_axdr_reader_t in;
        nd_axdr_reader_init(&in, capture_parameter, sizeof(capture_parameter));
        (void)nd_cosem_port_action(&objects, 16, ND_IC_MBUS_CLIENT,
                                   &mbus_clients[channel], 3, &in, NULL);
        nd_idis_profile_state_t *profile = nd_idis_profiles_find(
            &objects.profiles, &mbus_profiles[channel]);
        if (profile != NULL) {
            (void)nd_idis_profile_capture(profile, &port, 16, &now);
        }
    }
    uint8_t push_parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t push_in;
    nd_axdr_reader_init(&push_in, push_parameter, sizeof(push_parameter));
    (void)nd_cosem_port_action(&objects, 16, ND_IC_PUSH_SETUP, &ND_OBIS_PUSH_1,
                               1, &push_in, NULL);

    if (audit) {
        static uint8_t buffer[4096];
        char script_buf[32];
        size_t ok = 0;
        size_t fail = 0;
        for (size_t index = 0; index < OBIS_AUDIT_COUNT; index++) {
            nd_obis_t ln;
            if (parse_obis(obis_audit_entries[index].ln, &ln) != 0) {
                continue;
            }
            nd_axdr_writer_t out;
            nd_axdr_writer_init(&out, buffer, sizeof(buffer));
            int result = nd_cosem_port_get(
                &objects, 16, obis_audit_entries[index].ic, &ln,
                obis_audit_entries[index].attr, NULL, &out);
            if (result == ND_OK) {
                ok++;
            } else {
                fail++;
            }
            printf("AUDIT %u %s %u %d\n", obis_audit_entries[index].ic,
                   obis_audit_entries[index].ln, obis_audit_entries[index].attr,
                   result);
        }
        printf("AUDIT SUMMARY ok=%zu fail=%zu total=%zu\n", ok, fail,
               ok + fail);
        nd_disconnect_init(&objects.disconnect, 2, NULL);
        nd_disconnect_init(&objects.load_control_1, 2, NULL);
        nd_disconnect_init(&objects.load_control_2, 2, NULL);
        for (uint8_t relay = 0; relay < 4; relay++) {
            nd_disconnect_init(&objects.mbus_disconnect[relay], 2, NULL);
        }
        objects.calendar.calendar.has_passive = 1;
        objects.calendar.passive = objects.calendar.active;
        for (uint8_t extra = 0; extra < 3; extra++) {
            objects.calendar_extra[extra].calendar.has_passive = 1;
            objects.calendar_extra[extra].passive =
                objects.calendar_extra[extra].active;
        }
        size_t mok = 0;
        size_t mfail = 0;
        for (size_t index = 0; index < OBIS_METHOD_COUNT; index++) {
            nd_obis_t ln;
            if (parse_obis(obis_method_entries[index].ln, &ln) != 0) {
                continue;
            }
            uint8_t parameter[64];
            size_t parameter_len;
            if (obis_method_entries[index].ic == ND_IC_SCRIPT_TABLE) {
                int script = nd_idis_script_id(&objects.scripts, &ln, 0);
                parameter[0] = ND_AXDR_UNSIGNED;
                parameter[1] = (uint8_t)script;
                parameter_len = 2;
            } else {
                parameter_len = build_method_param(
                    obis_method_entries[index].param_kind, parameter,
                    sizeof(parameter));
            }
            nd_axdr_reader_t in;
            nd_axdr_reader_init(&in, parameter, parameter_len);
            uint8_t result_buffer[256];
            nd_axdr_writer_t out;
            nd_axdr_writer_init(&out, result_buffer, sizeof(result_buffer));
            int result = nd_cosem_port_action(
                &objects, 16, obis_method_entries[index].ic, &ln,
                obis_method_entries[index].method, &in, &out);
            if (result == ND_OK) {
                mok++;
            } else {
                mfail++;
            }
            printf("METHOD %u %s %u %d\n", obis_method_entries[index].ic,
                   obis_method_entries[index].ln,
                   obis_method_entries[index].method, result);
        }
        printf("METHOD SUMMARY ok=%zu other=%zu total=%zu\n", mok, mfail,
               mok + mfail);
        const nd_obis_t *script_tables[] = {
            &ND_OBIS_SCRIPT_TARIFF, &ND_OBIS_SCRIPT_MDI,
            &ND_OBIS_SCRIPT_DISCONNECT, &ND_OBIS_SCRIPT_LOAD_MGMT,
            &ND_OBIS_SCRIPT_IMAGE, &ND_OBIS_SCRIPT_PUSH,
            &ND_OBIS_SCRIPT_FUNCTION_CONTROL,
            &ND_OBIS_SCRIPT_MBUS_DISCONNECT,
            &ND_OBIS_SCRIPT_MBUS_FUNCTION_CONTROL,
            &ND_OBIS_SCRIPT_MBUS_PROFILE_CONTROL};
        nd_disconnect_init(&objects.disconnect, 2, NULL);
        nd_disconnect_init(&objects.load_control_1, 2, NULL);
        nd_disconnect_init(&objects.load_control_2, 2, NULL);
        for (uint8_t relay = 0; relay < 4; relay++) {
            nd_disconnect_init(&objects.mbus_disconnect[relay], 2, NULL);
        }
        size_t sok = 0;
        size_t sfail = 0;
        for (size_t table = 0;
             table < sizeof(script_tables) / sizeof(script_tables[0]);
             table++) {
            int count = nd_idis_script_count(&objects.scripts,
                                             script_tables[table]);
            for (int index = 0; index < count; index++) {
                int id = nd_idis_script_id(&objects.scripts,
                                           script_tables[table],
                                           (uint8_t)index);
                int result = nd_idis_script_execute(
                    &objects.scripts, &port, 16, script_tables[table],
                    (uint16_t)id);
                if (result == ND_OK) {
                    sok++;
                } else {
                    sfail++;
                }
                printf("SCRIPT %s %d %d\n",
                       obis_text(script_tables[table], script_buf,
                                 sizeof(script_buf)),
                       id, result);
            }
        }
        printf("SCRIPT SUMMARY ok=%zu fail=%zu total=%zu\n", sok, sfail,
               sok + sfail);
        return 0;
    }

    relays[0] = objects.load_control_1;
    relays[1] = objects.load_control_2;
    for (uint8_t relay = 0; relay < 4; relay++) {
        relays[2 + relay] = objects.mbus_disconnect[relay];
    }
    if (final_save) {
    nd_idis_profiles_sync(&objects.profiles);
    if (nd_idis_event_save(&objects.event, nd_file_store_port(&eeprom),
                           EEPROM_EVENT) != 0
        || nd_ic_demand_save(objects.demand, nd_file_store_port(&eeprom),
                             EEPROM_DEMAND) != 0
        || nd_ic_disconnect_save(&objects.disconnect,
                                 nd_file_store_port(&eeprom),
                                 EEPROM_DISCONNECT) != 0
        || nd_ic_extended_save(objects.max_demand,
                               nd_file_store_port(&eeprom),
                               EEPROM_MAX_DEMAND) != 0
        || nd_idis_energy_save(&objects.energy, nd_file_store_port(&eeprom),
                               EEPROM_ENERGY) != 0
        || nd_idis_clock_save(&objects.clock, nd_file_store_port(&eeprom),
                              EEPROM_CLOCK) != 0
        || nd_idis_pq_save(&objects.pq, nd_file_store_port(&eeprom),
                           EEPROM_PQ) != 0
        || nd_idis_calendar_save(&objects.calendar,
                                 nd_file_store_port(&eeprom),
                                 EEPROM_CALENDAR) != 0
        || nd_idis_calendar_save(&objects.calendar_extra[0],
                                 nd_file_store_port(&eeprom),
                                 EEPROM_CALENDAR_EXTRA) != 0
        || nd_idis_calendar_save(&objects.calendar_extra[1],
                                 nd_file_store_port(&eeprom),
                                 EEPROM_CALENDAR_EXTRA + 2048u) != 0
        || nd_idis_calendar_save(&objects.calendar_extra[2],
                                 nd_file_store_port(&eeprom),
                                 EEPROM_CALENDAR_EXTRA + 4096u) != 0
        || nd_ic_relays_save(relays, nd_file_store_port(&eeprom),
                             EEPROM_RELAYS) != 0
        || nd_idis_mbus_save(&objects.mbus, nd_file_store_port(&eeprom),
                             EEPROM_MBUS) != 0
        || nd_idis_push_save(&objects.push, nd_file_store_port(&eeprom),
                             EEPROM_PUSH) != 0
        || nd_idis_script_save(&objects.scripts, nd_file_store_port(&eeprom),
                               EEPROM_SCRIPT) != 0
        || nd_idis_single_action_save(&objects.single_action,
                                      nd_file_store_port(&eeprom),
                                      EEPROM_SINGLE_ACTION) != 0
        || nd_idis_power_save(&objects.power_failure,
                              nd_file_store_port(&eeprom),
                              EEPROM_POWER) != 0
        || nd_idis_monitor_save(&objects.monitors,
                                nd_file_store_port(&eeprom),
                                EEPROM_MONITOR) != 0
        || nd_idis_limiter_save(&objects.limiter,
                                nd_file_store_port(&eeprom),
                                EEPROM_LIMITER) != 0
        || nd_idis_image_save(&objects.image, nd_file_store_port(&eeprom),
                              EEPROM_IMAGE) != 0
        || nd_idis_setup_save(&objects.setup, nd_file_store_port(&eeprom),
                              EEPROM_SETUP) != 0) {
        fprintf(stderr, "state save failed\n");
        return 1;
    }
    }

    printf("profiles=%u billing=%u lp1=%u lp2=%u pq=%u event_log=%u "
           "event=%u counter=%" PRIu32 " ec2=%" PRIu32
           " alarm=0x%08" PRIX32 " scripts=%" PRIu32 "\n",
           objects.profiles.count,
           nd_profile_store_entries(&objects.profiles.items[0].store),
           nd_profile_store_entries(&objects.profiles.items[1].store),
           nd_profile_store_entries(&objects.profiles.items[2].store),
           nd_profile_store_entries(&objects.profiles.items[3].store),
           nd_profile_store_entries(&objects.profiles.items[4].store),
           objects.event.latest_event_code, objects.event.counters[0],
           objects.event.counters[1], nd_alarm_bits(&objects.event.alarm, 0),
           objects.scheduler.executed);
    printf("max_demand=%u,%u,%u,%u rate2=%u\n",
           objects.max_demand[0].value, objects.max_demand[5].value,
           objects.max_demand[10].value, objects.max_demand[15].value,
           objects.max_demand[2].value);
    uint8_t mbus_installed = 0;
    for (uint8_t channel = 0; channel < ND_MBUS_CHANNELS; channel++) {
        if (objects.mbus.state.clients[channel].primary_address != 0) {
            mbus_installed++;
        }
    }
    printf("energy_raw=%lld tariff1=%lld tariff2=%lld\n",
           (long long)nd_energy_set_read_raw(&objects.energy,
                                             ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                             0),
           (long long)nd_energy_set_read_raw(&objects.energy,
                                             ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                             1),
           (long long)nd_energy_set_read_raw(&objects.energy,
                                             ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                             2));
    printf("mbus=%u values=%u\n", mbus_installed,
           objects.mbus.values[0][0].value);
    printf("push=%" PRIu32 "\n", objects.push.state.setups[1].pushes);
    printf("script=%u\n",
           host_script_count(&objects, &ND_OBIS_SCRIPT_DISCONNECT));
    printf("power=%u/%u\n", objects.power_failure.count,
           objects.power_failure.last_duration);

    nd_file_store_close(&eeprom);
    nd_file_store_close(&flash);
    return 0;
}
