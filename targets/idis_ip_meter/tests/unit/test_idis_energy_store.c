#include "unity.h"

#include "infra/memory/byte_store.h"
#include "targets/idis_ip_meter/cosem/nd_idis_energy_store.h"

void setUp(void) {}
void tearDown(void) {}

static void test_energy_set_round_trip(void)
{
    static uint8_t memory[2048];
    nd_byte_store_t store;
    nd_byte_store_init(&store, memory, sizeof(memory));
    nd_energy_set_t set;
    nd_energy_set_init(&set, 2000);
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &set, ND_ENERGY_CHANNEL_ACTIVE_IMPORT, 1, 4000));
    TEST_ASSERT_EQUAL_INT(0, nd_energy_set_add_pulses(
                                 &set, ND_ENERGY_CHANNEL_REACTIVE_Q2, 2, 2000));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_save(
                                 &set, nd_byte_store_port(&store), 0));
    nd_energy_set_t restored;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_energy_load(
                                 &restored, nd_byte_store_port(&store), 0));
    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_set_read_raw(
                                       &restored, ND_ENERGY_CHANNEL_ACTIVE_IMPORT,
                                       1));
    TEST_ASSERT_EQUAL_INT64(10000, nd_energy_set_read_raw(
                                       &restored, ND_ENERGY_CHANNEL_REACTIVE_Q2,
                                       2));
    TEST_ASSERT_EQUAL_UINT32(2000, restored.meter_const);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_energy_set_round_trip);
    return UNITY_END();
}
