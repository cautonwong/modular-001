#include "unity.h"

#include "targets/idis_ip_meter/cosem/classes/nd_ic_register.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;
static nd_clock_port_t clock_port;

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 1, .day = 1};
    return 0;
}

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    clock_port = (nd_clock_port_t){.ctx = NULL, .read = fake_clock};
    nd_idis_clock_init(&objects.clock, &clock_port);
    nd_pq_config_t config = {
        .voltage_sag = 2000,
        .voltage_swell = 2500,
        .frequency_min = 4900,
        .frequency_max = 5100,
        .unbalance_permille = 50,
        .debounce_seconds = 3,
        .phase_count = 3,
    };
    nd_idis_pq_init(&objects.pq, &config);
    objects.reg.clock = &objects.clock;
    objects.reg.pq = &objects.pq;
}

void tearDown(void) {}

static int get_register(const nd_obis_t *ln, uint8_t attribute_id,
                        uint8_t *buffer, size_t size)
{
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, size);
    return nd_cosem_port_get(&objects, 16, ND_IC_REGISTER, ln, attribute_id,
                             NULL, &out);
}

static int set_register_unsigned(const nd_obis_t *ln, uint16_t value)
{
    uint8_t buffer[4];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    nd_axdr_put_tag(&out, ND_AXDR_LONG_UNSIGNED);
    nd_axdr_put_u16(&out, value);
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, buffer, out.len);
    return nd_cosem_port_set(&objects, 16, ND_IC_REGISTER, ln, 2, &in);
}

static void test_pq_thresholds_are_objects(void)
{
    uint8_t buffer[16];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SAG_THRESHOLD, 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG_UNSIGNED, buffer[0]);
    TEST_ASSERT_EQUAL_UINT16(2000, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SAG_THRESHOLD, 3, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_INTEGER, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[3]); /* scaler -1 */
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ENUM, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(35, buffer[5]); /* volt */

    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SWELL_THRESHOLD, 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT16(2500, (uint16_t)((buffer[1] << 8) | buffer[2]));
}

static void test_pq_thresholds_are_writable(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          set_register_unsigned(&ND_OBIS_PQ_SAG_THRESHOLD,
                                                2100));
    TEST_ASSERT_EQUAL_INT32(2100, objects.pq.config.voltage_sag);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          set_register_unsigned(&ND_OBIS_PQ_SWELL_THRESHOLD,
                                                2600));
    TEST_ASSERT_EQUAL_INT32(2600, objects.pq.config.voltage_swell);
}

static void test_time_thresholds_are_separate(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          set_register_unsigned(&ND_OBIS_PQ_SWELL_TIME, 7));
    TEST_ASSERT_EQUAL_UINT16(7, objects.pq.config.swell_debounce_seconds);
    TEST_ASSERT_EQUAL_UINT16(0, objects.pq.config.sag_debounce_seconds);
    uint8_t buffer[8];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SAG_TIME, 2, buffer,
                                       sizeof(buffer)));
    /* Unset thresholds fall back to the shared debounce value. */
    TEST_ASSERT_EQUAL_UINT16(3, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SWELL_TIME, 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT16(7, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SWELL_TIME, 3, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(0, buffer[3]); /* scaler 0 */
    TEST_ASSERT_EQUAL_UINT8(7, buffer[5]); /* second */
}

static void test_cut_threshold_object(void)
{
    uint8_t buffer[8];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_CUT_THRESHOLD, 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT16(0, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          set_register_unsigned(&ND_OBIS_PQ_CUT_THRESHOLD,
                                                1500));
    TEST_ASSERT_EQUAL_INT32(1500, objects.pq.config.voltage_cut);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          set_register_unsigned(&ND_OBIS_PQ_CUT_TIME, 4));
    TEST_ASSERT_EQUAL_UINT16(4, objects.pq.config.cut_debounce_seconds);
}

static void test_pq_statistics_objects(void)
{
    objects.pq.sag_phase_count[0] = 3;
    objects.pq.sag_phase_magnitude[0] = 1950;
    objects.pq.sag_phase_duration[0] = 12;
    uint8_t buffer[8];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SAG_COUNT[0], 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_LONG_UNSIGNED, buffer[0]);
    TEST_ASSERT_EQUAL_UINT16(3, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SAG_DURATION[0], 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT16(12, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_PQ_SAG_MAGNITUDE[0], 3, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_STRUCTURE, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[3]); /* scaler -1 */
    TEST_ASSERT_EQUAL_UINT8(35, buffer[5]);   /* volt */
}

static void test_clock_shift_limit_object(void)
{
    uint8_t buffer[8];
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          get_register(&ND_OBIS_CLOCK_SHIFT_LIMIT, 2, buffer,
                                       sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT16(60, (uint16_t)((buffer[1] << 8) | buffer[2]));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          set_register_unsigned(&ND_OBIS_CLOCK_SHIFT_LIMIT,
                                                120));
    TEST_ASSERT_EQUAL_UINT16(120, objects.clock.time_shift_limit);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16, ND_IC_REGISTER,
                                               &ND_OBIS_CLOCK_SHIFT_LIMIT, 1,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT16(60, objects.clock.time_shift_limit);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pq_thresholds_are_objects);
    RUN_TEST(test_pq_thresholds_are_writable);
    RUN_TEST(test_time_thresholds_are_separate);
    RUN_TEST(test_cut_threshold_object);
    RUN_TEST(test_pq_statistics_objects);
    RUN_TEST(test_clock_shift_limit_object);
    return UNITY_END();
}
