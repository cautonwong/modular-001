/* The book may not be committed before its base was loaded. A transient
 * EEPROM read failure on the first restore must be retried, must not turn the
 * zero book into a newer generation, and must not drop the pulses counted
 * while the restore is pending: they are applied once, on top of the
 * recovered book. */
#include "unity.h"

#include "metering_energy_harness.h"

void setUp(void) { harness_init(); }
void tearDown(void) {}

static void test_failed_restore_is_retried_and_pulses_apply_once(void)
{
    legacy_energy_record_t record;
    nd_energy_ledger_t *ledger;

    harness_seed_snapshot(5u);
    harness_reboot();
    ledger = harness_bind();

    harness.fault.fail_reads = 1;
    (void)metering_energy_add(ledger, ND_ENERGY_FORWARD,
                              40u * HARNESS_PULSES_PER_STEP);

    /* The zero book must not reach the legacy view. */
    TEST_ASSERT_EQUAL_UINT32(0u, nd_mbase_active_p(&harness.mbase)[1]);
    TEST_ASSERT_EQUAL_UINT32(0u, nd_mbase_Energy_chksum(&harness.mbase));

    /* A clear while the book is not restored is refused, not exported. */
    TEST_ASSERT_EQUAL_INT(-1, metering_energy_clear(ledger));
    TEST_ASSERT_EQUAL_UINT32(0u, nd_mbase_active_p(&harness.mbase)[1]);
    TEST_ASSERT_EQUAL_UINT32(0u, nd_mbase_Energy_chksum(&harness.mbase));

    harness.fault.fail_reads = 0;
    TEST_ASSERT_EQUAL_INT(0, harness_read_base(&record));
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);

    TEST_ASSERT_EQUAL_INT64(45, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_INT(0, harness_read_base(&record));
    TEST_ASSERT_EQUAL_UINT32(45u, record.active_p[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_failed_restore_is_retried_and_pulses_apply_once);
    return UNITY_END();
}
