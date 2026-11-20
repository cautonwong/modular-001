#include "unity.h"

#include <string.h>

#include "components/services/maintenance/nd_maintenance.h"

static const uint8_t release_frame[] = {
    0x68, 0x05, 0x01, 0x52, 0x45, 0x4C, 0x47, 0x18,
};

static int release_calls;
static uint32_t release_confirmation;
static int release_result;

static int fake_release(void *ctx, uint32_t confirmation)
{
    (void)ctx;
    release_calls++;
    release_confirmation = confirmation;
    return release_result;
}

static int feed(const uint8_t *bytes, size_t length)
{
    nd_maintenance_t maintenance;
    nd_maintenance_init(&maintenance);
    int result = 0;
    for (size_t index = 0; index < length; index++) {
        result = nd_maintenance_feed(&maintenance, bytes[index],
                                     fake_release, NULL);
    }
    return result;
}

void setUp(void)
{
    release_calls = 0;
    release_confirmation = 0;
    release_result = 0;
}

void tearDown(void) {}

static void test_release_frame_calls_the_consumer(void)
{
    TEST_ASSERT_EQUAL_INT(1, feed(release_frame, sizeof(release_frame)));
    TEST_ASSERT_EQUAL_INT(1, release_calls);
    TEST_ASSERT_EQUAL_UINT32(0x52454C47u, release_confirmation);
}

static void test_bad_checksum_is_rejected(void)
{
    uint8_t frame[sizeof(release_frame)];
    memcpy(frame, release_frame, sizeof(frame));
    frame[sizeof(frame) - 1] ^= 0xFFu;
    TEST_ASSERT_EQUAL_INT(-1, feed(frame, sizeof(frame)));
    TEST_ASSERT_EQUAL_INT(0, release_calls);
}

static void test_partial_frame_waits(void)
{
    TEST_ASSERT_EQUAL_INT(0, feed(release_frame, sizeof(release_frame) - 1));
    TEST_ASSERT_EQUAL_INT(0, release_calls);
}

/* The consumer aborts after every drain, so a prefix from one scan cycle
 * cannot be completed by unrelated bytes that arrive in the next one. */
static void test_partial_frame_does_not_survive_a_scan_cycle(void)
{
    nd_maintenance_t maintenance;
    nd_maintenance_init(&maintenance);
    for (size_t index = 0; index < 5; index++) {
        TEST_ASSERT_EQUAL_INT(
            0, nd_maintenance_feed(&maintenance, release_frame[index],
                                   fake_release, NULL));
    }
    nd_maintenance_abort(&maintenance);
    for (size_t index = 5; index < sizeof(release_frame); index++) {
        (void)nd_maintenance_feed(&maintenance, release_frame[index],
                                  fake_release, NULL);
    }
    TEST_ASSERT_EQUAL_INT(0, release_calls);
}

static void test_abort_lets_a_new_frame_start(void)
{
    nd_maintenance_t maintenance;
    nd_maintenance_init(&maintenance);
    for (size_t index = 0; index < 5; index++) {
        (void)nd_maintenance_feed(&maintenance, release_frame[index],
                                  fake_release, NULL);
    }
    nd_maintenance_abort(&maintenance);
    for (size_t index = 0; index < sizeof(release_frame); index++) {
        (void)nd_maintenance_feed(&maintenance, release_frame[index],
                                  fake_release, NULL);
    }
    TEST_ASSERT_EQUAL_INT(1, release_calls);
}

static void test_noise_then_frame_resynchronises(void)
{
    uint8_t bytes[3 + sizeof(release_frame)] = {0x00, 0x68, 0x00};
    memcpy(bytes + 3, release_frame, sizeof(release_frame));
    TEST_ASSERT_EQUAL_INT(1, feed(bytes, sizeof(bytes)));
    TEST_ASSERT_EQUAL_INT(1, release_calls);
}

static void test_consumer_failure_is_reported(void)
{
    release_result = -1;
    TEST_ASSERT_EQUAL_INT(-1, feed(release_frame, sizeof(release_frame)));
    TEST_ASSERT_EQUAL_INT(1, release_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_release_frame_calls_the_consumer);
    RUN_TEST(test_bad_checksum_is_rejected);
    RUN_TEST(test_partial_frame_waits);
    RUN_TEST(test_partial_frame_does_not_survive_a_scan_cycle);
    RUN_TEST(test_abort_lets_a_new_frame_start);
    RUN_TEST(test_noise_then_frame_resynchronises);
    RUN_TEST(test_consumer_failure_is_reported);
    return UNITY_END();
}
