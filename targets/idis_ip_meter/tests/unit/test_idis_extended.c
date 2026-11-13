#include "unity.h"
#include "infra/memory/byte_store.h"

#include "targets/idis_ip_meter/cosem/nd_cosem_classes.h"
#include "targets/idis_ip_meter/cosem/nd_cosem_port.h"
#include "targets/idis_ip_meter/cosem/nd_idis_objects.h"

static nd_idis_objects_t objects;

void setUp(void)
{
    objects = (nd_idis_objects_t){0};
    objects.max_demand[0] = (nd_idis_extended_state_t){
        .value = 1234,
        .status = 4,
        .capture_time = {.year = 2026, .month = 11, .day = 11},
    };
    objects.max_demand[19] = (nd_idis_extended_state_t){.value = 20};
}

void tearDown(void) {}

static void test_max_demand_attributes(void)
{
    const nd_obis_t first = {1, 0, 1, 6, 0, 255};
    uint8_t buffer[16];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &first, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x06, buffer[0]);
    TEST_ASSERT_EQUAL_UINT32(1234, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &first, 3,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(27, buffer[5]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &first, 4,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(4, buffer[1]);
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &first, 5,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(12, buffer[1]);
}

static void test_last_max_demand_index(void)
{
    const nd_obis_t last = {1, 0, 4, 6, 4, 255};
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &last, 2,
                                            NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(20, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
}


static void test_group_and_rate_select_the_register(void)
{
    const nd_obis_t group2_rate1 = {1, 0, 2, 6, 1, 255};
    objects.max_demand[(2 - 1) * 5 + 1] =
        (nd_idis_extended_state_t){.value = 77};
    uint8_t buffer[8];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER,
                                            &group2_rate1, 2, NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(77, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);

    /* Apparent energy groups 9/10 map to indices 20..29. */
    const nd_obis_t apparent = {1, 0, 9, 6, 2, 255};
    objects.max_demand[22] = (nd_idis_extended_state_t){.value = 88};
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_get(&objects, 16,
                                            ND_IC_EXTENDED_REGISTER, &apparent,
                                            2, NULL, &out));
    TEST_ASSERT_EQUAL_UINT32(88, ((uint32_t)buffer[1] << 24)
                                     | ((uint32_t)buffer[2] << 16)
                                     | ((uint32_t)buffer[3] << 8) | buffer[4]);
}

static void test_max_demand_update_keeps_maximum(void)
{
    nd_idis_extended_state_t states[ND_IDIS_MAX_DEMAND] = {0};
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_extended_update(states, 1, 0, 100, 0,
                                                     &time));
    TEST_ASSERT_EQUAL_UINT32(100, states[0].value);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_extended_update(states, 1, 0, 90, 0,
                                                     &time));
    TEST_ASSERT_EQUAL_UINT32(100, states[0].value);
    time.day = 12;
    TEST_ASSERT_EQUAL_INT(1, nd_idis_extended_update(states, 1, 0, 120, 4,
                                                     &time));
    TEST_ASSERT_EQUAL_UINT32(120, states[0].value);
    TEST_ASSERT_EQUAL_UINT8(4, states[0].status);
    TEST_ASSERT_EQUAL_UINT8(12, states[0].capture_time.day);
    TEST_ASSERT_EQUAL_INT(1, nd_idis_extended_update(states, 4, 4, 7, 0,
                                                     &time));
    TEST_ASSERT_EQUAL_UINT32(7, states[19].value);
}

static void test_max_demand_persists(void)
{
    static uint8_t memory[1024];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_idis_extended_state_t states[ND_IDIS_MAX_DEMAND] = {0};
    nd_datetime_t time = {.year = 2026, .month = 11, .day = 11};
    TEST_ASSERT_EQUAL_INT(1, nd_idis_extended_update(states, 2, 3, 99, 1,
                                                     &time));
    TEST_ASSERT_EQUAL_INT(0, nd_ic_extended_save(states,
                                                  nd_byte_store_port(&store), 0));
    nd_idis_extended_state_t restored[ND_IDIS_MAX_DEMAND] = {0};
    TEST_ASSERT_EQUAL_INT(0, nd_ic_extended_load(restored,
                                                  nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_UINT32(99, restored[8].value);
    TEST_ASSERT_EQUAL_UINT8(1, restored[8].status);
}

static void test_max_demand_reset(void)
{
    const nd_obis_t first = {1, 0, 1, 6, 0, 255};
    uint8_t parameter[2] = {ND_AXDR_INTEGER, 0};
    nd_axdr_reader_t in;
    nd_axdr_reader_init(&in, parameter, sizeof(parameter));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_cosem_port_action(&objects, 16,
                                               ND_IC_EXTENDED_REGISTER, &first,
                                               1, &in, NULL));
    TEST_ASSERT_EQUAL_UINT32(0, objects.max_demand[0].value);
    TEST_ASSERT_EQUAL_UINT8(0, objects.max_demand[0].status);
    /* resetting one rate leaves the other objects alone */
    TEST_ASSERT_EQUAL_UINT32(20, objects.max_demand[19].value);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max_demand_attributes);
    RUN_TEST(test_last_max_demand_index);
    RUN_TEST(test_group_and_rate_select_the_register);
    RUN_TEST(test_max_demand_update_keeps_maximum);
    RUN_TEST(test_max_demand_persists);
    RUN_TEST(test_max_demand_reset);
    return UNITY_END();
}
