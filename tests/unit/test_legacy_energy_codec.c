#include "unity.h"

#include <string.h>

#include "domain/metering/energy.h"
#include "targets/stategrid_meter/persistence/legacy_energy_codec.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"

static nd_energy_book_t book;
static legacy_energy_record_t record;

void setUp(void)
{
    nd_energy_book_init(&book, 2000, ND_CAL_ADD_POS | ND_CAL_ADD_NEG, 200);
    memset(&record, 0, sizeof(record));
}

void tearDown(void) {}

static void test_legacy_offsets_are_frozen(void)
{
    TEST_ASSERT_EQUAL_size_t(172, sizeof(legacy_energy_record_t));
    TEST_ASSERT_EQUAL_size_t(3, ND_LEGACY_ENERGY_OFFSET_IN_BLOCK);
}

static void test_encode_maps_domain_values_to_legacy_fields(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 2, 4000);
    nd_energy_add_pulses(&book, ND_ENERGY_REVERSE, 3, 20);

    legacy_energy_encode(&book, &record);

    TEST_ASSERT_EQUAL_UINT32(200, record.active_p[0]);
    TEST_ASSERT_EQUAL_UINT32(200, record.active_p[2]);
    TEST_ASSERT_EQUAL_UINT32(1, record.active_n[3]);
    TEST_ASSERT_EQUAL_UINT16(0, record.active_p_dec[0]);
    TEST_ASSERT_EQUAL_UINT32(201, record.month_energy);
}

static void test_decode_restores_domain_values(void)
{
    nd_energy_add_pulses(&book, ND_ENERGY_FORWARD, 2, 4017);
    nd_energy_add_pulses(&book, ND_ENERGY_REVERSE, 3, 37);
    legacy_energy_encode(&book, &record);

    nd_energy_book_t restored;
    nd_energy_book_init(&restored, 2000, ND_CAL_ADD_POS | ND_CAL_ADD_NEG, 200);
    legacy_energy_decode(&record, &restored);

    TEST_ASSERT_EQUAL_UINT32(book.forward.slots[0].integer, restored.forward.slots[0].integer);
    TEST_ASSERT_EQUAL_UINT16(book.forward.slots[2].fraction, restored.forward.slots[2].fraction);
    TEST_ASSERT_EQUAL_UINT32(book.reverse.slots[3].integer, restored.reverse.slots[3].integer);
    TEST_ASSERT_EQUAL_UINT32(book.month.integer, restored.month.integer);
    TEST_ASSERT_EQUAL_UINT16(book.month.fraction, restored.month.fraction);
}

static void test_encode_leaves_checksum_and_padding_untouched(void)
{
    record.energy_checksum = 0xAABBCCDD;
    record.use_energy_checksum = 0x11223344;
    record.reserved = 0x5A5A;

    legacy_energy_encode(&book, &record);

    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDD, record.energy_checksum);
    TEST_ASSERT_EQUAL_UINT32(0x11223344, record.use_energy_checksum);
    TEST_ASSERT_EQUAL_UINT16(0x5A5A, record.reserved);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_legacy_offsets_are_frozen);
    RUN_TEST(test_encode_maps_domain_values_to_legacy_fields);
    RUN_TEST(test_decode_restores_domain_values);
    RUN_TEST(test_encode_leaves_checksum_and_padding_untouched);
    return UNITY_END();
}
