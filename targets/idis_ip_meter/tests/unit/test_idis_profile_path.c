#include "unity.h"

#include <string.h>

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"
#include "targets/idis_ip_meter/cosem/nd_idis_capture.h"

static nd_dlms_datetime_t fake_clock_now;
static uint32_t fake_measurement_base;

void setUp(void)
{
    fake_clock_now = (nd_dlms_datetime_t){
        .year = 2026,
        .month = 11,
        .day = 11,
        .hour = 0,
        .minute = 0,
        .second = 0,
    };
    fake_measurement_base = 2300;
}
void tearDown(void) {}

static int fake_clock_read(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = fake_clock_now;
    return 0;
}

static int fake_measurement(void *ctx, nd_measure_quantity_t quantity,
                            uint8_t phase, nd_measurement_value_t *out)
{
    (void)ctx;
    (void)quantity;
    *out = (nd_measurement_value_t){
        .value = fake_measurement_base + phase,
        .scaler = -1,
        .unit = ND_MEASUREMENT_UNIT_VOLT,
        .phase = phase,
    };
    return 0;
}

static void test_profile_action_capture_and_get(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));

    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    const nd_capture_object_t captures[] = {
        {ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0},
        {ND_IC_REGISTER, ND_OBIS_VOLTAGE_L1, 2, 0},
    };
    nd_idis_objects_t objects = {0};
    objects.clock.port = &clock_port;
    objects.reg.measurement = &measurement;
    nd_idis_profile_config_t config = {
        .ln = ND_OBIS_BILLING_PROFILE,
        .store = nd_byte_store_port(&store),
        .base_offset = 0,
        .capacity = 4,
        .slot_size = 128,
        .clock = &clock_port,
        .capture_count = 2,
        .capture_period = 0,
    };
    config.captures[0] = captures[0];
    config.captures[1] = captures[1];
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, &config, 1));
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects.profiles, &port);

    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_PROFILE_GENERIC,
                                               &ND_OBIS_BILLING_PROFILE, 2,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&objects.profiles.items[0].store));
    TEST_ASSERT_EQUAL_UINT32(1, objects.event.counters[1]);

    uint8_t buffer[64] = {0};
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 7, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_size_t(5, out.len);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(1, ((uint32_t)buffer[1] << 24)
                                    | ((uint32_t)buffer[2] << 16)
                                    | ((uint32_t)buffer[3] << 8) | buffer[4]);

    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]); /* array */
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]); /* structure */
    TEST_ASSERT_EQUAL_UINT8(2, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[4]); /* clock */
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[17]); /* voltage register */

    uint8_t range[4] = {ND_AXDR_INTEGER, 1, ND_AXDR_INTEGER, 1};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, range, sizeof(range));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_PROFILE_GENERIC,
                                               &ND_OBIS_BILLING_PROFILE, 3,
                                               &in, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[2]);

    uint8_t index[2] = {ND_AXDR_INTEGER, 1};
    nd_axdr_reader_init(&in, index, sizeof(index));
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_PROFILE_GENERIC,
                                               &ND_OBIS_BILLING_PROFILE, 4,
                                               &in, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
}

static void test_profile_on_second_uses_capture_period(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    const nd_capture_object_t captures[] = {
        {ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0},
    };
    nd_idis_objects_t objects = {0};
    objects.clock.port = &clock_port;
    nd_idis_profile_config_t config = {
        .ln = ND_OBIS_LOAD_PROFILE_1,
        .store = nd_byte_store_port(&store),
        .base_offset = 0,
        .capacity = 4,
        .slot_size = 128,
        .clock = &clock_port,
        .capture_count = 1,
        .capture_period = 60,
    };
    config.captures[0] = captures[0];
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, &config, 1));
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects.profiles, &port);

    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11,
                                     .hour = 0, .minute = 0, .second = 0}};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_profiles_on_second(&objects.profiles, &port,
                                                       16, &now));
    now.time.second = 1;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_on_second(&objects.profiles, &port,
                                                       16, &now));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&objects.profiles.items[0].store));
}

static void test_multiple_profiles_are_independent(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    const nd_capture_object_t captures[] = {
        {ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0},
    };
    nd_idis_profile_config_t configs[2] = {
        {.ln = ND_OBIS_BILLING_PROFILE,
         .store = nd_byte_store_port(&store),
         .base_offset = 0,
         .capacity = 2,
         .slot_size = 128,
         .clock = &clock_port,
         .capture_count = 1,
         .capture_period = 0},
        {.ln = ND_OBIS_LOAD_PROFILE_1,
         .store = nd_byte_store_port(&store),
         .base_offset = 1024,
         .capacity = 2,
         .slot_size = 128,
         .clock = &clock_port,
         .capture_count = 1,
         .capture_period = 0},
    };
    configs[0].captures[0] = captures[0];
    configs[1].captures[0] = captures[0];

    nd_idis_objects_t objects = {0};
    objects.clock.port = &clock_port;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_profiles_init(&objects.profiles, configs, 2));
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects.profiles, &port);

    nd_idis_profile_state_t *billing =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_BILLING_PROFILE);
    nd_idis_profile_state_t *lp =
        nd_idis_profiles_find(&objects.profiles, &ND_OBIS_LOAD_PROFILE_1);
    TEST_ASSERT_NOT_NULL(billing);
    TEST_ASSERT_NOT_NULL(lp);
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    TEST_ASSERT_EQUAL_INT(ND_OK, nd_idis_profile_capture(billing, &port, 16, &now));
    TEST_ASSERT_EQUAL_UINT16(1, nd_profile_store_entries(&billing->store));
    TEST_ASSERT_EQUAL_UINT16(0, nd_profile_store_entries(&lp->store));
}


static void put_clock_value(nd_axdr_writer_t *out, uint8_t hour, uint8_t minute,
                            uint8_t second)
{
    nd_axdr_put_tag(out, ND_AXDR_DATE_TIME);
    nd_axdr_put_u16(out, 2026);
    nd_axdr_put_u8(out, 11);
    nd_axdr_put_u8(out, 11);
    nd_axdr_put_u8(out, 0);
    nd_axdr_put_u8(out, hour);
    nd_axdr_put_u8(out, minute);
    nd_axdr_put_u8(out, second);
    nd_axdr_put_u8(out, 0);
    nd_axdr_put_i16(out, 0);
    nd_axdr_put_u8(out, 0);
}

static void put_capture_definition(nd_axdr_writer_t *out, uint16_t class_id,
                                   const nd_obis_t *ln, uint8_t attribute_id)
{
    nd_axdr_put_structure_header(out, 4);
    nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(out, class_id);
    nd_axdr_put_tag(out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(out, 6);
    nd_axdr_put_octets(out, (const uint8_t *)ln, 6);
    nd_axdr_put_tag(out, ND_AXDR_INTEGER);
    nd_axdr_put_u8(out, attribute_id);
    nd_axdr_put_tag(out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(out, 0);
}

static size_t make_entry_selector(uint8_t *buffer, size_t capacity,
                                  uint32_t from_entry, uint32_t to_entry,
                                  uint16_t from_col, uint16_t to_col)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, capacity);
    nd_axdr_put_structure_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&writer, 2);
    nd_axdr_put_structure_header(&writer, 4);
    nd_axdr_put_tag(&writer, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&writer, from_entry);
    nd_axdr_put_tag(&writer, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&writer, to_entry);
    nd_axdr_put_tag(&writer, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&writer, from_col);
    nd_axdr_put_tag(&writer, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&writer, to_col);
    return writer.len;
}

static size_t make_clock_range_selector(uint8_t *buffer, size_t capacity,
                                        uint8_t from_hour, uint8_t from_minute,
                                        uint8_t to_hour, uint8_t to_minute)
{
    nd_axdr_writer_t writer;
    nd_axdr_writer_init(&writer, buffer, capacity);
    nd_axdr_put_structure_header(&writer, 2);
    nd_axdr_put_tag(&writer, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&writer, 1);
    nd_axdr_put_structure_header(&writer, 4);
    put_capture_definition(&writer, ND_IC_CLOCK, &ND_OBIS_CLOCK, 2);
    put_clock_value(&writer, from_hour, from_minute, 0);
    put_clock_value(&writer, to_hour, to_minute, 0);
    nd_axdr_put_array_header(&writer, 0);
    return writer.len;
}

static nd_idis_profile_state_t *setup_profile(nd_idis_objects_t *objects,
                                              nd_byte_store_t *store,
                                              uint8_t *memory, size_t size,
                                              nd_clock_port_t *clock_port,
                                              nd_measurement_port_t *measurement,
                                              uint8_t compressed,
                                              uint16_t period,
                                              uint8_t with_status,
                                              uint8_t delta)
{
    nd_byte_store_init(store, memory, size);
    const nd_capture_object_t clock_capture = {ND_IC_CLOCK, ND_OBIS_CLOCK, 2, 0};
    const nd_capture_object_t voltage_capture = {ND_IC_REGISTER, ND_OBIS_VOLTAGE_L1,
                                                 2, 0};
    const nd_capture_object_t status_capture = {ND_IC_DATA, ND_OBIS_PROFILE_STATUS_1,
                                                2, 0};
    nd_idis_profile_config_t config = {
        .ln = ND_OBIS_BILLING_PROFILE,
        .store = nd_byte_store_port(store),
        .base_offset = 0,
        .capacity = 8,
        .slot_size = 128,
        .clock = clock_port,
        .capture_count = with_status ? 3 : 2,
        .capture_period = period,
        .compressed = compressed,
        .delta = delta,
    };
    config.captures[0] = clock_capture;
    if (with_status) {
        config.captures[1] = status_capture;
        config.captures[2] = voltage_capture;
    } else {
        config.captures[1] = voltage_capture;
    }
    memset(objects, 0, sizeof(*objects));
    objects->clock.port = clock_port;
    objects->reg.measurement = measurement;
    if (nd_idis_profiles_init(&objects->profiles, &config, 1) != 0) {
        return NULL;
    }
    nd_cosem_port_t port = {
        .ctx = objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_idis_profiles_bind_port(&objects->profiles, &port);
    return &objects->profiles.items[0];
}

static void test_profile_selective_entry(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 0, 0, 0, 0);
    TEST_ASSERT_NOT_NULL(profile);
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11,
                                     .hour = 0, .minute = 0, .second = 0}};
    for (uint8_t index = 0; index < 3; index++) {
        now.time.minute = index;
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(profile, &port, 16, &now));
    }
    uint8_t selector[128];
    size_t selector_length = make_entry_selector(selector, sizeof(selector),
                                                2, 3, 1, 2);
    nd_axdr_reader_t selector_reader;
    nd_axdr_reader_init(&selector_reader, selector, selector_length);
    uint8_t buffer[128];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2,
                                            &selector_reader, &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[17]);

    selector_length = make_entry_selector(selector, sizeof(selector), 2, 3, 2, 2);
    nd_axdr_reader_init(&selector_reader, selector, selector_length);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2,
                                            &selector_reader, &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[4]);
}

static void test_profile_selective_clock_range(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 0, 60, 0, 0);
    TEST_ASSERT_NOT_NULL(profile);
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    for (uint8_t index = 0; index < 3; index++) {
        now.time.minute = index;
        fake_clock_now = (nd_dlms_datetime_t){
            .year = 2026, .month = 11, .day = 11, .minute = index};
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(profile, &port, 16, &now));
    }
    uint8_t selector[128];
    size_t selector_length = make_clock_range_selector(
        selector, sizeof(selector), 0, 1, 0, 2);
    nd_axdr_reader_t selector_reader;
    nd_axdr_reader_init(&selector_reader, selector, selector_length);
    uint8_t buffer[128];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2,
                                            &selector_reader, &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
}

static void test_profile_compresses_repeated_values(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 1, 60, 0, 0);
    TEST_ASSERT_NOT_NULL(profile);
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    for (uint8_t index = 0; index < 3; index++) {
        now.time.minute = index;
        fake_clock_now = (nd_dlms_datetime_t){
            .year = 2026, .month = 11, .day = 11, .minute = index};
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(profile, &port, 16, &now));
    }
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[17]);
    /* second and third entries carry only the clock; voltage is null. */
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[22]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[23]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[24]); /* clock = previous + period */
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[25]); /* repeated voltage */
}


static void after_capture_count(void *ctx)
{
    (*(unsigned int *)ctx)++;
}

static void test_profile_after_capture_callback(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.read = fake_clock_read};
    nd_measurement_port_t measurement = {.read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 0, 0, 0, 0);
    TEST_ASSERT_NOT_NULL(profile);
    unsigned int calls = 0;
    profile->after_capture = after_capture_count;
    profile->after_capture_ctx = &calls;
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_idis_profile_capture(profile, &port, 16, &now));
    TEST_ASSERT_EQUAL_UINT(1, calls);
}

static void test_axdr_skip_value_handles_nested_values(void)
{
    uint8_t buffer[32];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_DOUBLE_LONG_UNSIGNED);
    nd_axdr_put_u32(&out, 0x12345678);
    nd_axdr_put_structure_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_UNSIGNED);
    nd_axdr_put_u8(&out, 1);
    nd_axdr_put_array_header(&out, 2);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 10);
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, 20);
    nd_axdr_put_tag(&out, ND_AXDR_OCTET_STRING);
    nd_axdr_put_length(&out, 6);
    nd_axdr_put_octets(&out, (const uint8_t *)"ABCDEF", 6);
    TEST_ASSERT_TRUE(nd_axdr_writer_ok(&out));

    uint8_t long_header[4];
    nd_axdr_writer_t header;
    nd_axdr_writer_init(&header, long_header, sizeof(long_header));
    nd_axdr_put_array_header(&header, 960);
    TEST_ASSERT_EQUAL_UINT8(0x01, long_header[0]);
    TEST_ASSERT_EQUAL_UINT8(0x82, long_header[1]);
    TEST_ASSERT_EQUAL_UINT8(0x03, long_header[2]);
    TEST_ASSERT_EQUAL_UINT8(0xC0, long_header[3]);

    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_skip_value(&in));
    TEST_ASSERT_EQUAL_size_t(5, in.pos);
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_skip_value(&in));
    TEST_ASSERT_EQUAL_INT(0, nd_axdr_skip_value(&in));
    TEST_ASSERT_EQUAL_size_t(0, nd_axdr_remaining(&in));
}


static void test_profile_compresses_status(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 1, 0, 1, 0);
    TEST_ASSERT_NOT_NULL(profile);
    TEST_ASSERT_EQUAL_UINT8(3, profile->capture_count);
    TEST_ASSERT_EQUAL_UINT8(ND_IC_DATA, profile->captures[1].class_id);
    TEST_ASSERT_TRUE(nd_obis_eq(&profile->captures[1].ln, &ND_OBIS_PROFILE_STATUS_1));
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    const uint8_t statuses[] = {0, 0, 1};
    for (uint8_t index = 0; index < 3; index++) {
        now.time.minute = index;
        fake_clock_now = (nd_dlms_datetime_t){
            .year = 2026, .month = 11, .day = 11, .minute = index};
        nd_idis_profile_set_status(profile, statuses[index]);
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(profile, &port, 16, &now));
    }
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[17]); /* first status */
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[18]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[24]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[25]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[39]); /* repeated status */
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[41]);
    TEST_ASSERT_EQUAL_UINT8(3, buffer[42]);
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[56]); /* changed status */
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[57]);

    profile->ln = ND_OBIS_LOAD_PROFILE_1;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_DATA,
                                            &ND_OBIS_PROFILE_STATUS_1, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[1]);
}

static void test_profile_encodes_delta_values(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 1, 0, 0, 1);
    TEST_ASSERT_NOT_NULL(profile);
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    const uint32_t values[] = {1000, 1041, 1082};
    for (uint8_t index = 0; index < 3; index++) {
        now.time.minute = index;
        fake_clock_now = (nd_dlms_datetime_t){
            .year = 2026, .month = 11, .day = 11, .minute = index};
        fake_measurement_base = values[index];
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(profile, &port, 16, &now));
    }
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[17]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[22]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[23]);
    TEST_ASSERT_EQUAL_UINT8(0x1F, buffer[37]);
    TEST_ASSERT_EQUAL_UINT8(41, buffer[38]);
    TEST_ASSERT_EQUAL_UINT8(0x1F, buffer[54]);
    TEST_ASSERT_EQUAL_UINT8(41, buffer[55]);
}


static void test_profile_delta_falls_back_on_decrease(void)
{
    static uint8_t memory[4096];
    nd_byte_store_t store;
    nd_clock_port_t clock_port = {.ctx = NULL, .read = fake_clock_read};
    nd_measurement_port_t measurement = {.ctx = NULL,
                                         .read = fake_measurement};
    nd_idis_objects_t objects;
    nd_idis_profile_state_t *profile =
        setup_profile(&objects, &store, memory, sizeof(memory), &clock_port,
                      &measurement, 1, 0, 0, 1);
    TEST_ASSERT_NOT_NULL(profile);
    nd_cosem_port_t port = {
        .ctx = &objects,
        .get = nd_cosem_port_get,
        .set = nd_cosem_port_set,
        .action = nd_cosem_port_action,
    };
    nd_clock_value_t now = {.time = {.year = 2026, .month = 11, .day = 11}};
    const uint32_t values[] = {1000, 900, 950};
    for (uint8_t index = 0; index < 3; index++) {
        now.time.minute = index;
        fake_clock_now = (nd_dlms_datetime_t){
            .year = 2026, .month = 11, .day = 11, .minute = index};
        fake_measurement_base = values[index];
        TEST_ASSERT_EQUAL_INT(ND_OK,
                              nd_idis_profile_capture(profile, &port, 16, &now));
    }
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16, ND_IC_PROFILE_GENERIC,
                                            &ND_OBIS_BILLING_PROFILE, 2, NULL,
                                            &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[37]); /* decrease: full value */
    TEST_ASSERT_EQUAL_UINT8(0x1F, buffer[57]); /* increase: delta 50 */
    TEST_ASSERT_EQUAL_UINT8(50, buffer[58]);
}


static void test_capture_adapter_preserves_signed_register(void)
{
    const uint8_t raw[] = {ND_AXDR_DOUBLE_LONG, 0xFF, 0xFF, 0xFF, 0xFE};
    nd_axdr_reader_t reader;
    nd_axdr_reader_init(&reader, raw, sizeof(raw));
    nd_capture_value_t value;
    const nd_capture_object_t capture = {ND_IC_REGISTER,
                                         ND_OBIS_ACTIVE_ENERGY_NET, 2, 0};
    TEST_ASSERT_EQUAL_INT(0, nd_idis_capture_read(&capture, &reader, &value));
    TEST_ASSERT_EQUAL_UINT8(ND_CAPTURE_SIGNED, value.type);
    TEST_ASSERT_EQUAL_INT64(-2, value.value.signed_value);
    uint8_t out_buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, out_buffer, sizeof(out_buffer));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_capture_write(&capture, &value, &out));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_DOUBLE_LONG, out_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, out_buffer[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_profile_action_capture_and_get);
    RUN_TEST(test_profile_on_second_uses_capture_period);
    RUN_TEST(test_multiple_profiles_are_independent);
    RUN_TEST(test_profile_selective_entry);
    RUN_TEST(test_profile_selective_clock_range);
    RUN_TEST(test_profile_compresses_repeated_values);
    RUN_TEST(test_profile_compresses_status);
    RUN_TEST(test_profile_encodes_delta_values);
    RUN_TEST(test_profile_delta_falls_back_on_decrease);
    RUN_TEST(test_axdr_skip_value_handles_nested_values);
    RUN_TEST(test_profile_after_capture_callback);
    RUN_TEST(test_capture_adapter_preserves_signed_register);
    return UNITY_END();
}
