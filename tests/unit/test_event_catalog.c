#include "unity.h"

#include "domain/events/event_catalog.h"
#include "domain/events/event_id.h"

void setUp(void) {}
void tearDown(void) {}

static void assert_entry(uint16_t record_id, uint32_t oad, uint8_t mode)
{
    nd_event_catalog_entry_t entry;
    TEST_ASSERT_EQUAL_INT(1, nd_event_catalog_lookup(record_id, &entry));
    TEST_ASSERT_EQUAL_HEX32(oad, entry.oad);
    TEST_ASSERT_EQUAL_UINT8(mode, entry.mode);
}

static void test_catalog_event_pairs(void)
{
    assert_entry(OverCurrentAS, 0x30050700u, 10);
    assert_entry(OverCurrentAE, 0x30050700u, 11);
    assert_entry(PowerOffE, 0x30110200u, 11);
    assert_entry(CoverOpenE, 0x301B0200u, 11);
    assert_entry(MeterWr645R, 0xFF310000u, 11);
    assert_entry(SwitchErrE, 0x302B0200u, 11);
    assert_entry(PowerAbnormalE, 0x302C0200u, 11);
    assert_entry(RTCErrE, 0x302E0200u, 11);
    assert_entry(EmuErrE, 0x302F0200u, 11);
    assert_entry(ZeroLineErrE, 0x30400200u, 11);
}

static void test_catalog_single_events(void)
{
    assert_entry(MeterClrS, 0x30130200u, 10);
    assert_entry(EventClrS, 0x30150200u, 10);
    assert_entry(SetTimeS, 0x30160200u, 10);
    assert_entry(BroadcastTimeS, 0x303C0200u, 10);
}

static void test_catalog_table_groups(void)
{
    assert_entry(DayTabWrS, 0x30170200u, 10);
    assert_entry(BillDayWrS, 0x301A0200u, 10);
    assert_entry(SwitchOffS, 0x301F0200u, 10);
    assert_entry(ActCombWrS, 0x30220200u, 10);
    assert_entry(FeeTabWrS, 0x30240200u, 10);
    assert_entry(RefundS, 0x30290200u, 10);
}

static void test_catalog_freeze_groups(void)
{
    assert_entry(MinuteFreeze1, 0x50020200u, 0);
    assert_entry(MinuteFreeze8, 0x5002E200u, 7);
    assert_entry(HourFreeze, 0x50030200u, 0);
    assert_entry(MonthFreeze, 0x50060200u, 0);
    assert_entry(SeasonTabCutFreeze, 0x50080200u, 0);
    assert_entry(StepCutFreeze, 0x500B0200u, 0);
    assert_entry(InstantFreeze, 0x50000200u, 0);
    assert_entry(StepBillFreeze, 0x50110200u, 0);
}

static void test_catalog_rejects_unmapped_ids(void)
{
    nd_event_catalog_entry_t entry;

    TEST_ASSERT_EQUAL_INT(0, nd_event_catalog_lookup(YearFreeze, &entry));
    TEST_ASSERT_EQUAL_INT(0, nd_event_catalog_lookup(RECORD_ID_END, &entry));
    TEST_ASSERT_EQUAL_INT(0, nd_event_catalog_lookup(1, &entry));
    TEST_ASSERT_EQUAL_INT(0, nd_event_catalog_lookup(OverCurrentAS, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_catalog_event_pairs);
    RUN_TEST(test_catalog_single_events);
    RUN_TEST(test_catalog_table_groups);
    RUN_TEST(test_catalog_freeze_groups);
    RUN_TEST(test_catalog_rejects_unmapped_ids);
    return UNITY_END();
}
