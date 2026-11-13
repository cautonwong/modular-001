#include "unity.h"

#include <string.h>

#include "application/adapters/dl698/report_encoder.h"

void setUp(void) {}
void tearDown(void) {}

static void test_3011_start_frame(void)
{
    uint8_t buffer[64];
    nd_report_encoder_t encoder;
    const uint32_t oads[2] = {0x30110200u, 0x30110201u};
    const uint8_t extra[3] = {0x01u, 0xAAu, 0xBBu};

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_begin_3011(&encoder, 0x05, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_3011_rcsd(&encoder, oads, 2));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_append(&encoder, extra,
                                                      sizeof(extra)));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_finish_3011(&encoder));

    TEST_ASSERT_EQUAL_UINT8(0x88, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x30, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[9]);
    TEST_ASSERT_EQUAL_UINT8(0x30, buffer[10]);
    TEST_ASSERT_EQUAL_UINT8(0x30, buffer[15]);
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[16]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[17]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[18]);
    TEST_ASSERT_EQUAL_INT(22, encoder.offset);
}

static void test_3011_restore_frame_writes_length_prefix(void)
{
    uint8_t buffer[64];
    nd_report_encoder_t encoder;
    const uint32_t oads[1] = {0x30110200u};
    const uint8_t extra[2] = {0x00u, 0x00u};

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_begin_3011(&encoder, 0x07, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_3011_rcsd(&encoder, oads, 1));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_append(&encoder, extra, 2));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_finish_3011(&encoder));

    TEST_ASSERT_EQUAL_UINT8(0x10, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(encoder.offset - 3), buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x88, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x07, buffer[5]);
}

static void test_normal_frame_encodes_items_and_backfills_length(void)
{
    uint8_t buffer[64];
    uint8_t items[10];
    nd_report_encoder_t encoder;
    uint8_t i;

    for (i = 0; i < sizeof(items); i++) {
        items[i] = (uint8_t)(0x10u + i);
    }

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_begin_normal(&encoder, 0x12, 2));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_3320(&encoder, items, 2));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_2015(&encoder, 0x11223344u));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_end_normal(&encoder));

    TEST_ASSERT_EQUAL_UINT8(0x10, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(encoder.offset - 3), buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x88, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x12, buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x33, buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[13]);
    TEST_ASSERT_EQUAL_UINT8(0x10, buffer[14]);
    TEST_ASSERT_EQUAL_UINT8(0x20, buffer[24]);
    TEST_ASSERT_EQUAL_UINT8(0x15, buffer[25]);
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[31]);
    TEST_ASSERT_EQUAL_UINT8(0x44, buffer[34]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[35]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[36]);
}

static void test_encoder_rejects_overflow(void)
{
    uint8_t buffer[4];
    nd_report_encoder_t encoder;
    const uint32_t oad = 0x30110200u;

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_encoder_begin_3011(&encoder, 1, 0));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_encoder_3011_rcsd(&encoder, &oad, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_encoder_init(&encoder, NULL, 4));
}

static void test_rn_mac_tail_uses_esam_bytes_and_random(void)
{
    uint8_t buffer[32];
    uint8_t random12[12];
    uint8_t esam_tail[17];
    nd_report_encoder_t encoder;
    uint8_t i;

    for (i = 0; i < sizeof(random12); i++) {
        random12[i] = (uint8_t)(0x20u + i);
    }
    for (i = 0; i < sizeof(esam_tail); i++) {
        esam_tail[i] = (uint8_t)(0x40u + i);
    }

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_rn_mac_tail(
        &encoder, random12, esam_tail, 0));
    TEST_ASSERT_EQUAL_INT(19, encoder.offset);
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x0C, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x20, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x2B, buffer[13]);
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[14]);
    TEST_ASSERT_EQUAL_UINT8(0x4D, buffer[15]);
    TEST_ASSERT_EQUAL_UINT8(0x50, buffer[18]);
}

static void test_rn_mac_tail_fills_ff_when_esam_is_abnormal(void)
{
    uint8_t buffer[32];
    uint8_t random12[12] = {0};
    nd_report_encoder_t encoder;
    uint8_t i;

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_rn_mac_tail(
        &encoder, random12, NULL, 1));
    for (i = 15; i < 19; i++) {
        TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[i]);
    }
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[14]);
}

static void test_rn_mac_tail_rejects_missing_inputs(void)
{
    uint8_t buffer[4];
    uint8_t random12[12] = {0};
    nd_report_encoder_t encoder;

    TEST_ASSERT_EQUAL_INT(0, nd_report_encoder_init(&encoder, buffer,
                                                     sizeof(buffer)));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_encoder_rn_mac_tail(
        &encoder, random12, NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_encoder_rn_mac_tail(
        &encoder, NULL, NULL, 1));
    TEST_ASSERT_EQUAL_INT(-1, nd_report_encoder_rn_mac_tail(
        &encoder, random12, NULL, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_3011_start_frame);
    RUN_TEST(test_3011_restore_frame_writes_length_prefix);
    RUN_TEST(test_normal_frame_encodes_items_and_backfills_length);
    RUN_TEST(test_encoder_rejects_overflow);
    RUN_TEST(test_rn_mac_tail_uses_esam_bytes_and_random);
    RUN_TEST(test_rn_mac_tail_fills_ff_when_esam_is_abnormal);
    RUN_TEST(test_rn_mac_tail_rejects_missing_inputs);
    return UNITY_END();
}
