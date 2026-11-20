/* Energy_Meter runs before report_2015state_judge in the same second. A
 * successful persist must not clear a storage fault the reporter has not seen
 * yet, or the device error is never reported. */
#include "unity.h"

#include "metering_energy_harness.h"

void setUp(void) { harness_init(); }
void tearDown(void) {}

static void test_storage_fault_stays_latched_after_a_later_success(void)
{
    nd_energy_ledger_t *ledger;

    harness_seed_snapshot(5u);
    harness_reboot();
    ledger = harness_bind();

    harness.fault.fail_writes = 1;
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_clear(ledger));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);

    harness.fault.fail_writes = 0;
    TEST_ASSERT_EQUAL_INT(0, metering_energy_clear(ledger));
    TEST_ASSERT_EQUAL_UINT8(1u, harness.instance.storage_fault);

    /* report_2015state_judge acknowledges after it queues the event. */
    metering_energy_storage_fault_reported(&harness.instance);
    TEST_ASSERT_EQUAL_UINT8(0u, harness.instance.storage_fault);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_storage_fault_stays_latched_after_a_later_success);
    return UNITY_END();
}
