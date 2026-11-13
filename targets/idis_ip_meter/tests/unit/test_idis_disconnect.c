#include "unity.h"
#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

static nd_idis_objects_t objects;

static int fake_clock(void *ctx, nd_dlms_datetime_t *now)
{
    (void)ctx;
    *now = (nd_dlms_datetime_t){.year = 2026, .month = 11, .day = 11};
    return 0;
}

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    objects.clock.port = &(nd_clock_port_t){.read = fake_clock};
    nd_disconnect_init(&objects.disconnect, 2, NULL);
}

void tearDown(void) {}

static void test_disconnect_attributes(void)
{
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DISCONNECT_CONTROL,
                                            &ND_OBIS_DISCONNECT_CONTROL, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DISCONNECT_CONTROL,
                                            &ND_OBIS_DISCONNECT_CONTROL, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x16, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_DISCONNECT_CONTROL,
                                            &ND_OBIS_DISCONNECT_CONTROL, 4,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
}

static void test_remote_disconnect_and_reconnect(void)
{
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DISCONNECT_CONTROL,
                                               &ND_OBIS_DISCONNECT_CONTROL, 1,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_DISCONNECTED,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_UINT8(0, objects.disconnect.output_state);
    TEST_ASSERT_EQUAL_UINT8(62, objects.event.latest_event_code);
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_DISCONNECT_CONTROL,
                                               &ND_OBIS_DISCONNECT_CONTROL, 2,
                                               NULL, NULL));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_CONNECTED,
                            objects.disconnect.control_state);
    TEST_ASSERT_EQUAL_UINT8(1, objects.disconnect.output_state);
    TEST_ASSERT_EQUAL_UINT8(63, objects.event.latest_event_code);
}


static void test_disconnect_state_persists(void)
{
    static uint8_t memory[256];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_datetime_t now = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(0, nd_disconnect_request(
                                 &objects.disconnect,
                                 ND_DISCONNECT_REMOTE_DISCONNECT, &now));
    TEST_ASSERT_EQUAL_INT(0, nd_ic_disconnect_save(
                                 &objects.disconnect, nd_byte_store_port(&store),
                                 0));
    nd_disconnect_snapshot_t restored = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_ic_disconnect_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT8(ND_DISCONNECT_DISCONNECTED,
                            restored.control_state);
    TEST_ASSERT_EQUAL_UINT8(0, restored.output_state);
    TEST_ASSERT_EQUAL_UINT8(2, restored.control_mode);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_disconnect_attributes);
    RUN_TEST(test_remote_disconnect_and_reconnect);
    RUN_TEST(test_disconnect_state_persists);
    return UNITY_END();
}
