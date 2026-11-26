#include <stdint.h>
#include <string.h>

#include "baremetal.h"
#include "board_ports.h"
#include "components/protocol/dlms/include/nd_cosem_port.h"
#include "components/services/maintenance/nd_maintenance.h"
#include "targets/idis_ip_meter/cosem/nd_idis_registry.h"
#include "cpu_surport.h"
#include "domain/common/calendar.h"
#include "components/services/push/push.h"
#include "targets/idis_ip_meter/board/board_io.h"
#include "targets/idis_ip_meter/board/meter_config.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_calendar.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_clock.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_data.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_demand.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_disconnect.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_extended_register.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_image_transfer.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_limiter.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_push.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_register_monitor.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_script_table.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_setup.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_single_action.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_average.h"
#include "targets/idis_ip_meter/cosem/nd_idis_calibration.h"
#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event_map.h"
#include "targets/idis_ip_meter/cosem/nd_idis_mbus.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/cosem/nd_idis_pq.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profile_table.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profiles.h"
#include "targets/idis_ip_meter/cosem/nd_idis_scheduler.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"
#include "infra/drivers/storage/block/block_ops.h"
#include "infra/storage/pulse_store.h"
#include "infra/soc/v85xxp/v8500/time_link.h"
#include "infra/soc/v85xxp/v8500/time_link.h"
#include "targets/idis_ip_meter/board/board_eeprom.h"
#include "targets/idis_ip_meter/board/board_flash.h"


enum {
    EEPROM_EVENT = 0u,
    EEPROM_DEMAND = 1024u,
    EEPROM_DISCONNECT = 2048u,
    EEPROM_MAX_DEMAND = 3072u,
    EEPROM_ENERGY = 4096u,
    EEPROM_CLOCK = 5120u,
    EEPROM_PQ = 6144u,
    EEPROM_CALENDAR = 7168u,
    EEPROM_CALENDAR_EXTRA = 9216u,
    EEPROM_RELAYS = 15360u,
    EEPROM_MBUS = 16384u,
    EEPROM_PUSH = 18432u,
    EEPROM_SCRIPT = 22528u,
    EEPROM_SINGLE_ACTION = 30720u,
    EEPROM_POWER = 32768u,
    EEPROM_MONITOR = 33792u,
    EEPROM_LIMITER = 34816u,
    EEPROM_IMAGE = 35840u,
    EEPROM_SETUP = 36864u,
    EEPROM_REGISTRY = ND_IDIS_REGISTRY_EEPROM_OFFSET,
    EEPROM_PERIOD = 300u,
    EEPROM_ENERGY_BASE = ND_IDIS_ENERGY_BASE_OFFSET,
    EEPROM_ENERGY_BASE_ALT = ND_IDIS_ENERGY_BASE_ALT_OFFSET,
    EEPROM_ENERGY_BASE_BYTES = ND_IDIS_ENERGY_BASE_BYTES,
    /* Free between the 640 KiB profile region and the 1 MiB image region. */
    FLASH_ENERGY_JOURNAL = 0xA0000u,
    FLASH_ENERGY_JOURNAL_BYTES = 16384u,
    FLASH_IMAGE = 1048576u
};

typedef struct {
    nd_cosem_port_t port;
    nd_clock_port_t clock_port;
    nd_measurement_port_t measurement;
    board_measurement_calibration_t measurement_calibration;
    nd_maintenance_t maintenance;
    nd_pulse_store_storage_t energy_pulse_storage;
    nd_eeprom_block_t energy_eeprom_block;
    nd_flash_block_t energy_flash_block;
    /* cm0_systick_init() programs a 10 ms SysTick, and ndv_timer_tick() uses
     * that same unit. Keep the second counter separate from the timer ticks. */
    uint64_t energy_pulse_fraction;
    int energy_commit_pending;
    int energy_appends_pending;
} nd_idis_composition_t;

/* Board-owned hardware buffers stay in board_ports.c. */
typedef struct {
    nd_idis_objects_t objects;
    nd_idis_composition_t composition;
    uint32_t last_save;
    int power_failed;
    int cover_open;
    int key_down;
    int backlight;
    int relay_connected;
    int relay_feedback_pending;
    unsigned relay_feedback_seconds;
    int battery_low;
    nd_idis_board_state_t board;
    board_measurement_ctx_t measurement_ctx;
} nd_idis_app_context_t;

static nd_idis_app_context_t g_idis_app;

/* The local optical port is the physical gate; the release entry additionally
 * requires management SAP 1 and a local (non-remote) association. */
static int maintenance_release(void *ctx, uint32_t confirmation)
{
    return nd_cosem_port_release_legacy(ctx, ND_IDIS_SAP_MANAGEMENT,
                                        confirmation) == ND_OK
               ? 0
               : -1;
}

static int clock_read(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    if (board_rtc_read(now) == 0) {
        return 0;
    }
    uint32_t seconds = baremetal_uptime_seconds();
    nd_datetime_t fallback;
    nd_calendar_from_elapsed(nd_calendar_total_day(1, 1, 2026), seconds,
                             &fallback);
    *now = (nd_dlms_datetime_t){
        .year = fallback.year,
        .month = fallback.month,
        .day = fallback.day,
        .hour = fallback.hour,
        .minute = fallback.minute,
        .second = fallback.second,
        .status = 0x01, /* DLMS clock status: invalid value */
    };
    return 0;
}

static int clock_write(void *ctx, const nd_dlms_datetime_t *now)
{
    (void)ctx;
    nd_datetime_t datetime = {
        .second = now->second,
        .minute = now->minute,
        .hour = now->hour,
        .day = now->day,
        .month = now->month,
        .year = now->year,
    };
    uint8_t buffer[7];
    nd_calendar_to_rtc_bcd(&datetime, buffer);
    buffer[6] = nd_calendar_weekday(datetime.day, datetime.month,
                                    datetime.year);
    return write_mcu_rtc(0, buffer, 7) ? 0 : -1;
}

static void rtc_init(void)
{
    nd_dlms_datetime_t current;
    if (board_rtc_read(&current) == 0) {
        return;
    }
    nd_dlms_datetime_t default_time = {
        .year = 2026,
        .month = 1,
        .day = 1,
        .hour = 0,
        .minute = 0,
        .second = 0,
    };
    (void)clock_write(NULL, &default_time);
}

static nd_datetime_t datetime_from_dlms(const nd_dlms_datetime_t *now)
{
    return (nd_datetime_t){
        .second = now->second,
        .minute = now->minute,
        .hour = now->hour,
        .day = now->day,
        .month = now->month,
        .year = now->year,
    };
}

static void profile_watchdog_feed(void *ctx)
{
    (void)ctx;
    mcu_clear_watchdog();
}

_Static_assert(BOARD_FLASH_SIZE % BOARD_FLASH_SECTOR == 0,
               "the energy pulse block erases whole flash sectors");

static int energy_pulse_store_init(void)
{
    nd_eeprom_block_init(&g_idis_app.composition.energy_eeprom_block, board_eeprom(&g_idis_app.board),
                         BOARD_EEPROM_SIZE);
    nd_flash_block_init(&g_idis_app.composition.energy_flash_block, board_flash(&g_idis_app.board),
                        BOARD_FLASH_SIZE);
    nd_pulse_store_config_t config = {
        .block = nd_flash_block_port(&g_idis_app.composition.energy_flash_block),
        .base_block = nd_eeprom_block_port(&g_idis_app.composition.energy_eeprom_block),
        .journal_offset = FLASH_ENERGY_JOURNAL,
        .journal_bytes = FLASH_ENERGY_JOURNAL_BYTES,
        .base_offset = {EEPROM_ENERGY_BASE, EEPROM_ENERGY_BASE_ALT},
        .base_capacity = EEPROM_ENERGY_BASE_BYTES,
        /* The MX25L3206E byte-program endurance is not confirmed; 8 bounds a
         * power cut at 7 pulses and cuts byte programs by 8. */
        .granularity = 8u,
    };

    return nd_pulse_store_init(&g_idis_app.composition.energy_pulse_storage, &config);
}

static int load_persisted(void)
{
    mcu_clear_watchdog();
    nd_store_t *eeprom = board_store_port(&g_idis_app.board);
    (void)nd_idis_clock_load(&g_idis_app.objects.clock, eeprom, EEPROM_CLOCK);
    /* A present journal that cannot load the book would keep running on the
     * zero book and silently drop the billing register; fail the boot. */
    if (nd_idis_energy_load(
            &g_idis_app.objects.energy,
            nd_pulse_store_port(&g_idis_app.composition.energy_pulse_storage), eeprom,
            EEPROM_ENERGY) != 0) {
        return -1;
    }
    (void)nd_ic_demand_load(g_idis_app.objects.demand, eeprom, EEPROM_DEMAND);
    (void)nd_ic_extended_load(g_idis_app.objects.max_demand, eeprom, EEPROM_MAX_DEMAND);
    (void)nd_ic_disconnect_load(&g_idis_app.objects.disconnect, eeprom,
                                EEPROM_DISCONNECT);
    nd_disconnect_snapshot_t relays[6] = {
        g_idis_app.objects.load_control_1, g_idis_app.objects.load_control_2,
        g_idis_app.objects.mbus_disconnect[0], g_idis_app.objects.mbus_disconnect[1],
        g_idis_app.objects.mbus_disconnect[2], g_idis_app.objects.mbus_disconnect[3],
    };
    (void)nd_ic_relays_load(relays, eeprom, EEPROM_RELAYS);
    g_idis_app.objects.load_control_1 = relays[0];
    g_idis_app.objects.load_control_2 = relays[1];
    for (uint8_t relay = 0; relay < 4; relay++) {
        g_idis_app.objects.mbus_disconnect[relay] = relays[2 + relay];
    }
    (void)nd_idis_pq_load(&g_idis_app.objects.pq, eeprom, EEPROM_PQ);
    (void)nd_idis_power_load(&g_idis_app.objects.power_failure, eeprom, EEPROM_POWER);
    (void)nd_idis_calendar_load(&g_idis_app.objects.calendar, eeprom, EEPROM_CALENDAR);
    for (uint8_t index = 0; index < 3; index++) {
        (void)nd_idis_calendar_load(&g_idis_app.objects.calendar_extra[index], eeprom,
                                    EEPROM_CALENDAR_EXTRA + index * 2048u);
    }
    (void)nd_idis_mbus_load(&g_idis_app.objects.mbus, eeprom, EEPROM_MBUS);
    (void)nd_idis_push_load(&g_idis_app.objects.push, eeprom, EEPROM_PUSH);
    (void)nd_idis_script_load(&g_idis_app.objects.scripts, eeprom, EEPROM_SCRIPT);
    (void)nd_idis_single_action_load(&g_idis_app.objects.single_action, eeprom,
                                     EEPROM_SINGLE_ACTION);
    (void)nd_idis_monitor_load(&g_idis_app.objects.monitors, eeprom, EEPROM_MONITOR);
    (void)nd_idis_limiter_load(&g_idis_app.objects.limiter, eeprom, EEPROM_LIMITER);
    (void)nd_idis_image_load(&g_idis_app.objects.image, eeprom, EEPROM_IMAGE);
    (void)nd_idis_setup_load(&g_idis_app.objects.setup, eeprom, EEPROM_SETUP);
    if (nd_idis_registry_load(&g_idis_app.objects.association.association.base, eeprom,
                              EEPROM_REGISTRY) != 0) {
        nd_idis_association_init(&g_idis_app.objects.association);
    }
    (void)nd_idis_event_load(&g_idis_app.objects.event, eeprom, EEPROM_EVENT);
    mcu_clear_watchdog();
    return 0;
}

static void save_persisted(void)
{
    mcu_clear_watchdog();
    nd_store_t *eeprom = board_store_port(&g_idis_app.board);
    (void)nd_idis_clock_save(&g_idis_app.objects.clock, eeprom, EEPROM_CLOCK);
    (void)nd_idis_energy_save(&g_idis_app.objects.energy,
                              nd_pulse_store_port(&g_idis_app.composition.energy_pulse_storage),
                              eeprom, EEPROM_ENERGY);
    (void)nd_ic_demand_save(g_idis_app.objects.demand, eeprom, EEPROM_DEMAND);
    (void)nd_ic_extended_save(g_idis_app.objects.max_demand, eeprom, EEPROM_MAX_DEMAND);
    (void)nd_ic_disconnect_save(&g_idis_app.objects.disconnect, eeprom,
                                EEPROM_DISCONNECT);
    nd_disconnect_snapshot_t relays[6] = {
        g_idis_app.objects.load_control_1, g_idis_app.objects.load_control_2,
        g_idis_app.objects.mbus_disconnect[0], g_idis_app.objects.mbus_disconnect[1],
        g_idis_app.objects.mbus_disconnect[2], g_idis_app.objects.mbus_disconnect[3],
    };
    (void)nd_ic_relays_save(relays, eeprom, EEPROM_RELAYS);
    (void)nd_idis_pq_save(&g_idis_app.objects.pq, eeprom, EEPROM_PQ);
    (void)nd_idis_power_save(&g_idis_app.objects.power_failure, eeprom, EEPROM_POWER);
    (void)nd_idis_calendar_save(&g_idis_app.objects.calendar, eeprom, EEPROM_CALENDAR);
    for (uint8_t index = 0; index < 3; index++) {
        (void)nd_idis_calendar_save(&g_idis_app.objects.calendar_extra[index], eeprom,
                                    EEPROM_CALENDAR_EXTRA + index * 2048u);
    }
    (void)nd_idis_mbus_save(&g_idis_app.objects.mbus, eeprom, EEPROM_MBUS);
    (void)nd_idis_push_save(&g_idis_app.objects.push, eeprom, EEPROM_PUSH);
    mcu_clear_watchdog();
    (void)nd_idis_single_action_save(&g_idis_app.objects.single_action, eeprom,
                                     EEPROM_SINGLE_ACTION);
    (void)nd_idis_monitor_save(&g_idis_app.objects.monitors, eeprom, EEPROM_MONITOR);
    (void)nd_idis_limiter_save(&g_idis_app.objects.limiter, eeprom, EEPROM_LIMITER);
    (void)nd_idis_image_save(&g_idis_app.objects.image, eeprom, EEPROM_IMAGE);
    (void)nd_idis_setup_save(&g_idis_app.objects.setup, eeprom, EEPROM_SETUP);
    mcu_clear_watchdog();
    (void)nd_idis_event_save(&g_idis_app.objects.event, eeprom, EEPROM_EVENT);
    mcu_clear_watchdog();
}

static int save_energy_book(void *ctx)
{
    (void)ctx;
    mcu_clear_watchdog();
    /* A reset is not a pulse: fold it into a new base and reset the journal
     * before reporting success, or replay would resurrect the cleared bills. */
    int result = nd_idis_energy_commit(
        &g_idis_app.objects.energy,
        nd_pulse_store_port(&g_idis_app.composition.energy_pulse_storage),
        board_store_port(&g_idis_app.board), EEPROM_ENERGY);
    if (result == 0) {
        g_idis_app.composition.energy_appends_pending = 0;
    }
    mcu_clear_watchdog();
    return result;
}

static void set_product_identity(void)
{
    static const uint8_t ldn[] = "HXE1040000000001";
    static const uint8_t device_id[] = "00000001";
    static const uint8_t server_title[8] = {
        'H', 'X', 'E', 0x68, 0x00, 0x00, 0x00, 0x01,
    };
    g_idis_app.objects.ldn_len = sizeof(ldn) - 1;
    memcpy(g_idis_app.objects.ldn, ldn, g_idis_app.objects.ldn_len);
    g_idis_app.objects.device_id_len[0] = sizeof(device_id) - 1;
    memcpy(g_idis_app.objects.device_id[0], device_id, g_idis_app.objects.device_id_len[0]);
    memcpy(g_idis_app.objects.association.security.server_title, server_title,
           sizeof(server_title));
    memcpy(g_idis_app.objects.association.consumer.server_title, server_title,
           sizeof(server_title));
}

static int init_objects(nd_idis_calibration_status_t calibration)
{
    memset(&g_idis_app.objects, 0, sizeof(g_idis_app.objects));

    g_idis_app.composition.clock_port = (nd_clock_port_t){
        .ctx = NULL,
        .read = clock_read,
        .write = clock_write,
    };
    g_idis_app.measurement_ctx = (board_measurement_ctx_t){
        .calibration = &g_idis_app.composition.measurement_calibration,
        .state = &g_idis_app.board,
    };
    g_idis_app.composition.measurement = (nd_measurement_port_t){
        .ctx = &g_idis_app.measurement_ctx,
        .read = board_measurement_read,
    };

    nd_idis_clock_init(&g_idis_app.objects.clock, &g_idis_app.composition.clock_port);
    nd_energy_set_init(&g_idis_app.objects.energy, 2000);
    nd_idis_average_init(&g_idis_app.objects.average, &g_idis_app.composition.measurement);
    nd_pq_config_t pq_config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 50,
        .debounce_seconds = 1,
        .phase_count = 3,
    };
    nd_idis_pq_init(&g_idis_app.objects.pq, &pq_config);
    nd_power_failure_init(&g_idis_app.objects.power_failure, 60);

    for (uint8_t group = 0; group < ND_IDIS_DEMAND_MAX; group++) {
        g_idis_app.objects.demand[group].current.period_seconds = 900;
        g_idis_app.objects.demand[group].current.periods = 15;
    }
    for (uint8_t group = 0; group < ND_IDIS_DEMAND_MAX; group++) {
        g_idis_app.objects.demand_acc[group] = (nd_idis_demand_acc_t){0};
    }

    nd_disconnect_init(&g_idis_app.objects.disconnect, 2, NULL);
    nd_disconnect_init(&g_idis_app.objects.load_control_1, 2, NULL);
    nd_disconnect_init(&g_idis_app.objects.load_control_2, 2, NULL);
    for (uint8_t relay = 0; relay < 4; relay++) {
        nd_disconnect_init(&g_idis_app.objects.mbus_disconnect[relay], 2, NULL);
    }

    nd_ic_register_activation_init(&g_idis_app.objects.activation);
    nd_ic_function_control_init(&g_idis_app.objects.functions);
    nd_idis_push_init(&g_idis_app.objects.push);
    nd_idis_mbus_init(&g_idis_app.objects.mbus);
    nd_idis_calendar_init(&g_idis_app.objects.calendar);
    nd_idis_calendar_apply_defaults(&g_idis_app.objects.calendar, &ND_OBIS_SCRIPT_TARIFF, 1);
    for (uint8_t index = 0; index < 3; index++) {
        nd_idis_calendar_init(&g_idis_app.objects.calendar_extra[index]);
    }

    nd_idis_script_init(&g_idis_app.objects.scripts);
    nd_idis_single_action_init(&g_idis_app.objects.single_action);
    nd_idis_monitors_init(&g_idis_app.objects.monitors);
    nd_idis_limiter_init(&g_idis_app.objects.limiter);
    nd_idis_image_init(&g_idis_app.objects.image);
    nd_idis_setup_init(&g_idis_app.objects.setup);
    nd_idis_association_init(&g_idis_app.objects.association);
    set_product_identity();
    g_idis_app.objects.access_enforced = 1;
    nd_idis_scheduler_init(&g_idis_app.objects.scheduler);

    nd_idis_event_init(&g_idis_app.objects.event, nd_idis_event_rules,
                       (uint16_t)nd_idis_event_rule_count);

    nd_profile_watchdog_t profile_watchdog = {
        .ctx = NULL,
        .feed = profile_watchdog_feed,
    };
    /* Fail closed on a pre-v4 image: keep it, hold appends, and let the bench
     * reset/migration discard it. See docs/design/record-retrieval.md.
     * A device error instead stops the boot: running with zero profiles
     * leaves every class-7 object undefined, and the watchdog restarts the
     * scan after main returns. */
    if (nd_idis_profile_table_boot(&g_idis_app.objects.profiles, board_flash_port(&g_idis_app.board),
                                   &g_idis_app.composition.clock_port,
                                   &profile_watchdog) != 0) {
        return -1;
    }
    mcu_clear_watchdog();

    g_idis_app.composition.port = (nd_cosem_port_t){
        .ctx = &g_idis_app.objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_maintenance_init(&g_idis_app.composition.maintenance);
    nd_idis_profiles_bind_port(&g_idis_app.objects.profiles, &g_idis_app.composition.port);
    nd_idis_script_bind_port(&g_idis_app.objects.scripts, &g_idis_app.composition.port);
    nd_idis_script_bind_store(&g_idis_app.objects.scripts, board_store_port(&g_idis_app.board),
                              EEPROM_SCRIPT);
    nd_idis_image_bind_store(&g_idis_app.objects.image, board_flash_port(&g_idis_app.board), FLASH_IMAGE);

    g_idis_app.objects.reg.clock = &g_idis_app.objects.clock;
    g_idis_app.objects.reg.energy = &g_idis_app.objects.energy;
    g_idis_app.objects.reg.save_energy = save_energy_book;
    g_idis_app.objects.reg.measurement = &g_idis_app.composition.measurement;
#ifndef IDIS_PHASES
#define IDIS_PHASES 1
#endif
    g_idis_app.objects.reg.phases = IDIS_PHASES;
    g_idis_app.objects.reg.average = &g_idis_app.objects.average;
    g_idis_app.objects.reg.pq = &g_idis_app.objects.pq;
    g_idis_app.objects.reg.power = &g_idis_app.objects.power_failure;

    nd_idis_event_bind_log(
        &g_idis_app.objects.event,
        nd_idis_profiles_find(&g_idis_app.objects.profiles, &ND_OBIS_STANDARD_EVENT_LOG),
        &g_idis_app.composition.port, &g_idis_app.composition.clock_port, 16);
    nd_idis_event_bind_category_log(
        &g_idis_app.objects.event, ND_IDIS_EVENT_LOG_FRAUD,
        nd_idis_profiles_find(&g_idis_app.objects.profiles, &ND_OBIS_FRAUD_LOG));
    nd_idis_event_bind_category_log(
        &g_idis_app.objects.event, ND_IDIS_EVENT_LOG_DISCONNECTOR,
        nd_idis_profiles_find(&g_idis_app.objects.profiles,
                              &ND_OBIS_DISCONNECTOR_LOG));
    nd_idis_event_bind_category_log(
        &g_idis_app.objects.event, ND_IDIS_EVENT_LOG_MBUS,
        nd_idis_profiles_find(&g_idis_app.objects.profiles, &ND_OBIS_MBUS_EVENT_LOG));
    nd_idis_event_bind_category_log(
        &g_idis_app.objects.event, ND_IDIS_EVENT_LOG_PQ,
        nd_idis_profiles_find(&g_idis_app.objects.profiles, &ND_OBIS_PQ_LOG));
    nd_idis_event_bind_category_log(
        &g_idis_app.objects.event, ND_IDIS_EVENT_LOG_COMMUNICATION,
        nd_idis_profiles_find(&g_idis_app.objects.profiles, &ND_OBIS_COMM_LOG));
    nd_idis_event_bind_category_log(
        &g_idis_app.objects.event, ND_IDIS_EVENT_LOG_POWER_FAILURE,
        nd_idis_profiles_find(&g_idis_app.objects.profiles, &ND_OBIS_POWER_FAILURE_LOG));
    nd_idis_event_bind_profiles(&g_idis_app.objects.event, &g_idis_app.objects.profiles);

    if (energy_pulse_store_init() != 0 || load_persisted() != 0) {
        return -1;
    }
    nd_idis_single_action_apply_defaults(&g_idis_app.objects.single_action);
    nd_dlms_datetime_t boot_clock;
    if (clock_read(NULL, &boot_clock) != 0
        || (boot_clock.status & 0x01) != 0) {
        /* IDIS default tariff: script 1 when the clock is invalid. */
        (void)nd_idis_script_execute(&g_idis_app.objects.scripts, &g_idis_app.composition.port, 1,
                                     &ND_OBIS_SCRIPT_TARIFF, 1);
    }
    /* Event 12: a retained pre-v4 image or an interrupted release holds every
     * profile store, so the operator can see why no profile grows. */
    if (nd_idis_profiles_legacy_held(&g_idis_app.objects.profiles)) {
        (void)nd_idis_event_raise(&g_idis_app.objects.event, 12);
    }
    uint16_t calibration_event =
        nd_idis_calibration_fault_event(calibration);
    if (calibration_event != 0) {
        (void)nd_idis_event_raise(&g_idis_app.objects.event, calibration_event);
    }
    return 0;
}

static void poll_gpio(void)
{
    int open = !nd_board_cover_closed();
    if (open && !g_idis_app.cover_open) {
        (void)nd_idis_event_raise(&g_idis_app.objects.event, 40);
    }
    g_idis_app.cover_open = open;

    int pressed = nd_board_key_pressed();
    if ((pressed && !g_idis_app.key_down) || (board_key_event_take(&g_idis_app.board) && !g_idis_app.key_down)) {
        g_idis_app.backlight = !g_idis_app.backlight;
        nd_board_backlight_set(g_idis_app.backlight);
    }
    g_idis_app.key_down = pressed;
}

static void poll_adc(void)
{
    int32_t battery = 0;
    if (board_adc_read(ND_ADC_CHANNEL_BATTERY, &battery) != 0) {
        return;
    }
    if (battery < 25000 && !g_idis_app.battery_low) {
        g_idis_app.battery_low = 1;
        (void)nd_idis_event_raise(&g_idis_app.objects.event, 8);
    } else if (battery >= 25000) {
        g_idis_app.battery_low = 0;
    }
}

static void accumulate_energy(void)
{
    nd_measurement_value_t power;
    if (board_measurement_read(&g_idis_app.measurement_ctx,
                               ND_MEASURE_ACTIVE_POWER, 0, &power) != 0) {
        return;
    }
    uint32_t pulses = nd_idis_energy_pulses(
        &g_idis_app.composition.energy_pulse_fraction, power.value,
        g_idis_app.objects.energy.meter_const);
    if (pulses > 0) {
        nd_energy_direction_t direction =
            power.value < 0 ? ND_ENERGY_REVERSE : ND_ENERGY_FORWARD;
        uint8_t tariff =
            nd_ic_register_activation_tariff(&g_idis_app.objects.activation);
        nd_pulse_store_t *store =
            nd_pulse_store_port(&g_idis_app.composition.energy_pulse_storage);
        g_idis_app.composition.energy_appends_pending = 1;
        for (uint32_t pulse = 0; pulse < pulses; pulse++) {
            /* Each append programs flash, and FULL commits before retrying. */
            mcu_clear_watchdog();
            if (nd_idis_energy_pulse(&g_idis_app.objects.energy, store, direction,
                                     tariff) != 0) {
                g_idis_app.composition.energy_commit_pending = 1;
            }
        }
    }
}

static void accumulate_demand(void)
{
    static const nd_measure_quantity_t quantities[ND_IDIS_DEMAND_MAX] = {
        ND_MEASURE_ACTIVE_POWER,          ND_MEASURE_ACTIVE_POWER_EXPORT,
        ND_MEASURE_REACTIVE_POWER_IMPORT, ND_MEASURE_REACTIVE_POWER_EXPORT,
        ND_MEASURE_APPARENT_POWER_IMPORT, ND_MEASURE_APPARENT_POWER_EXPORT,
    };
    for (uint8_t group = 0; group < ND_IDIS_DEMAND_MAX; group++) {
        nd_measurement_value_t power;
        if (board_measurement_read(&g_idis_app.measurement_ctx,
                                   quantities[group], 0, &power) == 0) {
            nd_idis_demand_accumulate(&g_idis_app.objects.demand_acc[group],
                                      (int32_t)power.value);
        }
    }
}


static void save_critical_state(void)
{
    nd_store_t *eeprom = board_store_port(&g_idis_app.board);
    /* Power-off folds the accepted pulses that only the journal holds. With
     * none since the last commit, the base is already current. */
    if (g_idis_app.composition.energy_appends_pending
        && nd_idis_energy_commit(
               &g_idis_app.objects.energy,
               nd_pulse_store_port(&g_idis_app.composition.energy_pulse_storage), eeprom,
               EEPROM_ENERGY) == 0) {
        g_idis_app.composition.energy_appends_pending = 0;
    }
    (void)nd_ic_demand_save(g_idis_app.objects.demand, eeprom, EEPROM_DEMAND);
    (void)nd_ic_extended_save(g_idis_app.objects.max_demand, eeprom, EEPROM_MAX_DEMAND);
    (void)nd_idis_event_save(&g_idis_app.objects.event, eeprom, EEPROM_EVENT);
    (void)nd_idis_power_save(&g_idis_app.objects.power_failure, eeprom, EEPROM_POWER);
    (void)nd_idis_clock_save(&g_idis_app.objects.clock, eeprom, EEPROM_CLOCK);
}

static void on_second(uint32_t seconds)
{
    mcu_clear_watchdog();
    uint8_t maintenance_byte;
    while (board_ir_read(&maintenance_byte)) {
        (void)nd_maintenance_feed(&g_idis_app.composition.maintenance, maintenance_byte,
                                  maintenance_release, &g_idis_app.objects);
    }
    /* One drain is the inter-byte timeout: a prefix left over from an
     * earlier cycle cannot be completed by later, unrelated bytes. */
    nd_maintenance_abort(&g_idis_app.composition.maintenance);
    nd_dlms_datetime_t dlms;
    clock_read(NULL, &dlms);
    nd_datetime_t now = datetime_from_dlms(&dlms);
    nd_clock_value_t clock = {.time = now, .day_of_week = 0};

    int failed = ac_power_off_monitor() != 0u;
    if (failed && !g_idis_app.power_failed) {
        (void)nd_power_failure_off(&g_idis_app.objects.power_failure, &now);
        (void)nd_idis_event_raise(&g_idis_app.objects.event, 1);
        save_critical_state();
    } else if (!failed && g_idis_app.power_failed) {
        (void)nd_power_failure_on(&g_idis_app.objects.power_failure, &now);
        (void)nd_idis_event_raise(&g_idis_app.objects.event, 2);
        save_critical_state();
    }
    g_idis_app.power_failed = failed;

    accumulate_energy();
    accumulate_demand();

    (void)nd_idis_average_on_second(&g_idis_app.objects.average);
    (void)nd_idis_pq_on_second(&g_idis_app.objects.pq, &g_idis_app.composition.measurement,
                               &g_idis_app.objects.event, &now);
    (void)nd_idis_calendar_tick(&g_idis_app.objects.calendar, &now);
    for (uint8_t index = 0; index < 3; index++) {
        (void)nd_idis_calendar_tick(&g_idis_app.objects.calendar_extra[index], &now);
    }
    const nd_idis_calendar_state_t *calendars[4] = {
        &g_idis_app.objects.calendar, &g_idis_app.objects.calendar_extra[0],
        &g_idis_app.objects.calendar_extra[1], &g_idis_app.objects.calendar_extra[2],
    };
    (void)nd_idis_scheduler_on_second_multi(
        &g_idis_app.objects.scheduler, calendars, 4, &g_idis_app.objects.scripts,
        &g_idis_app.composition.port, 1, &now);
    (void)nd_push_on_second(&g_idis_app.objects.push.state, &now);
    (void)nd_idis_single_action_on_second(&g_idis_app.objects.single_action,
                                          &g_idis_app.composition.port, &g_idis_app.objects.scripts,
                                          1, &now);
    (void)nd_idis_monitor_on_second(&g_idis_app.objects.monitors, &g_idis_app.composition.port,
                                    &g_idis_app.objects.scripts, 1);
    (void)nd_idis_limiter_on_second(&g_idis_app.objects.limiter, &g_idis_app.composition.port,
                                    &g_idis_app.objects.scripts, 1);
    for (uint8_t channel = 1; channel <= ND_MBUS_CHANNELS; channel++) {
        (void)nd_idis_mbus_on_second(&g_idis_app.objects.mbus, channel);
    }
    (void)nd_idis_profiles_on_second(&g_idis_app.objects.profiles, &g_idis_app.composition.port,
                                     16, &clock);

    int connected =
        g_idis_app.objects.disconnect.control_state == ND_DISCONNECT_CONNECTED;
    if (connected != g_idis_app.relay_connected) {
        board_relay_apply(connected);
        g_idis_app.relay_connected = connected;
        g_idis_app.relay_feedback_pending = 1;
        g_idis_app.relay_feedback_seconds = 0;
    }
    if (g_idis_app.relay_feedback_pending) {
        g_idis_app.relay_feedback_seconds++;
        if (g_idis_app.relay_feedback_seconds >= 2u) {
            int actual = nd_board_relay_is_off() ? 0 : 1;
            if (actual != g_idis_app.relay_connected) {
                (void)nd_idis_event_raise(&g_idis_app.objects.event, 68);
            }
            g_idis_app.relay_feedback_pending = 0;
        }
    }

    poll_gpio();
    if (seconds % 60u == 0u) {
        poll_adc();
    }
    if (seconds - g_idis_app.last_save >= EEPROM_PERIOD) {
        g_idis_app.last_save = seconds;
        save_persisted();
    }
    if (g_idis_app.composition.energy_commit_pending
        && nd_idis_energy_commit(
               &g_idis_app.objects.energy,
               nd_pulse_store_port(&g_idis_app.composition.energy_pulse_storage),
               board_store_port(&g_idis_app.board), EEPROM_ENERGY) == 0) {
        g_idis_app.composition.energy_commit_pending = 0;
        g_idis_app.composition.energy_appends_pending = 0;
    }
}

int main(void)
{
    g_idis_app.relay_connected = -1;
    mcu_init(NULL);
    cm0_systick_init(1);
    ndv_timer_init();
    board_bind(&g_idis_app.board);
    __asm volatile("cpsie i" ::: "memory");
    mcu_clear_watchdog();

    nd_board_relay_led_set(1);
    board_gpio_init(&g_idis_app.board);
    nd_board_relay_feedback_input_enable();
    nd_board_cover_input_enable();
    nd_board_key_input_enable();
    board_store_init(&g_idis_app.board);
    board_flash_init(&g_idis_app.board);
    nd_idis_calibration_status_t calibration =
        board_measurement_init(&g_idis_app.board, &g_idis_app.composition.measurement_calibration);
    board_uart_init(&g_idis_app.board);
    rtc_init();
    if (init_objects(calibration) != 0) {
        return -1;
    }
    mcu_clear_watchdog();

    uint32_t last_second = 0;
    while (1) {
        uint32_t seconds = baremetal_uptime_seconds();
        if (seconds == last_second) {
            __asm volatile("wfi" ::: "memory");
            continue;
        }
        last_second = seconds;
        on_second(seconds);
    }
}
