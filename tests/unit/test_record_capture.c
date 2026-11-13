#include "unity.h"

#include "domain/events/record_capture.h"

void setUp(void) {}
void tearDown(void) {}

static void test_capture_set_time_maps_start_and_end(void)
{
    TEST_ASSERT_EQUAL_UINT32(
        0x40002200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_SET_TIME, 0x201E0200u, 0));
    TEST_ASSERT_EQUAL_UINT32(
        0x40008200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_SET_TIME, 0x20200200u, 0));
}

static void test_capture_program_maps_first_time_set_only(void)
{
    TEST_ASSERT_EQUAL_UINT32(
        0x40002200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_PROGRAM, 0x201E0200u, 1));
    TEST_ASSERT_EQUAL_UINT32(
        0x201E0200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_PROGRAM, 0x201E0200u, 0));
}

static void test_capture_default_maps_back_to_event_times(void)
{
    TEST_ASSERT_EQUAL_UINT32(
        0x201E0200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_DEFAULT, 0x40002200u, 0));
    TEST_ASSERT_EQUAL_UINT32(
        0x20200200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_DEFAULT, 0x40008200u, 0));
}

static void test_capture_normal_energy_oad_becomes_high_precision(void)
{
    TEST_ASSERT_EQUAL_UINT32(
        0x00100400u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_DEFAULT, 0x00100200u, 0));
    TEST_ASSERT_EQUAL_UINT32(
        0x201E0200u,
        nd_record_capture_oad(ND_RECORD_CAPTURE_DEFAULT, 0x40002200u, 0));
}

static void test_capture_visibility_end_only_oads(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x33110206u, ND_RECORD_KIND_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(1, nd_record_capture_visible(
        ND_RECORD_CAPTURE_START_MODE, 0x33110206u, ND_RECORD_KIND_DEFAULT));
}

static void test_capture_visibility_event_times(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x201E0200u, ND_RECORD_KIND_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_START_MODE, 0x20200200u, ND_RECORD_KIND_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x20200200u, ND_RECORD_KIND_METER_WR));
    TEST_ASSERT_EQUAL_UINT8(1, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x20200200u, ND_RECORD_KIND_DEFAULT));
}

static void test_capture_visibility_scheme_bands(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, nd_record_capture_visible(
        ND_RECORD_CAPTURE_START_MODE, 0x00002200u, ND_RECORD_KIND_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x00002200u, ND_RECORD_KIND_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_START_MODE, 0x00006200u, ND_RECORD_KIND_DEFAULT));
    TEST_ASSERT_EQUAL_UINT8(1, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x00006200u, ND_RECORD_KIND_DEFAULT));
}

static void test_capture_visibility_record_kinds_override(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x00006200u,
        ND_RECORD_KIND_METER_WR_R));
    TEST_ASSERT_EQUAL_UINT8(1, nd_record_capture_visible(
        ND_RECORD_CAPTURE_END_MODE, 0x20200200u,
        ND_RECORD_KIND_SET_TIME));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_capture_set_time_maps_start_and_end);
    RUN_TEST(test_capture_program_maps_first_time_set_only);
    RUN_TEST(test_capture_default_maps_back_to_event_times);
    RUN_TEST(test_capture_normal_energy_oad_becomes_high_precision);
    RUN_TEST(test_capture_visibility_end_only_oads);
    RUN_TEST(test_capture_visibility_event_times);
    RUN_TEST(test_capture_visibility_scheme_bands);
    RUN_TEST(test_capture_visibility_record_kinds_override);
    return UNITY_END();
}
