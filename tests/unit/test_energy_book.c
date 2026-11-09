#include "unity.h"

#include "domain/metering/energy.h"

static nd_energy_book_t book;

void setUp(void)
{
    nd_energy_book_init(&book, 2000, ND_CAL_ADD_POS | ND_CAL_ADD_NEG, 200);
}

void tearDown(void) {}

static void test_init_zeroes_book(void)
{
    TEST_ASSERT_EQUAL_UINT32(2000, book.meter_const);
    TEST_ASSERT_EQUAL_UINT32(0, book.forward.slots[0].integer);
    TEST_ASSERT_EQUAL_UINT32(0, book.month.integer);
}

static void test_one_pulse_lands_in_total_and_tariff(void)
{
    nd_energy_update_t update = nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 3, 1);

    TEST_ASSERT_EQUAL_UINT16(1, book.forward.slots[0].fraction);
    TEST_ASSERT_EQUAL_UINT16(1, book.forward.slots[3].fraction);
    TEST_ASSERT_TRUE(update.energy_changed);
    TEST_ASSERT_FALSE(update.energy_save_due);
}

static void test_step_carries_into_hundredths_of_kwh(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 3, 20);

    TEST_ASSERT_EQUAL_UINT32(1, book.forward.slots[3].integer);
    TEST_ASSERT_EQUAL_UINT16(0, book.forward.slots[3].fraction);
    TEST_ASSERT_EQUAL_UINT32(1, book.forward.slots[0].integer);
}

static void test_read_resolutions_match_legacy_formula(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 0, 4000);

    TEST_ASSERT_EQUAL_INT64(20000, nd_energy_read(&book, ND_ENERGY_FORWARD, 0,
                                                  ND_ENERGY_RES_0_0001_KWH));
    TEST_ASSERT_EQUAL_INT64(200, nd_energy_read(&book, ND_ENERGY_FORWARD, 0,
                                                ND_ENERGY_RES_0_01_KWH));
}

static void test_save_due_when_total_crosses_step_boundary(void)
{
    nd_energy_update_t update = nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 0, 4000);

    TEST_ASSERT_TRUE(update.energy_save_due);
}

static void test_month_accumulates_when_cal_word_allows(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 0, 20);
    nd_energy_add_pulses(&book, ND_ENERGY_REVERSE, 0, 20);

    TEST_ASSERT_EQUAL_UINT32(2, book.month.integer);
}

static void test_month_ignores_direction_outside_cal_word(void)
{
    nd_energy_book_init(&book, 2000, ND_CAL_ADD_POS, 200);

    nd_energy_update_t update = nd_energy_add_pulses(&book, ND_ENERGY_REVERSE, 0, 20);

    TEST_ASSERT_TRUE(update.energy_changed);
    TEST_ASSERT_FALSE(update.month_changed);
    TEST_ASSERT_EQUAL_UINT32(0, book.month.integer);
}

static void test_combined_read_sums_both_directions(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 0, 20);
    nd_energy_add_pulses(&book, ND_ENERGY_REVERSE, 0, 20);

    TEST_ASSERT_EQUAL_INT64(200, nd_energy_read_combined(&book, 0,
                                                         ND_ENERGY_RES_0_0001_KWH));
}

static void test_tariff_zero_and_out_of_range_map_to_first_step(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 0, 20);
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 200, 20);

    TEST_ASSERT_EQUAL_UINT32(2, book.forward.slots[1].integer);
    TEST_ASSERT_EQUAL_UINT32(2, book.forward.slots[0].integer);
}

static void test_zero_meter_const_is_inert(void)
{
    nd_energy_book_init(&book, 0, ND_CAL_ADD_POS, 200);

    nd_energy_update_t update = nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 1, 100);

    TEST_ASSERT_FALSE(update.energy_changed);
    TEST_ASSERT_EQUAL_UINT32(0, book.forward.slots[0].integer);
}

static void test_clear_zeroes_energy_and_month(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 1, 4000);
    nd_energy_add_pulses(&book, ND_ENERGY_REVERSE, 2, 4000);

    nd_energy_clear(&book);

    TEST_ASSERT_EQUAL_UINT32(0, book.forward.slots[0].integer);
    TEST_ASSERT_EQUAL_UINT32(0, book.reverse.slots[2].integer);
    TEST_ASSERT_EQUAL_UINT32(0, book.month.integer);
}

static void test_set_total_matches_factory_behaviour(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 3, 4000);

    nd_energy_set_total(&book, ND_ENERGY_FORWARD, 1234);

    TEST_ASSERT_EQUAL_UINT32(1234, book.forward.slots[0].integer);
    TEST_ASSERT_EQUAL_UINT32(1234, book.forward.slots[1].integer);
    TEST_ASSERT_EQUAL_UINT32(0, book.forward.slots[3].integer);
    TEST_ASSERT_EQUAL_UINT32(0, book.month.integer);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_zeroes_book);
    RUN_TEST(test_one_pulse_lands_in_total_and_tariff);
    RUN_TEST(test_step_carries_into_hundredths_of_kwh);
    RUN_TEST(test_read_resolutions_match_legacy_formula);
    RUN_TEST(test_save_due_when_total_crosses_step_boundary);
    RUN_TEST(test_month_accumulates_when_cal_word_allows);
    RUN_TEST(test_month_ignores_direction_outside_cal_word);
    RUN_TEST(test_combined_read_sums_both_directions);
    RUN_TEST(test_tariff_zero_and_out_of_range_map_to_first_step);
    RUN_TEST(test_zero_meter_const_is_inert);
    RUN_TEST(test_clear_zeroes_energy_and_month);
    RUN_TEST(test_set_total_matches_factory_behaviour);
    return UNITY_END();
}
