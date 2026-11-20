#include "unity.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/classes/nd_ic_profile_generic.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/cosem/nd_idis_profile_table.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static uint8_t flash[ND_IDIS_PROFILE_FLASH_BYTES];
static nd_byte_store_t store;
static nd_idis_objects_t objects;
static nd_cosem_port_t port;

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 11, .day = 11};
    return 0;
}

static int fake_measurement(void *ctx, nd_measure_quantity_t quantity,
                            uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    *out = (nd_measurement_value_t){
        .value = quantity == ND_MEASURE_FREQUENCY ? 5000 : 2300,
        .scaler = quantity == ND_MEASURE_FREQUENCY ? -2 : -1,
        .unit = quantity == ND_MEASURE_FREQUENCY ? ND_MEASUREMENT_UNIT_HERTZ
                                                 : ND_MEASUREMENT_UNIT_VOLT,
        .phase = phase,
    };
    return 0;
}

void setUp(void)
{
    memset(flash, 0, sizeof(flash));
    nd_byte_store_init(&store, flash, sizeof(flash));
}

void tearDown(void) {}

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFu;
    for (size_t index = 0; index < length; index++) {
        crc ^= (uint16_t)data[index] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* The pre-v4 metadata copy: magic, version 3, CRC over the first 30 bytes. */
static void put_legacy_meta(uint8_t *meta)
{
    memset(meta, 0, 32);
    put_u32(meta, 0x49445031u);
    meta[4] = 3;
    put_u16(&meta[5], 64);
    put_u16(&meta[7], 3);
    put_u16(&meta[15], 1);
    put_u16(&meta[30], crc16(meta, 30));
}

typedef struct {
    nd_store_t port;
    nd_store_t inner;
    unsigned long reads;
    unsigned long long bytes;
    unsigned long feeds;
    size_t since_feed;
    size_t max_gap;
    int fail_all;
} scan_probe_t;

static int scan_read(void *ctx, uint32_t offset, uint8_t *buffer,
                     size_t length)
{
    scan_probe_t *probe = ctx;
    probe->reads++;
    probe->bytes += length;
    probe->since_feed += length;
    if (probe->fail_all) {
        return -1;
    }
    return probe->inner.read(probe->inner.ctx, offset, buffer, length);
}

static int scan_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                      size_t length)
{
    scan_probe_t *probe = ctx;
    return probe->inner.write(probe->inner.ctx, offset, buffer, length);
}

static int scan_erase(void *ctx, uint32_t offset, size_t length)
{
    scan_probe_t *probe = ctx;
    return probe->inner.erase(probe->inner.ctx, offset, length);
}

static void scan_feed(void *ctx)
{
    scan_probe_t *probe = ctx;
    probe->feeds++;
    if (probe->since_feed > probe->max_gap) {
        probe->max_gap = probe->since_feed;
    }
    probe->since_feed = 0;
}

static void build(nd_idis_profile_config_t *configs, nd_clock_port_t *clock)
{
    *clock = (nd_clock_port_t){.ctx = NULL, .read = fake_clock,
                               .write = NULL};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_build(
                              configs, ND_IDIS_PROFILE_TABLE_COUNT,
                              nd_byte_store_port(&store), clock, NULL));
}

static void test_table_covers_all_pack3_profiles(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    const nd_obis_t expected[] = {
        ND_OBIS_BILLING_PROFILE,     ND_OBIS_LOAD_PROFILE_1,
        ND_OBIS_LOAD_PROFILE_2,      ND_OBIS_PQ_PROFILE,
        ND_OBIS_STANDARD_EVENT_LOG,  ND_OBIS_FRAUD_LOG,
        ND_OBIS_DISCONNECTOR_LOG,    ND_OBIS_MBUS_EVENT_LOG,
        ND_OBIS_PQ_LOG,              ND_OBIS_COMM_LOG,
        ND_OBIS_COMM_DETAILS_LOG,    ND_OBIS_POWER_FAILURE_LOG,
        ND_OBIS_MBUS_LOAD_PROFILE_1, ND_OBIS_MBUS_LOAD_PROFILE_2,
        ND_OBIS_MBUS_LOAD_PROFILE_3, ND_OBIS_MBUS_LOAD_PROFILE_4,
        ND_OBIS_MBUS_CONTROL_LOG_1,  ND_OBIS_MBUS_CONTROL_LOG_2,
        ND_OBIS_MBUS_CONTROL_LOG_3,  ND_OBIS_MBUS_CONTROL_LOG_4,
        ND_OBIS_DISPLAY_GENERAL,     ND_OBIS_DISPLAY_ALTERNATE,
    };
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        TEST_ASSERT_TRUE(nd_obis_eq(&configs[index].ln, &expected[index]));
        TEST_ASSERT_TRUE(configs[index].capture_count > 0);
        TEST_ASSERT_TRUE(configs[index].clock != NULL);
        TEST_ASSERT_TRUE(configs[index].store != NULL);
        TEST_ASSERT_TRUE(configs[index].capacity > 0);
        TEST_ASSERT_TRUE(configs[index].slot_size > 0);
    }
    TEST_ASSERT_EQUAL_UINT32(900, configs[1].capture_period);
    TEST_ASSERT_EQUAL_UINT32(86400, configs[2].capture_period);
    TEST_ASSERT_EQUAL_UINT32(60, configs[3].capture_period);
    TEST_ASSERT_EQUAL_UINT8(12, configs[3].capture_count);
}

static void test_table_regions_do_not_overlap(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    for (uint8_t left = 0; left < ND_IDIS_PROFILE_TABLE_COUNT; left++) {
        uint32_t left_end =
            configs[left].base_offset
            + PROFILE_REGION_BYTES(configs[left].capacity);
        for (uint8_t right = left + 1; right < ND_IDIS_PROFILE_TABLE_COUNT;
             right++) {
            uint32_t right_end =
                configs[right].base_offset
                + PROFILE_REGION_BYTES(configs[right].capacity);
            TEST_ASSERT_TRUE(left_end <= configs[right].base_offset
                             || right_end <= configs[left].base_offset);
            TEST_ASSERT_FALSE(nd_obis_eq(&configs[left].ln,
                                         &configs[right].ln));
        }
        TEST_ASSERT_TRUE(left_end <= ND_IDIS_PROFILE_FLASH_BYTES);
    }
}

static void test_table_layout_is_sector_aligned(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        TEST_ASSERT_EQUAL_UINT32(0, configs[index].base_offset
                                       % PROFILE_SECTOR_BYTES);
        TEST_ASSERT_EQUAL_UINT16(PROFILE_SLOT_BYTES, configs[index].slot_size);
        TEST_ASSERT_EQUAL_UINT32(PROFILE_SECTOR_BYTES,
                                 configs[index].sector_size);
        TEST_ASSERT_TRUE(configs[index].base_offset
                             + PROFILE_REGION_BYTES(configs[index].capacity)
                         <= ND_IDIS_PROFILE_FLASH_BYTES);
    }
}

static void test_lower_profile_write_stays_inside_its_region(void)
{
    static uint8_t region[16384];
    static uint8_t before[8192];
    static nd_byte_store_t region_store;
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_idis_profiles_t profiles;
    nd_clock_port_t clock;
    build(configs, &clock);
    memset(region, 0, sizeof(region));
    nd_byte_store_init(&region_store, region, sizeof(region));
    configs[0].store = nd_byte_store_port(&region_store);
    memcpy(before, region + configs[1].base_offset, sizeof(before));
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&profiles, configs, 1, 0,
                                                   NULL));
    for (uint16_t index = 0; index < configs[0].capacity + 40u; index++) {
        uint8_t record = (uint8_t)index;
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(
                                     &profiles.items[0].store, &time, 0,
                                     &record, 1));
    }
    TEST_ASSERT_EQUAL_UINT8_ARRAY(before, region + configs[1].base_offset,
                                  sizeof(before));
}

static void test_profiles_init_accepts_table(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, configs,
                                                   ND_IDIS_PROFILE_TABLE_COUNT,
                                                   0, NULL));
    TEST_ASSERT_EQUAL_UINT8(ND_IDIS_PROFILE_TABLE_COUNT, objects.profiles.count);
    TEST_ASSERT_NOT_NULL(nd_idis_profiles_find(&objects.profiles,
                                               &ND_OBIS_BILLING_PROFILE));
    TEST_ASSERT_NOT_NULL(nd_idis_profiles_find(&objects.profiles,
                                               &ND_OBIS_PQ_PROFILE));
    TEST_ASSERT_NOT_NULL(nd_idis_profiles_find(&objects.profiles,
                                               &ND_OBIS_DISPLAY_ALTERNATE));
}

static void test_every_profile_captures(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock;
    build(configs, &clock);

    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    objects = (nd_idis_objects_t){0};
    objects.clock.port = &clock;
    objects.reg.measurement = &measurement;
    objects.reg.energy = &objects.energy;
    nd_energy_set_init(&objects.energy, 2000);
    (void)nd_energy_set_add_pulses(&objects.energy,
                                   ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 0, 4000);
    nd_idis_average_init(&objects.average, &measurement);
    port = (nd_cosem_port_t){
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, configs,
                                                   ND_IDIS_PROFILE_TABLE_COUNT,
                                                   0, NULL));
    nd_idis_profiles_bind_port(&objects.profiles, &port);

    nd_clock_value_t now = {
        .time = {.year = 2026, .month = 11, .day = 11, .hour = 9},
    };
    for (uint8_t index = 0; index < objects.profiles.count; index++) {
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(
                                  &objects.profiles.items[index], &port, 16,
                                  &now));
        TEST_ASSERT_EQUAL_UINT16(
            1, nd_profile_store_entries(&objects.profiles.items[index].store));
    }
}

static void test_legacy_image_at_moved_base_is_detected(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    build(configs, &clock);
    /* Index 3 is PQ; the v4 layout moved its base to 172032. */
    TEST_ASSERT_EQUAL_UINT32(172032u, configs[3].base_offset);
    TEST_ASSERT_EQUAL_UINT32(167936u,
                             configs[3].store_opts.legacy_base_offset);
    put_legacy_meta(flash + 167936u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, nd_byte_store_port(&store),
                              &clock, NULL, 0));
    TEST_ASSERT_EQUAL_UINT8(ND_IDIS_PROFILE_TABLE_COUNT,
                            objects.profiles.count);
    nd_idis_profile_state_t *pq =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_PQ_PROFILE);
    nd_idis_profile_state_t *billing =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_BILLING_PROFILE);
    TEST_ASSERT_NOT_NULL(pq);
    TEST_ASSERT_NOT_NULL(billing);
    uint8_t record = 0x31;
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    /* Retain holds every store: an active v4 ring could otherwise overwrite
     * the old image, whose region overlaps a neighbouring v4 region. */
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_append(&pq->store, &time, 0,
                                                      &record, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_append(&billing->store, &time, 0,
                                                      &record, 1));
}

static void test_legacy_reset_policy_extinguishes_image(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    put_legacy_meta(flash + 167936u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, nd_byte_store_port(&store),
                              &clock, NULL, 1));
    nd_idis_profile_state_t *pq =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_PQ_PROFILE);
    TEST_ASSERT_NOT_NULL(pq);
    uint8_t record = 0x31;
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&pq->store, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, nd_byte_store_port(&store),
                              &clock, NULL, 0));
}

typedef struct {
    nd_store_t port;
    uint8_t *data;
    size_t size;
    int fail_erase;
    /* 1-based erase call that fails; 0 = never. */
    unsigned fail_erase_at;
    unsigned erases;
    /* Fail marker reads once the marker sector has been erased twice (the
     * activate erase and the clear erase). */
    int fail_marker_read_after_clear;
    unsigned marker_erases;
    /* Watchdog feed sequence: transactions between two feeds. */
    unsigned feeds;
    size_t since_feed;
    size_t max_gap;
} ram_store_t;

static int ram_read(void *ctx, uint32_t offset, uint8_t *buffer,
                    size_t length)
{
    ram_store_t *ram = ctx;
    ram->since_feed++;
    if (ram->fail_marker_read_after_clear && ram->marker_erases >= 2u
        && offset >= ND_IDIS_RELEASE_MARKER_OFFSET) {
        return -1;
    }
    if (offset > ram->size || length > ram->size - offset) {
        return -1;
    }
    memcpy(buffer, ram->data + offset, length);
    return 0;
}

static int ram_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                     size_t length)
{
    ram_store_t *ram = ctx;
    ram->since_feed++;
    if (offset > ram->size || length > ram->size - offset) {
        return -1;
    }
    memcpy(ram->data + offset, buffer, length);
    return 0;
}

static int ram_erase(void *ctx, uint32_t offset, size_t length)
{
    ram_store_t *ram = ctx;
    ram->erases++;
    ram->since_feed++;
    if (offset == ND_IDIS_RELEASE_MARKER_OFFSET
        && length == ND_IDIS_RELEASE_MARKER_BYTES) {
        ram->marker_erases++;
    }
    if (ram->fail_erase
        || (ram->fail_erase_at != 0u && ram->erases >= ram->fail_erase_at)
        || offset > ram->size || length > ram->size - offset) {
        return -1;
    }
    memset(ram->data + offset, 0xFF, length);
    return 0;
}

static void ram_feed(void *ctx)
{
    ram_store_t *ram = ctx;
    ram->feeds++;
    if (ram->since_feed > ram->max_gap) {
        ram->max_gap = ram->since_feed;
    }
    ram->since_feed = 0;
}

/* Sector erase comes from this port, so the ring really moves a 4 KiB sector
 * and the overlap between Billing and the pre-v4 LOAD_PROFILE_1 base is
 * exercised. */
static void ram_store_init(ram_store_t *ram, uint8_t *data, size_t size)
{
    *ram = (ram_store_t){
        .port = {.ctx = ram, .read = ram_read, .write = ram_write,
                 .erase = ram_erase},
        .data = data,
        .size = size,
    };
}

static void test_reset_refused_while_legacy_image_is_held(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    build(configs, &clock);
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    /* 32 Billing records park its head on the sector that holds the pre-v4
     * LOAD_PROFILE_1 metadata at 4096; a per-store reset would erase it. */
    nd_profile_store_t billing;
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_init(
        &billing, &ram.port, configs[0].base_offset, configs[0].capacity,
        configs[0].slot_size, configs[0].sector_size, NULL));
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    for (uint8_t index = 0; index < 32; index++) {
        TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&billing, &time, 0,
                                                         &index, 1));
    }
    uint8_t meta[32];
    put_legacy_meta(meta);
    memcpy(flash + 4096u, meta, sizeof(meta));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    int result = nd_ic_profile_generic_action(&objects.profiles, 0,
                                              &ND_OBIS_BILLING_PROFILE, 1,
                                              NULL, NULL);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(meta, flash + 4096u, sizeof(meta));
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE, result);
}

static void test_reset_failure_is_reported(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    ram.fail_erase = 1;
    TEST_ASSERT_EQUAL_INT(ND_ERR_OPERATION_NOT_POSSIBLE,
                          nd_ic_profile_generic_action(
                              &objects.profiles, 0,
                              &ND_OBIS_BILLING_PROFILE, 1, NULL, NULL));
}

static void test_reset_allowed_without_legacy_hold(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    nd_idis_profile_state_t *billing =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_BILLING_PROFILE);
    TEST_ASSERT_NOT_NULL(billing);
    uint8_t record = 0x42;
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&billing->store, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_ic_profile_generic_action(
                              &objects.profiles, 0,
                              &ND_OBIS_BILLING_PROFILE, 1, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&billing->store));
}

static void reset_parameter(uint32_t confirmation, uint8_t *buffer,
                            size_t *length)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, 8);
    nd_axdr_put_tag(&writer, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&writer, confirmation);
    *length = writer.len;
}

static void test_release_legacy_resumes_appends(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    build(configs, &clock);
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    nd_idis_profile_state_t *lp1 =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_LOAD_PROFILE_1);
    TEST_ASSERT_NOT_NULL(lp1);
    uint8_t record = 0x42;
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_append(&lp1->store, &time, 0,
                                                      &record, 1));

    /* Management subject only: public, pre-established and CIP are refused. */
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_release_legacy(&objects, ND_IDIS_SAP_PUBLIC,
                                     ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_release_legacy(&objects, ND_IDIS_SAP_PREESTABLISHED,
                                     ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_release_legacy(&objects, ND_IDIS_SAP_CIP,
                                     ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    /* A wrong value is not a release and the method-1 reset stays refused. */
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_release_legacy(&objects, ND_IDIS_SAP_MANAGEMENT,
                                     0x12345678u));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_release_legacy(
                   &objects, ND_IDIS_SAP_MANAGEMENT,
                   ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&lp1->store, &time, 0,
                                                     &record, 1));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&lp1->store));

    /* The cleared marker and the reset marks keep the release across a boot:
     * no ring probes for a pre-v4 image and no hold is re-applied. */
    nd_idis_release_marker_t marker = {
        .store = configs[0].store,
        .offset = ND_IDIS_RELEASE_MARKER_OFFSET,
    };
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(
                                 &objects.profiles, configs,
                                 ND_IDIS_PROFILE_TABLE_COUNT, 0, &marker));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&lp1->store));
}

static void test_release_without_hold_keeps_profile_records(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    build(configs, &clock);
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    nd_idis_profile_state_t *billing =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_BILLING_PROFILE);
    TEST_ASSERT_NOT_NULL(billing);
    uint8_t record = 0x42;
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&billing->store, &time, 0,
                                                     &record, 1));

    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_release_legacy(
                   &objects, ND_IDIS_SAP_MANAGEMENT,
                   ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&billing->store));
}

static void test_release_resets_every_profile_store(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    build(configs, &clock);
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    nd_idis_profile_state_t *billing =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_BILLING_PROFILE);
    TEST_ASSERT_NOT_NULL(billing);
    uint8_t record = 0x42;
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(0, nd_profile_store_append(&billing->store, &time, 0,
                                                     &record, 1));

    /* A legacy image elsewhere holds the table; the release also discards
     * this non-legacy ring's records. */
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&billing->store));

    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_release_legacy(
                   &objects, ND_IDIS_SAP_MANAGEMENT,
                   ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&billing->store));
}

/* The release loop resets 22 rings, each with a sector erase and a mark
 * write. on_second feeds once at entry, so each reset must feed too; a feed
 * after every reset leaves at most two store transactions between feeds. */
static void test_release_feeds_watchdog_between_rings(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    nd_profile_watchdog_t watchdog = {.ctx = &ram, .feed = ram_feed};
    TEST_ASSERT_EQUAL_INT(
        ND_IDIS_PROFILE_TABLE_COUNT,
        nd_idis_profile_table_build(configs, ND_IDIS_PROFILE_TABLE_COUNT,
                                    &ram.port, &clock, &watchdog));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(
                                 &objects.profiles, configs,
                                 ND_IDIS_PROFILE_TABLE_COUNT, 0, NULL));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    ram.feeds = 0;
    ram.since_feed = 0;
    ram.max_gap = 0;
    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_profiles_release_legacy(&objects.profiles));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(ND_IDIS_PROFILE_TABLE_COUNT, ram.feeds);
    TEST_ASSERT_LESS_OR_EQUAL_size_t(2, ram.max_gap);
}

static void test_release_failure_reholds_every_store(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    build(configs, &clock);
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    /* Erase 1 is the marker sector; erase 2 is the first ring reset. */
    ram.fail_erase_at = 2u;

    TEST_ASSERT_EQUAL_INT(
        ND_ERR_OPERATION_NOT_POSSIBLE,
        nd_cosem_port_release_legacy(&objects, ND_IDIS_SAP_MANAGEMENT,
                                     ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    ram.fail_erase_at = 0u;
    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_release_legacy(
                   &objects, ND_IDIS_SAP_MANAGEMENT,
                   ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
}

static void release_failure_stays_held_after_reboot(unsigned fail_erase_at)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    ram.fail_erase_at = fail_erase_at;
    TEST_ASSERT_EQUAL_INT(-1,
                          nd_idis_profiles_release_legacy(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    /* The first rings now carry reset marks, so their legacy probe is off and
     * the retained image is no longer detectable. The release failure must
     * outlive the reboot anyway, or a power cut silently completes it. */
    ram.fail_erase_at = 0u;
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(-1, nd_profile_store_append(
                                  &objects.profiles.items[0].store,
                                  &(nd_datetime_t){.year = 2026, .month = 11,
                                                   .day = 11},
                                  0, (const uint8_t *)"x", 1));
}

static void test_release_failure_at_last_ring_survives_reboot(void)
{
    /* Erase 1 activates the marker; ring resets follow. The last ring is the
     * one whose failure leaves 21 reset marks behind. */
    release_failure_stays_held_after_reboot(1u + ND_IDIS_PROFILE_TABLE_COUNT);
}

static void test_release_failure_mid_ring_survives_reboot(void)
{
    release_failure_stays_held_after_reboot(1u + ND_IDIS_PROFILE_TABLE_COUNT / 2u);
}

/* A marker sector whose 512 slots are all ACTIVE cannot accept the retry
 * record, so activation fails closed and every store stays held. */
static void test_marker_full_refuses_release(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    for (uint16_t slot = 0; slot < ND_IDIS_RELEASE_MARKER_BYTES / 8u; slot++) {
        uint8_t *record = flash + ND_IDIS_RELEASE_MARKER_OFFSET + slot * 8u;
        put_u32(&record[0], 0x52454C31u);
        put_u32(&record[4], (uint32_t)~0x52454C31u);
    }
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(-1,
                          nd_idis_profiles_release_legacy(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
}

/* A record that is neither the ACTIVE pair nor blank is a torn write. It must
 * not hold by itself; with a real legacy image the release erases it. */
static void test_torn_marker_record_is_not_active(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    uint8_t *torn = flash + ND_IDIS_RELEASE_MARKER_OFFSET;
    put_u32(&torn[0], 0x52454C31u);
    put_u32(&torn[4], 0x52454C31u); /* not the complement */
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));

    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_profiles_release_legacy(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
    for (uint16_t index = 0; index < ND_IDIS_RELEASE_MARKER_BYTES; index++) {
        TEST_ASSERT_EQUAL_UINT8(
            0xFF, flash[ND_IDIS_RELEASE_MARKER_OFFSET + index]);
    }
}

/* The clear erase has already run when its proving scan fails. The release
 * reports failure and re-holds, though every ring is reset and the marker is
 * blank; a retry (as after a watchdog reboot) finishes idempotently. */
static void test_failed_clear_scan_reports_after_release_complete(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));

    ram.fail_marker_read_after_clear = 1;
    TEST_ASSERT_EQUAL_INT(-1,
                          nd_idis_profiles_release_legacy(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    TEST_ASSERT_EQUAL_UINT(2, ram.marker_erases);
    for (uint8_t index = 0; index < ND_IDIS_PROFILE_TABLE_COUNT; index++) {
        TEST_ASSERT_EQUAL_UINT16(
            0, nd_profile_store_entries(&objects.profiles.items[index].store));
    }
    for (uint16_t index = 0; index < ND_IDIS_RELEASE_MARKER_BYTES; index++) {
        TEST_ASSERT_EQUAL_UINT8(
            0xFF, flash[ND_IDIS_RELEASE_MARKER_OFFSET + index]);
    }

    ram.fail_marker_read_after_clear = 0;
    TEST_ASSERT_EQUAL_INT(0,
                          nd_idis_profiles_release_legacy(&objects.profiles));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
}

static void test_release_refused_for_remote_management(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));

    objects.remote_client = 1;
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_ACCESS_DENIED,
        nd_cosem_port_release_legacy(&objects, ND_IDIS_SAP_MANAGEMENT,
                                     ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
    objects.remote_client = 0;

    TEST_ASSERT_EQUAL_INT(
        ND_OK, nd_cosem_port_release_legacy(
                   &objects, ND_IDIS_SAP_MANAGEMENT,
                   ND_IDIS_LEGACY_RELEASE_CONFIRMATION));
}

static void test_profile_method1_cannot_release(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    ram_store_t ram;
    ram_store_init(&ram, flash, sizeof(flash));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &ram.port, &clock, NULL, 0));

    /* Class-7 method 1 has no parameters by definition, so RELG cannot ride
     * it; a parameter-carrying ACTION must not wipe the table. */
    uint8_t parameter[8];
    size_t length = 0;
    reset_parameter(ND_IDIS_LEGACY_RELEASE_CONFIRMATION, parameter, &length);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, length);
    TEST_ASSERT_EQUAL_INT(
        ND_ERR_OPERATION_NOT_POSSIBLE,
        nd_ic_profile_generic_action(&objects.profiles, ND_IDIS_SAP_MANAGEMENT,
                                     &ND_OBIS_STANDARD_EVENT_LOG, 1, &in,
                                     NULL));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
}

static void test_boot_refuses_zero_profiles(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    scan_probe_t probe = {
        .inner = *nd_byte_store_port(&store),
        .fail_all = 1,
    };
    probe.port = (nd_store_t){.ctx = &probe, .read = scan_read,
                              .write = scan_write, .erase = scan_erase};
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_profile_table_boot(
                                  &objects.profiles, &probe.port, &clock,
                                  NULL));
    probe.fail_all = 0;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profile_table_boot(
                                 &objects.profiles, nd_byte_store_port(&store),
                                 &clock, NULL));
    TEST_ASSERT_EQUAL_UINT8(ND_IDIS_PROFILE_TABLE_COUNT,
                            objects.profiles.count);
}

static void test_legacy_hold_is_observable(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    memset(&objects.profiles, 0, sizeof(objects.profiles));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_legacy_held(&objects.profiles));
    put_legacy_meta(flash + 4096u);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, nd_byte_store_port(&store),
                              &clock, NULL, 0));
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_legacy_held(&objects.profiles));
}

static void test_boot_scan_feeds_watchdog_within_budget(void)
{
    nd_clock_port_t clock = {.read = fake_clock};
    scan_probe_t probe = {.inner = *nd_byte_store_port(&store)};
    probe.port = (nd_store_t){.ctx = &probe, .read = scan_read,
                              .write = scan_write, .erase = scan_erase};
    nd_profile_watchdog_t watchdog = {.ctx = &probe, .feed = scan_feed};
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_PROFILE_TABLE_COUNT,
                          nd_idis_profile_table_init(
                              &objects.profiles, &probe.port, &clock,
                              &watchdog, 0));
    clock_gettime(CLOCK_MONOTONIC, &end);
    double host_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0
                   + (double)(end.tv_nsec - start.tv_nsec) / 1e6;
    /* Every store read is followed by a feed. The largest read is one slot,
     * so the modelled bit-bang gap is 128 B * 5 us/B = 0.64 ms against the
     * 2 s watchdog. Device time still needs HIL. */
    TEST_ASSERT_EQUAL_UINT(probe.reads, probe.feeds);
    TEST_ASSERT_LESS_OR_EQUAL_size_t(PROFILE_SLOT_BYTES, probe.max_gap);
    TEST_ASSERT_TRUE(probe.reads >= 8000);
    printf("boot scan: reads=%lu bytes=%llu feeds=%lu max_gap=%zu B "
           "host_ms=%.3f model_gap_ms=%.3f\n",
           probe.reads, probe.bytes, probe.feeds, probe.max_gap, host_ms,
           (double)probe.max_gap * 5e-3);
}

static void test_build_rejects_invalid_arguments(void)
{
    nd_idis_profile_config_t configs[ND_IDIS_PROFILE_TABLE_COUNT];
    nd_clock_port_t clock = {.read = fake_clock};
    nd_store_t *store_port = nd_byte_store_port(&store);

    TEST_ASSERT_EQUAL_INT(-1, nd_idis_profile_table_build(
                                  NULL, ND_IDIS_PROFILE_TABLE_COUNT,
                                  store_port, &clock, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_profile_table_build(
                                  configs, ND_IDIS_PROFILE_TABLE_COUNT - 1,
                                  store_port, &clock, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_profile_table_build(
                                  configs, ND_IDIS_PROFILE_TABLE_COUNT, NULL,
                                  &clock, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_profile_table_build(
                                  configs, ND_IDIS_PROFILE_TABLE_COUNT,
                                  store_port, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_profile_table_init(NULL, store_port,
                                                         &clock, NULL, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_table_covers_all_pack3_profiles);
    RUN_TEST(test_table_regions_do_not_overlap);
    RUN_TEST(test_table_layout_is_sector_aligned);
    RUN_TEST(test_lower_profile_write_stays_inside_its_region);
    RUN_TEST(test_profiles_init_accepts_table);
    RUN_TEST(test_every_profile_captures);
    RUN_TEST(test_legacy_image_at_moved_base_is_detected);
    RUN_TEST(test_legacy_reset_policy_extinguishes_image);
    RUN_TEST(test_reset_refused_while_legacy_image_is_held);
    RUN_TEST(test_reset_failure_is_reported);
    RUN_TEST(test_reset_allowed_without_legacy_hold);
    RUN_TEST(test_release_legacy_resumes_appends);
    RUN_TEST(test_release_without_hold_keeps_profile_records);
    RUN_TEST(test_release_resets_every_profile_store);
    RUN_TEST(test_release_feeds_watchdog_between_rings);
    RUN_TEST(test_release_failure_reholds_every_store);
    RUN_TEST(test_release_failure_at_last_ring_survives_reboot);
    RUN_TEST(test_release_failure_mid_ring_survives_reboot);
    RUN_TEST(test_marker_full_refuses_release);
    RUN_TEST(test_torn_marker_record_is_not_active);
    RUN_TEST(test_failed_clear_scan_reports_after_release_complete);
    RUN_TEST(test_release_refused_for_remote_management);
    RUN_TEST(test_profile_method1_cannot_release);
    RUN_TEST(test_boot_refuses_zero_profiles);
    RUN_TEST(test_legacy_hold_is_observable);
    RUN_TEST(test_boot_scan_feeds_watchdog_within_budget);
    RUN_TEST(test_build_rejects_invalid_arguments);
    return UNITY_END();
}
