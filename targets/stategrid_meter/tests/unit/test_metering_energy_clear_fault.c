/* A clear or total set that never reaches the store must report failure. The
 * pre-clear snapshot stays the boot source, so "success" would be a lie. */
#include "unity.h"

#include "metering_energy_harness.h"

void setUp(void) { harness_init(); }
void tearDown(void) {}

static void test_clear_and_set_total_fail_when_the_commit_fails(void)
{
    legacy_energy_record_t record;
    nd_energy_ledger_t *ledger;

    harness_seed_snapshot(5u);
    harness_reboot();
    ledger = harness_bind();
    TEST_ASSERT_EQUAL_INT64(5, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));

    harness.fault.fail_writes = 1;
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_clear(ledger));
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_set_total(
                                  ledger, ND_ENERGY_FORWARD, 7u));

    harness.fault.fail_writes = 0;
    TEST_ASSERT_EQUAL_INT(0, harness_read_base(&record));
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clear_and_set_total_fail_when_the_commit_fails);
    return UNITY_END();
}
