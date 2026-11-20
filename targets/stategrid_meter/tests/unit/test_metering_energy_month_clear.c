/* Monthly clear must clear the book, not only the legacy RAM view: the book
 * is exported into the view on every pulse and is the commit source. */
#include "unity.h"

#include "metering_energy_harness.h"

void setUp(void) { harness_init(); }
void tearDown(void) {}

static void test_month_clear_survives_the_next_pulse_and_reboot(void)
{
    legacy_energy_record_t record;
    nd_energy_ledger_t *ledger;

    harness_seed_snapshot(5u);
    harness_reboot();
    ledger = harness_bind();
    TEST_ASSERT_EQUAL_INT64(5, metering_energy_read(ledger, ND_ENERGY_FORWARD,
                                                    1, ND_ENERGY_RES_0_01_KWH));
    TEST_ASSERT_EQUAL_UINT32(5u, nd_mbase_MonthEnegery(&harness.mbase));

    TEST_ASSERT_EQUAL_INT(0, metering_energy_clear_month(ledger));
    TEST_ASSERT_EQUAL_UINT32(0u, nd_mbase_MonthEnegery(&harness.mbase));

    /* The next second exports the book again; only the new pulse may count. */
    (void)metering_energy_add(ledger, ND_ENERGY_FORWARD,
                              HARNESS_PULSES_PER_STEP);
    TEST_ASSERT_EQUAL_UINT32(1u, nd_mbase_MonthEnegery(&harness.mbase));

    /* The pulse since the clear is below the save step; the committed month
     * is the cleared value the next boot restores. */
    TEST_ASSERT_EQUAL_INT(0, harness_read_base(&record));
    TEST_ASSERT_EQUAL_UINT32(0u, record.month_energy);
    TEST_ASSERT_EQUAL_UINT32(5u, record.active_p[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_month_clear_survives_the_next_pulse_and_reboot);
    return UNITY_END();
}
