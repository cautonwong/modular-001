#include <string.h>

#include "unity.h"

#include "targets/idis_ip_meter/cosem/nd_idis_calibration.h"
#include "targets/idis_ip_meter/cosem/nd_idis_event_desc.h"
#include "targets/idis_ip_meter/cosem/nd_idis_registry.h"

#define EEPROM_SIZE 0x10000u

/* The 16 B production-tool record: magic 0x43414C31 ("CAL1") and three
 * little-endian int32 factors, documented in docs/idis-object-registry.md.
 * There is no checksum field and no version, so the magic is the whole gate. */
static const uint8_t GOLDEN_CALIBRATION[16] = {
    0x31, 0x4C, 0x41, 0x43, 0x81, 0xA8, 0x12, 0x01,
    0x42, 0x81, 0xBA, 0x01, 0xA3, 0xF0, 0x19, 0x00,
};

static uint8_t image[EEPROM_SIZE];
static unsigned int writes;
static uint32_t last_write_offset;
static unsigned int current_reads;
static unsigned int legacy_reads;

static int fake_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    (void)ctx;
    if (offset > EEPROM_SIZE || length > EEPROM_SIZE - offset) {
        return -1;
    }
    if (offset == ND_IDIS_CALIBRATION_OFFSET) {
        current_reads++;
    }
    if (offset == ND_IDIS_CALIBRATION_LEGACY_OFFSET) {
        legacy_reads++;
    }
    memcpy(buffer, image + offset, length);
    return 0;
}

static int fake_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                      size_t length)
{
    (void)ctx;
    if (offset > EEPROM_SIZE || length > EEPROM_SIZE - offset) {
        return -1;
    }
    writes++;
    last_write_offset = offset;
    memcpy(image + offset, buffer, length);
    return 0;
}

static nd_store_t store = {
    .ctx = NULL,
    .read = fake_read,
    .write = fake_write,
    .erase = NULL,
};

static void seed_record(uint32_t offset, int32_t voltage, int32_t current,
                        int32_t power)
{
    nd_idis_calibration_record_t record = {
        .magic = ND_IDIS_CALIBRATION_MAGIC,
        .voltage = voltage,
        .current = current,
        .power = power,
    };
    memcpy(image + offset, &record, sizeof(record));
}

/* The production tool writes the blob; this builds the same bytes for the
 * documented maximum entry count the target actually ships today. */
static void seed_registry(uint16_t count)
{
    uint8_t *blob = image + ND_IDIS_REGISTRY_EEPROM_OFFSET;
    memcpy(blob, "NDRG", 4);
    blob[4] = 1;
    blob[5] = ND_IDIS_REGISTRY_ENTRY_SIZE;
    blob[6] = (uint8_t)(count >> 8);
    blob[7] = (uint8_t)count;
    for (uint16_t index = 0; index < count; index++) {
        uint8_t *entry = blob + ND_IDIS_REGISTRY_HEADER_SIZE
                         + (size_t)index * ND_IDIS_REGISTRY_ENTRY_SIZE;
        entry[0] = 8;
        entry[1] = 0;
        entry[2] = 0;
        entry[3] = 0;
        entry[4] = 1;
        entry[5] = 0;
        entry[6] = (uint8_t)(index >> 8);
        entry[7] = (uint8_t)index;
    }
    size_t length = ND_IDIS_REGISTRY_HEADER_SIZE
                    + (size_t)count * ND_IDIS_REGISTRY_ENTRY_SIZE;
    uint16_t sum = 0;
    for (size_t index = 0; index < length; index++) {
        sum = (uint16_t)(sum + blob[index]);
    }
    blob[length] = (uint8_t)(sum >> 8);
    blob[length + 1] = (uint8_t)sum;
}

static void test_current_slot_survives_a_full_registry_write(void)
{
    seed_record(ND_IDIS_CALIBRATION_OFFSET, 18000001, 29000002, 1700003);
    seed_registry(354);

    nd_idis_registry_t registry;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_load(
                                 &registry, &store,
                                 ND_IDIS_REGISTRY_EEPROM_OFFSET));
    TEST_ASSERT_EQUAL_UINT16(354, registry.count);

    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_LOADED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_INT32(18000001, record.voltage);
    TEST_ASSERT_EQUAL_INT32(29000002, record.current);
    TEST_ASSERT_EQUAL_INT32(1700003, record.power);
}

static void test_migration_adopts_the_legacy_record_once(void)
{
    seed_record(ND_IDIS_CALIBRATION_LEGACY_OFFSET, 18000011, 29000012,
                1700004);
    uint8_t legacy_before[ND_IDIS_CALIBRATION_RECORD_SIZE];
    memcpy(legacy_before, image + ND_IDIS_CALIBRATION_LEGACY_OFFSET,
           sizeof(legacy_before));

    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_ADOPTED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_INT32(18000011, record.voltage);
    TEST_ASSERT_EQUAL_INT32(29000012, record.current);
    TEST_ASSERT_EQUAL_INT32(1700004, record.power);
    TEST_ASSERT_EQUAL_UINT(1u, writes);
    TEST_ASSERT_EQUAL_UINT32(ND_IDIS_CALIBRATION_OFFSET, last_write_offset);
    TEST_ASSERT_EQUAL_MEMORY(legacy_before,
                             image + ND_IDIS_CALIBRATION_LEGACY_OFFSET,
                             sizeof(legacy_before));

    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_LOADED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(1u, writes);
}

static void test_damaged_legacy_record_reports_a_fault(void)
{
    seed_record(ND_IDIS_CALIBRATION_LEGACY_OFFSET, -1, 29000012, 1700004);
    nd_idis_calibration_record_t record;
    memset(&record, 0xA5, sizeof(record));

    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_FAULT,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
    TEST_ASSERT_EQUAL_INT32((int32_t)0xA5A5A5A5, record.voltage);
    TEST_ASSERT_EQUAL_UINT8(0xFF, image[ND_IDIS_CALIBRATION_OFFSET]);
}

static void test_damaged_current_record_reports_a_fault(void)
{
    seed_record(ND_IDIS_CALIBRATION_OFFSET, 18000001, 0, 1700003);
    nd_idis_calibration_record_t record = {0};

    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_FAULT,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
}

static void test_blank_eeprom_keeps_the_defaults(void)
{
    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_NONE,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
    TEST_ASSERT_EQUAL_INT32(0, record.voltage);
}

static void test_slot_sits_between_the_registry_and_the_energy_base(void)
{
    uint32_t registry_end = ND_IDIS_REGISTRY_EEPROM_OFFSET
                            + ND_IDIS_REGISTRY_HEADER_SIZE
                            + ND_IDIS_REGISTRY_MAX * ND_IDIS_REGISTRY_ENTRY_SIZE
                            + ND_IDIS_REGISTRY_TRAILER_SIZE;
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(registry_end,
                                        ND_IDIS_CALIBRATION_OFFSET);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(
        ND_IDIS_ENERGY_BASE_OFFSET,
        ND_IDIS_CALIBRATION_OFFSET + ND_IDIS_CALIBRATION_RECORD_SIZE);
}

static void test_registry_trailer_over_the_legacy_slot_reports_lost(void)
{
    /* 255 entries put the 2-byte blob checksum at 0xE000. */
    seed_registry(255);
    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_LOST,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
}

static void test_full_registry_over_the_legacy_slot_reports_lost(void)
{
    seed_registry(354);
    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_LOST,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
}

static void test_registry_below_the_legacy_slot_keeps_defaults(void)
{
    seed_registry(254);
    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_NONE,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
}

static void test_absent_registry_does_not_report_lost(void)
{
    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_NONE,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(0u, writes);
}

static void test_legacy_slot_is_never_read_after_adoption(void)
{
    seed_record(ND_IDIS_CALIBRATION_LEGACY_OFFSET, 18000011, 29000012,
                1700004);
    nd_idis_calibration_record_t record = {0};
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_ADOPTED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(1u, legacy_reads);
    TEST_ASSERT_EQUAL_UINT(1u, current_reads);

    seed_record(ND_IDIS_CALIBRATION_LEGACY_OFFSET, 1, 2, 3);
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_LOADED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(1u, legacy_reads);
    TEST_ASSERT_EQUAL_UINT(2u, current_reads);
    TEST_ASSERT_EQUAL_INT32(18000011, record.voltage);
}

static void test_damaged_or_lost_record_maps_to_the_nv_memory_error(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, nd_idis_calibration_fault_event(
                                    ND_IDIS_CALIBRATION_NONE));
    TEST_ASSERT_EQUAL_UINT16(0, nd_idis_calibration_fault_event(
                                    ND_IDIS_CALIBRATION_LOADED));
    TEST_ASSERT_EQUAL_UINT16(0, nd_idis_calibration_fault_event(
                                    ND_IDIS_CALIBRATION_ADOPTED));
    TEST_ASSERT_EQUAL_UINT16(
        ND_IDIS_CALIBRATION_FAULT_EVENT,
        nd_idis_calibration_fault_event(ND_IDIS_CALIBRATION_FAULT));
    TEST_ASSERT_EQUAL_UINT16(
        ND_IDIS_CALIBRATION_FAULT_EVENT,
        nd_idis_calibration_fault_event(ND_IDIS_CALIBRATION_LOST));

    const nd_idis_event_descriptor_t *descriptor =
        nd_idis_event_describe(ND_IDIS_CALIBRATION_FAULT_EVENT);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_INT(ND_EVENT_ERROR, descriptor->kind);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_SEVERITY_ERROR, descriptor->severity);
    TEST_ASSERT_EQUAL_UINT8(ND_EVENT_SOURCE_MEMORY, descriptor->source);
}

static void test_golden_current_slot_loads(void)
{
    nd_idis_calibration_record_t record = {0};

    memcpy(image + ND_IDIS_CALIBRATION_OFFSET, GOLDEN_CALIBRATION,
           sizeof(GOLDEN_CALIBRATION));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_LOADED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_INT32(18000001, record.voltage);
    TEST_ASSERT_EQUAL_INT32(29000002, record.current);
    TEST_ASSERT_EQUAL_INT32(1700003, record.power);
}

static void test_golden_legacy_adoption_copies_the_literal(void)
{
    nd_idis_calibration_record_t record = {0};

    memcpy(image + ND_IDIS_CALIBRATION_LEGACY_OFFSET, GOLDEN_CALIBRATION,
           sizeof(GOLDEN_CALIBRATION));
    TEST_ASSERT_EQUAL_INT(ND_IDIS_CALIBRATION_ADOPTED,
                          nd_idis_calibration_load(&record, &store));
    TEST_ASSERT_EQUAL_UINT(1u, writes);
    TEST_ASSERT_EQUAL_UINT32(ND_IDIS_CALIBRATION_OFFSET, last_write_offset);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(GOLDEN_CALIBRATION,
                                  image + ND_IDIS_CALIBRATION_OFFSET,
                                  sizeof(GOLDEN_CALIBRATION));
}

void setUp(void)
{
    memset(image, 0xFF, sizeof(image));
    writes = 0;
    last_write_offset = 0;
    current_reads = 0;
    legacy_reads = 0;
}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_current_slot_survives_a_full_registry_write);
    RUN_TEST(test_migration_adopts_the_legacy_record_once);
    RUN_TEST(test_damaged_legacy_record_reports_a_fault);
    RUN_TEST(test_damaged_current_record_reports_a_fault);
    RUN_TEST(test_blank_eeprom_keeps_the_defaults);
    RUN_TEST(test_slot_sits_between_the_registry_and_the_energy_base);
    RUN_TEST(test_registry_trailer_over_the_legacy_slot_reports_lost);
    RUN_TEST(test_full_registry_over_the_legacy_slot_reports_lost);
    RUN_TEST(test_registry_below_the_legacy_slot_keeps_defaults);
    RUN_TEST(test_absent_registry_does_not_report_lost);
    RUN_TEST(test_legacy_slot_is_never_read_after_adoption);
    RUN_TEST(test_damaged_or_lost_record_maps_to_the_nv_memory_error);
    RUN_TEST(test_golden_current_slot_loads);
    RUN_TEST(test_golden_legacy_adoption_copies_the_literal);
    return UNITY_END();
}
