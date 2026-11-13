#include "unity.h"

#include "domain/load_control/disconnect.h"

static nd_disconnect_snapshot_t snapshot;

void setUp(void)
{
    nd_disconnect_init(&snapshot, 2, &(nd_datetime_t){0});
}

void tearDown(void) {}

static void test_remote_disconnect_and_direct_reconnect(void)
{
    nd_datetime_t now = {1, 2, 3, 4, 5, 2026};
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &snapshot, ND_DISCONNECT_REMOTE_DISCONNECT, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_DISCONNECTED, snapshot.control_state);
    TEST_ASSERT_EQUAL_UINT8(0, snapshot.output_state);
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &snapshot, ND_DISCONNECT_REMOTE_RECONNECT, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED, snapshot.control_state);
    TEST_ASSERT_EQUAL_UINT8(1, snapshot.output_state);
}

static void test_ready_state_and_manual_reconnect(void)
{
    nd_datetime_t now = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &snapshot, ND_DISCONNECT_MANUAL_DISCONNECT, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            snapshot.control_state);
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &snapshot, ND_DISCONNECT_MANUAL_RECONNECT, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED, snapshot.control_state);
}

static void test_mode_restrictions(void)
{
    nd_datetime_t now = {0};
    nd_disconnect_init(&snapshot, 3, &now);
    TEST_ASSERT_EQUAL_INT(-1, nd_disconnect_request(
                                  &snapshot, ND_DISCONNECT_LOCAL_DISCONNECT, &now));
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &snapshot, ND_DISCONNECT_REMOTE_DISCONNECT, &now));
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &snapshot, ND_DISCONNECT_REMOTE_RECONNECT, &now));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_READY_FOR_RECONNECTION,
                            snapshot.control_state);
}

static void test_output_feedback_and_invalid(void)
{
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_set_output(&snapshot, 0));
    TEST_ASSERT_EQUAL_UINT8(0, snapshot.output_state);
    TEST_ASSERT_EQUAL_INT(-1, nd_disconnect_request(NULL, ND_DISCONNECT_REMOTE_DISCONNECT,
                                                    &(nd_datetime_t){0}));
    TEST_ASSERT_EQUAL_INT(-1, nd_disconnect_set_output(NULL, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_remote_disconnect_and_direct_reconnect);
    RUN_TEST(test_ready_state_and_manual_reconnect);
    RUN_TEST(test_mode_restrictions);
    RUN_TEST(test_output_feedback_and_invalid);
    return UNITY_END();
}
