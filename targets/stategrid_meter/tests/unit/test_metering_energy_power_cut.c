/* Pulses counted while the restore is pending live only in RAM: the snapshot
 * store has no per-pulse append. A power cut during that window loses them and
 * the next boot restores the recovered book (durable base plus replay), never
 * the zero book and never a second copy of the window. */
#include "unity.h"

#include "metering_energy_harness.h"

void setUp(void) { harness_init(); }
void tearDown(void) {}

static void test_power_cut_during_the_restore_window_keeps_the_durable_book(void)
{
    nd_energy_ledger_t *ledger;

    harness_seed_snapshot(5u);
    harness_reboot();
    ledger = harness_bind();

    harness.fault.fail_reads = 1;
    (void)metering_energy_add(ledger, ND_ENERGY_FORWARD,
                              40u * HARNESS_PULSES_PER_STEP);

    /* Power cut: the window's pulses were never appended durably. */
    harness.fault.fail_reads = 0;
    harness_reboot();
    ledger = harness_bind();

    TEST_ASSERT_EQUAL_INT64(5, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                   1, ND_ENERGY_RES_0_01_KWH));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_cut_during_the_restore_window_keeps_the_durable_book);
    return UNITY_END();
}
