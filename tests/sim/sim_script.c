#include "sim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu_surport.h"
#include "domain/common/status.h"
#include "kernel/legacy_control_block.h"
#include "nidavellir.h"
#include "infra/boards/stategrid_single_phase/meter_config.h"
#include "infra/boards/stategrid_single_phase/uart_driver.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

#define SIM_SCENARIO_MAX_EVENTS 64
#define SIM_SCENARIO_MAX_UART 544
#define SIM_UART_CHANNELS 3

/* The firmware's pulse accumulator drains only while the count is below 64. */
#define SIM_PULSE_MAX 64

enum sim_scenario_kind {
    SIM_SCENARIO_POWER,
    SIM_SCENARIO_UART,
    SIM_SCENARIO_RELAY_FEEDBACK,
    SIM_SCENARIO_COVER,
    SIM_SCENARIO_KEY,
    SIM_SCENARIO_PULSE,
    SIM_SCENARIO_METER,
    SIM_SCENARIO_TIME,
    SIM_SCENARIO_ENERGY_CLEAR,
    SIM_SCENARIO_CHIP,
    SIM_SCENARIO_ADC,
};

enum sim_meter_field {
    SIM_METER_VOLTAGE = 1u << 0,
    SIM_METER_CURRENT = 1u << 1,
    SIM_METER_POWER = 1u << 2,
    SIM_METER_CONST = 1u << 3,
    SIM_METER_CAL = 1u << 4,
    SIM_METER_DIRECTION = 1u << 5,
    SIM_METER_TARIFF = 1u << 6,
};

typedef struct {
    unsigned cycle;
    uint8_t kind;
    uint8_t value;
    uint8_t channel;
    uint8_t count;
    uint8_t meter_mask;
    uint8_t meter_cal;
    uint8_t meter_direction;
    uint8_t meter_tariff;
    uint16_t len;
    int32_t meter[3];
    uint32_t meter_const;
    struct tm when;
    uint8_t data[SIM_SCENARIO_MAX_UART];
} sim_scenario_event_t;

static sim_scenario_event_t scenario_events[SIM_SCENARIO_MAX_EVENTS];
static size_t scenario_event_count;

static uint8_t scenario_power_on = 1;
static uint8_t scenario_relay_off = 1;
static uint8_t scenario_cover_closed = 1;
static uint8_t scenario_key_pressed;

static struct instance_t *scenario_instance;
static unsigned scenario_cycle;

static struct fifo_t *uart_tx_fifo[SIM_UART_CHANNELS];
static struct fifo_t *uart_rx_fifo[SIM_UART_CHANNELS];

static FILE *uart_tx_file;
static FILE *uart_tx_hex_file;
static FILE *relay_log_file;
static FILE *backlight_log_file;
static FILE *lcd_log_file;

extern void Energy_Pluse_Add(void);
extern void Energy_clr(struct instance_t *instance);

static int scenario_error(const char *path, unsigned line, const char *message)
{
    fprintf(stderr, "firmware_sim: %s:%u: %s\n", path, line, message);
    return -1;
}

static char *next_token(char **cursor)
{
    char *p = *cursor;
    char *start;

    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0' || *p == '#') {
        *cursor = p;
        return NULL;
    }
    start = p;
    while ((*p != '\0') && (*p != ' ') && (*p != '\t') && (*p != '#')) {
        p++;
    }
    if (*p != '\0') {
        *p++ = '\0';
    }
    *cursor = p;
    return start;
}

static int parse_on_off(const char *token, uint8_t *value)
{
    if (strcmp(token, "on") == 0) {
        *value = 1;
        return 0;
    }
    if (strcmp(token, "off") == 0) {
        *value = 0;
        return 0;
    }
    return -1;
}

static int parse_long_token(const char *token, long minimum, long maximum,
                            long *value)
{
    char *end;
    long parsed = strtol(token, &end, 10);

    if ((*token == '\0') || (*end != '\0') || (parsed < minimum)
        || (parsed > maximum)) {
        return -1;
    }
    *value = parsed;
    return 0;
}

static int hex_nibble(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'F')) {
        return c - 'A' + 10;
    }
    return -1;
}

static int parse_hex_token(const char *token, uint8_t *data, uint16_t *len)
{
    size_t digits = strlen(token);

    if ((digits == 0) || ((digits % 2) != 0) || (*len + (digits / 2) > SIM_SCENARIO_MAX_UART)) {
        return -1;
    }
    for (size_t i = 0; i < digits; i += 2) {
        int high = hex_nibble(token[i]);
        int low = hex_nibble(token[i + 1]);

        if ((high < 0) || (low < 0)) {
            return -1;
        }
        data[(*len)++] = (uint8_t)((high << 4) | low);
    }
    return 0;
}

int sim_scenario_load(const char *path)
{
    FILE *file;
    char line[1024];
    unsigned line_number = 0;

    if (path == NULL) {
        return 0;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        sim_scenario_event_t *event;
        char *cursor = line;
        char *token;
        char *end;
        unsigned long cycle;

        line_number++;
        if ((strchr(line, '\n') == NULL) && !feof(file)) {
            fclose(file);
            return scenario_error(path, line_number, "line is too long");
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (scenario_event_count >= SIM_SCENARIO_MAX_EVENTS) {
            fclose(file);
            return scenario_error(path, line_number, "too many scenario events");
        }

        token = next_token(&cursor);
        if ((token == NULL) || (strcmp(token, "at") != 0)) {
            fclose(file);
            return scenario_error(path, line_number, "expected 'at <cycle>'");
        }
        token = next_token(&cursor);
        if (token == NULL) {
            fclose(file);
            return scenario_error(path, line_number, "missing cycle");
        }
        cycle = strtoul(token, &end, 10);
        if ((*token == '\0') || (*end != '\0')) {
            fclose(file);
            return scenario_error(path, line_number, "cycle is not a number");
        }

        event = &scenario_events[scenario_event_count];
        memset(event, 0, sizeof(*event));
        event->cycle = (unsigned)cycle;

        token = next_token(&cursor);
        if (token == NULL) {
            fclose(file);
            return scenario_error(path, line_number, "missing command");
        }
        if (strcmp(token, "power") == 0) {
            event->kind = SIM_SCENARIO_POWER;
            token = next_token(&cursor);
            if ((token == NULL) || (parse_on_off(token, &event->value) != 0)) {
                fclose(file);
                return scenario_error(path, line_number, "power expects on|off");
            }
        } else if (strcmp(token, "uart") == 0) {
            event->kind = SIM_SCENARIO_UART;
            token = next_token(&cursor);
            if (token == NULL) {
                fclose(file);
                return scenario_error(path, line_number, "uart expects a channel");
            }
            cycle = strtoul(token, &end, 10);
            if ((*token == '\0') || (*end != '\0') || (cycle >= SIM_UART_CHANNELS)) {
                fclose(file);
                return scenario_error(path, line_number, "uart channel must be 0 to 2");
            }
            event->channel = (uint8_t)cycle;
            for (;;) {
                token = next_token(&cursor);
                if (token == NULL) {
                    break;
                }
                if (parse_hex_token(token, event->data, &event->len) != 0) {
                    fclose(file);
                    return scenario_error(path, line_number,
                                          "uart expects an even number of hex digits");
                }
            }
            if (event->len == 0) {
                fclose(file);
                return scenario_error(path, line_number, "uart expects hex bytes");
            }
        } else if (strcmp(token, "relay_feedback") == 0) {
            event->kind = SIM_SCENARIO_RELAY_FEEDBACK;
            token = next_token(&cursor);
            if ((token == NULL) || (parse_on_off(token, &event->value) != 0)) {
                fclose(file);
                return scenario_error(path, line_number,
                                      "relay_feedback expects on|off");
            }
        } else if (strcmp(token, "cover") == 0) {
            event->kind = SIM_SCENARIO_COVER;
            token = next_token(&cursor);
            if (token == NULL) {
                fclose(file);
                return scenario_error(path, line_number, "cover expects open|closed");
            }
            if (strcmp(token, "open") == 0) {
                event->value = 0;
            } else if (strcmp(token, "closed") == 0) {
                event->value = 1;
            } else {
                fclose(file);
                return scenario_error(path, line_number, "cover expects open|closed");
            }
        } else if (strcmp(token, "key") == 0) {
            event->kind = SIM_SCENARIO_KEY;
            token = next_token(&cursor);
            if (token == NULL) {
                fclose(file);
                return scenario_error(path, line_number, "key expects press|release");
            }
            if (strcmp(token, "press") == 0) {
                event->value = 1;
            } else if (strcmp(token, "release") == 0) {
                event->value = 0;
            } else {
                fclose(file);
                return scenario_error(path, line_number, "key expects press|release");
            }
        } else if (strcmp(token, "chip") == 0) {
            event->kind = SIM_SCENARIO_CHIP;
            token = next_token(&cursor);
            if (token == NULL) {
                fclose(file);
                return scenario_error(path, line_number, "chip expects ok|fault");
            }
            if (strcmp(token, "ok") == 0) {
                event->value = 0;
            } else if (strcmp(token, "fault") == 0) {
                event->value = 1;
            } else {
                fclose(file);
                return scenario_error(path, line_number, "chip expects ok|fault");
            }
        } else if (strcmp(token, "adc") == 0) {
            event->kind = SIM_SCENARIO_ADC;
            token = next_token(&cursor);
            if (token == NULL) {
                fclose(file);
                return scenario_error(path, line_number, "adc expects temperature|battery");
            }
            if (strcmp(token, "temperature") == 0) {
                event->channel = ND_ADC_CHANNEL_TEMPERATURE;
            } else if (strcmp(token, "battery") == 0) {
                event->channel = ND_ADC_CHANNEL_BATTERY;
            } else {
                fclose(file);
                return scenario_error(path, line_number, "adc expects temperature|battery");
            }
            token = next_token(&cursor);
            long adc_value;
            if ((token == NULL) || (parse_long_token(token, -1000000, 1000000,
                                                     &adc_value) != 0)) {
                fclose(file);
                return scenario_error(path, line_number, "adc expects an integer");
            }
            event->meter[0] = (int32_t)adc_value;
        } else if (strcmp(token, "pulse") == 0) {
            long count;

            event->kind = SIM_SCENARIO_PULSE;
            token = next_token(&cursor);
            if ((token == NULL)
                || (parse_long_token(token, 1, SIM_PULSE_MAX, &count) != 0)) {
                fclose(file);
                return scenario_error(path, line_number, "pulse expects 1..64");
            }
            event->count = (uint8_t)count;
        } else if (strcmp(token, "energy") == 0) {
            event->kind = SIM_SCENARIO_ENERGY_CLEAR;
            token = next_token(&cursor);
            if ((token == NULL) || (strcmp(token, "clear") != 0)) {
                fclose(file);
                return scenario_error(path, line_number, "energy expects clear");
            }
        } else if (strcmp(token, "meter") == 0) {
            event->kind = SIM_SCENARIO_METER;
            for (;;) {
                char *field;
                long value;

                field = next_token(&cursor);
                if (field == NULL) {
                    break;
                }
                token = next_token(&cursor);
                if (token == NULL) {
                    fclose(file);
                    return scenario_error(path, line_number,
                                          "meter expects a value after every field");
                }
                if (strcmp(field, "voltage") == 0) {
                    if ((event->meter_mask & SIM_METER_VOLTAGE) != 0
                        || (parse_long_token(token, 0, 6553, &value) != 0)) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter voltage expects 0..6553 V");
                    }
                    event->meter[0] = (int32_t)(value * 10); /* 0.1 V */
                    event->meter_mask |= SIM_METER_VOLTAGE;
                } else if (strcmp(field, "current") == 0) {
                    if ((event->meter_mask & SIM_METER_CURRENT) != 0
                        || (parse_long_token(token, -100000, 100000, &value) != 0)) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter current expects -100000..100000 A");
                    }
                    event->meter[1] = (int32_t)(value * 1000); /* 0.001 A */
                    event->meter_mask |= SIM_METER_CURRENT;
                } else if (strcmp(field, "power") == 0) {
                    if ((event->meter_mask & SIM_METER_POWER) != 0
                        || (parse_long_token(token, -100000000, 100000000, &value) != 0)) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter power expects -100000000..100000000 W");
                    }
                    event->meter[2] = (int32_t)(value * 10); /* 0.1 W */
                    event->meter_mask |= SIM_METER_POWER;
                } else if (strcmp(field, "const") == 0) {
                    char *end;
                    unsigned long parsed = strtoul(token, &end, 10);

                    if ((event->meter_mask & SIM_METER_CONST) != 0
                        || (*token == '\0') || (*end != '\0') || (parsed == 0)
                        || (parsed > 0xFFFFFFFFUL)) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter const expects 1..4294967295");
                    }
                    event->meter_const = (uint32_t)parsed;
                    event->meter_mask |= SIM_METER_CONST;
                } else if (strcmp(field, "cal") == 0) {
                    if ((event->meter_mask & SIM_METER_CAL) != 0
                        || (parse_long_token(token, 0, 255, &value) != 0)) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter cal expects 0..255");
                    }
                    event->meter_cal = (uint8_t)value;
                    event->meter_mask |= SIM_METER_CAL;
                } else if (strcmp(field, "direction") == 0) {
                    if ((event->meter_mask & SIM_METER_DIRECTION) != 0) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter direction expects forward|reverse");
                    }
                    if (strcmp(token, "forward") == 0) {
                        event->meter_direction = 0;
                    } else if (strcmp(token, "reverse") == 0) {
                        event->meter_direction = 1;
                    } else {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter direction expects forward|reverse");
                    }
                    event->meter_mask |= SIM_METER_DIRECTION;
                } else if (strcmp(field, "tariff") == 0) {
                    if ((event->meter_mask & SIM_METER_TARIFF) != 0
                        || (parse_long_token(token, 0, 12, &value) != 0)) {
                        fclose(file);
                        return scenario_error(path, line_number,
                                              "meter tariff expects 0..12");
                    }
                    event->meter_tariff = (uint8_t)value;
                    event->meter_mask |= SIM_METER_TARIFF;
                } else {
                    fclose(file);
                    return scenario_error(path, line_number,
                                          "meter expects voltage|current|power|const|cal|"
                                          "direction|tariff");
                }
            }
            if (event->meter_mask == 0) {
                fclose(file);
                return scenario_error(path, line_number,
                                      "meter expects at least one field");
            }
        } else if (strcmp(token, "time") == 0) {
            int year, month, day, hour, minute, second;
            char extra;
            char *quote;

            event->kind = SIM_SCENARIO_TIME;
            cursor += strspn(cursor, " \t");
            quote = (*cursor == '\'') ? strchr(cursor + 1, '\'') : NULL;
            if (quote == NULL) {
                fclose(file);
                return scenario_error(path, line_number,
                                      "time expects 'YYYY-MM-DD HH:MM:SS'");
            }
            *quote = '\0';
            if ((sscanf(cursor + 1, "%d-%d-%d %d:%d:%d%c", &year, &month, &day,
                        &hour, &minute, &second, &extra) != 6)
                || (year < 1970) || (month < 1) || (month > 12) || (day < 1)
                || (day > 31) || (hour < 0) || (hour > 23) || (minute < 0)
                || (minute > 59) || (second < 0) || (second > 59)) {
                fclose(file);
                return scenario_error(path, line_number,
                                      "time expects 'YYYY-MM-DD HH:MM:SS'");
            }
            memset(&event->when, 0, sizeof(event->when));
            event->when.tm_year = year - 1900;
            event->when.tm_mon = month - 1;
            event->when.tm_mday = day;
            event->when.tm_hour = hour;
            event->when.tm_min = minute;
            event->when.tm_sec = second;
            cursor = quote + 1;
        } else {
            fclose(file);
            return scenario_error(path, line_number, "unknown command");
        }

        token = next_token(&cursor);
        if (token != NULL) {
            fclose(file);
            return scenario_error(path, line_number, "unexpected trailing text");
        }
        scenario_event_count++;
    }

    fclose(file);
    return 0;
}

void sim_scenario_apply(struct instance_t *instance, unsigned cycle)
{
    scenario_instance = instance;
    scenario_cycle = cycle;

    for (size_t i = 0; i < scenario_event_count; i++) {
        const sim_scenario_event_t *event = &scenario_events[i];

        if (event->cycle != cycle) {
            continue;
        }
        switch (event->kind) {
        case SIM_SCENARIO_POWER:
            scenario_power_on = event->value;
            if (g_sim.verbose) {
                sim_trace_printf("[sim] power %s", event->value ? "on" : "off");
            }
            break;
        case SIM_SCENARIO_UART: {
            struct fifo_t *rx = uart_rx_fifo[event->channel];
            uint16_t written = 0;

            if (rx != NULL) {
                written = fifoBuf_putData(rx, event->data, event->len);
            }
            if (instance->pipe_uart[event->channel].buf_ptr != NULL) {
                fifoBuf_putByte(&instance->pipe_uart[event->channel],
                                IRQ_UART_RX_TM_OUT);
            }
            if (g_sim.verbose) {
                sim_trace_printf("[sim] uart rx ch=%u len=%u written=%u",
                                 event->channel, event->len, written);
            }
            break;
        }
        case SIM_SCENARIO_RELAY_FEEDBACK:
            scenario_relay_off = (uint8_t)!event->value;
            if (g_sim.verbose) {
                sim_trace_printf("[sim] relay_feedback %s",
                                 event->value ? "on" : "off");
            }
            break;
        case SIM_SCENARIO_COVER: {
            struct mbase_control_block_t *mbase =
                (struct mbase_control_block_t *)get_pipeline_ctx(instance,
                                                                 pid_meter_base);

            scenario_cover_closed = event->value;
            if (mbase != NULL) {
                nd_mbase_CoverOpenS_status(mbase) =
                    event->value ? ND_EVENT_RESET : ND_EVENT_SET;
            }
            if (g_sim.verbose) {
                sim_trace_printf("[sim] cover %s", event->value ? "closed" : "open");
            }
            break;
        }
        case SIM_SCENARIO_KEY:
            scenario_key_pressed = event->value;
            if (event->value && (instance->pipe_key.buf_ptr != NULL)) {
                fifoBuf_putByte(&instance->pipe_key, IRQ_PMU);
            }
            if (g_sim.verbose) {
                sim_trace_printf("[sim] key %s", event->value ? "press" : "release");
            }
            break;
        case SIM_SCENARIO_PULSE:
            for (uint8_t pulse = 0; pulse < event->count; pulse++) {
                Energy_Pluse_Add();
            }
            if (g_sim.verbose) {
                sim_trace_printf("[sim] pulse count=%u", event->count);
            }
            break;
        case SIM_SCENARIO_TIME:
            instance->time = event->when;
            if (g_sim.verbose) {
                sim_trace_printf("[sim] time %04d-%02d-%02d %02d:%02d:%02d",
                                 event->when.tm_year + 1900,
                                 event->when.tm_mon + 1, event->when.tm_mday,
                                 event->when.tm_hour, event->when.tm_min,
                                 event->when.tm_sec);
            }
            break;
        case SIM_SCENARIO_ENERGY_CLEAR:
            Energy_clr(instance);
            if (g_sim.verbose) {
                sim_trace_printf("[sim] energy clear");
            }
            break;
        case SIM_SCENARIO_CHIP:
            sim_meter_set_chip_fault(event->value);
            if (g_sim.verbose) {
                sim_trace_printf("[sim] chip %s", event->value ? "fault" : "ok");
            }
            break;
        case SIM_SCENARIO_ADC:
            sim_meter_set_adc(event->channel, event->meter[0]);
            if (g_sim.verbose) {
                sim_trace_printf("[sim] adc channel=%u value=%ld",
                                 event->channel, (long)event->meter[0]);
            }
            break;
        case SIM_SCENARIO_METER:
            /* Applied at the Energy_Pluse_Count stage, after
             * Electrical_measurement has written its own values. */
            break;
        default:
            break;
        }
    }
}

void sim_scenario_apply_stage(const char *name)
{
    struct mbase_control_block_t *mbase;

    if ((scenario_instance == NULL) || (strcmp(name, "Energy_Pluse_Count") != 0)) {
        return;
    }
    mbase = (struct mbase_control_block_t *)get_pipeline_ctx(scenario_instance,
                                                             pid_meter_base);
    if (mbase == NULL) {
        return;
    }
    for (size_t i = 0; i < scenario_event_count; i++) {
        const sim_scenario_event_t *event = &scenario_events[i];

        if ((event->kind != SIM_SCENARIO_METER)
            || (event->cycle != scenario_cycle)) {
            continue;
        }
        if ((event->meter_mask & SIM_METER_VOLTAGE) != 0) {
            nd_mbase_voltage_ac(mbase) = (uint32_t)event->meter[0];
        }
        if ((event->meter_mask & SIM_METER_CURRENT) != 0) {
            nd_mbase_current_l(mbase) = event->meter[1];
        }
        if ((event->meter_mask & SIM_METER_POWER) != 0) {
            nd_mbase_active_power(mbase) = event->meter[2];
        }
        if ((event->meter_mask & SIM_METER_CONST) != 0) {
            nd_mbase_meter_const(mbase) = event->meter_const;
        }
        if ((event->meter_mask & SIM_METER_CAL) != 0) {
            nd_mbase_active_energy_cal_word(mbase) = event->meter_cal;
        }
        if ((event->meter_mask & SIM_METER_DIRECTION) != 0) {
            nd_mbase_power_dir(mbase) = event->meter_direction;
        }
        if ((event->meter_mask & SIM_METER_TARIFF) != 0) {
            nd_mbase_TOUNo(mbase) = event->meter_tariff;
        }
        if (g_sim.verbose) {
            sim_trace_printf("[sim] meter cycle=%u mask=0x%02x v=%ld i=%ld p=%ld "
                             "const=%lu cal=%u dir=%u tariff=%u",
                             scenario_cycle, event->meter_mask,
                             (long)event->meter[0], (long)event->meter[1],
                             (long)event->meter[2],
                             (unsigned long)event->meter_const,
                             (unsigned)event->meter_cal,
                             (unsigned)event->meter_direction,
                             (unsigned)event->meter_tariff);
        }
    }
}

int sim_scenario_power_on(void) { return scenario_power_on; }
int sim_scenario_relay_is_off(void) { return scenario_relay_off; }
int sim_scenario_cover_closed(void) { return scenario_cover_closed; }
int sim_scenario_key_pressed(void) { return scenario_key_pressed; }
int sim_scenario_key_released(void) { return !scenario_key_pressed; }

static void capture_stamp(char *buffer, size_t size)
{
    struct tm when = {0};

    if (scenario_instance != NULL) {
        when = scenario_instance->time;
    }
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             when.tm_year + 1900, when.tm_mon + 1, when.tm_mday, when.tm_hour,
             when.tm_min, when.tm_sec);
}

void sim_log_relay(const char *action)
{
    char stamp[20];

    if (relay_log_file == NULL) {
        return;
    }
    capture_stamp(stamp, sizeof(stamp));
    fprintf(relay_log_file, "cycle %u time %s action %s\n", scenario_cycle, stamp,
            action);
}

void sim_log_backlight(int on)
{
    char stamp[20];

    if (backlight_log_file == NULL) {
        return;
    }
    capture_stamp(stamp, sizeof(stamp));
    fprintf(backlight_log_file, "cycle %u time %s value %d\n", scenario_cycle,
            stamp, on ? 1 : 0);
}

void sim_log_lcd(const uint8_t *segments, unsigned len)
{
    char stamp[20];

    if (lcd_log_file == NULL) {
        return;
    }
    capture_stamp(stamp, sizeof(stamp));
    fprintf(lcd_log_file, "cycle %u time %s fb ", scenario_cycle, stamp);
    for (unsigned i = 0; i < len; i++) {
        fprintf(lcd_log_file, "%02x", segments[i]);
    }
    fputc('\n', lcd_log_file);
}

int sim_script_open(void)
{
    if (g_sim.uart_tx_path != NULL) {
        uart_tx_file = fopen(g_sim.uart_tx_path, "wb");
        if (uart_tx_file == NULL) {
            perror(g_sim.uart_tx_path);
            return -1;
        }
    }
    if (g_sim.uart_tx_hex_path != NULL) {
        uart_tx_hex_file = fopen(g_sim.uart_tx_hex_path, "w");
        if (uart_tx_hex_file == NULL) {
            perror(g_sim.uart_tx_hex_path);
            sim_script_close();
            return -1;
        }
    }
    if (g_sim.relay_log_path != NULL) {
        relay_log_file = fopen(g_sim.relay_log_path, "w");
        if (relay_log_file == NULL) {
            perror(g_sim.relay_log_path);
            sim_script_close();
            return -1;
        }
    }
    if (g_sim.backlight_log_path != NULL) {
        backlight_log_file = fopen(g_sim.backlight_log_path, "w");
        if (backlight_log_file == NULL) {
            perror(g_sim.backlight_log_path);
            sim_script_close();
            return -1;
        }
    }
    if (g_sim.lcd_log_path != NULL) {
        lcd_log_file = fopen(g_sim.lcd_log_path, "w");
        if (lcd_log_file == NULL) {
            perror(g_sim.lcd_log_path);
            sim_script_close();
            return -1;
        }
    }
    return 0;
}

void sim_script_close(void)
{
    if (uart_tx_file != NULL) {
        fclose(uart_tx_file);
        uart_tx_file = NULL;
    }
    if (uart_tx_hex_file != NULL) {
        fclose(uart_tx_hex_file);
        uart_tx_hex_file = NULL;
    }
    if (relay_log_file != NULL) {
        fclose(relay_log_file);
        relay_log_file = NULL;
    }
    if (backlight_log_file != NULL) {
        fclose(backlight_log_file);
        backlight_log_file = NULL;
    }
    if (lcd_log_file != NULL) {
        fclose(lcd_log_file);
        lcd_log_file = NULL;
    }
}

void sim_uart_bind(uint8_t channel, void *tx_fifo, void *rx_fifo)
{
    if (channel < SIM_UART_CHANNELS) {
        uart_tx_fifo[channel] = tx_fifo;
        uart_rx_fifo[channel] = rx_fifo;
    }
}

static void uart_emit(uint8_t byte)
{
    if (uart_tx_file != NULL) {
        fputc(byte, uart_tx_file);
    }
    if (uart_tx_hex_file != NULL) {
        fprintf(uart_tx_hex_file, "%02x\n", byte);
    }
}

void sim_uart_tx_byte(uint8_t byte, unsigned index)
{
    if (index == ND_UART_RS4851) {
        uart_emit(byte);
    }
}

void sim_uart_drain(void)
{
    for (uint8_t channel = 0; channel < SIM_UART_CHANNELS; channel++) {
        struct fifo_t *tx = uart_tx_fifo[channel];

        if (tx == NULL) {
            continue;
        }
        while (fifoBuf_getUsed(tx) > 0) {
            uint8_t byte = fifoBuf_getByte(tx);

            if (channel == UART_RS4851_CHL) {
                uart_emit(byte);
            }
        }
    }
    if (uart_tx_file != NULL) {
        fflush(uart_tx_file);
    }
    if (uart_tx_hex_file != NULL) {
        fflush(uart_tx_hex_file);
    }
    if (g_sim.verbose) {
        for (uint8_t channel = 0; channel < SIM_UART_CHANNELS; channel++) {
            if (uart_rx_fifo[channel] != NULL) {
                uint16_t left = fifoBuf_getUsed(uart_rx_fifo[channel]);

                if (left > 0) {
                    sim_trace_printf("[sim] uart rx ch=%u pending=%u", channel,
                                     left);
                }
            }
        }
    }
}
