#include <string.h>

#include "unity.h"

#include "targets/idis_ip_meter/cosem/classes/nd_ic_association.h"
#include "targets/idis_ip_meter/cosem/nd_idis_registry.h"
#include "targets/idis_ip_meter/mapper_idis/nd_idis_ids.h"

#define STORE_SIZE 64

static uint8_t store_data[STORE_SIZE];

static int fake_read(void *ctx, uint32_t offset, uint8_t *buffer, size_t length)
{
    (void)ctx;
    if (offset + length > STORE_SIZE) {
        return -1;
    }
    memcpy(buffer, store_data + offset, length);
    return 0;
}

static int fake_write(void *ctx, uint32_t offset, const uint8_t *buffer,
                      size_t length)
{
    (void)ctx;
    if (offset + length > STORE_SIZE) {
        return -1;
    }
    memcpy(store_data + offset, buffer, length);
    return 0;
}

static nd_store_t store = {
    .ctx = NULL,
    .read = fake_read,
    .write = fake_write,
    .erase = NULL,
};

static uint16_t blob_checksum(const uint8_t *blob, size_t length)
{
    uint16_t sum = 0;
    for (size_t index = 0; index < length; index++) {
        sum = (uint16_t)(sum + blob[index]);
    }
    return sum;
}

static size_t make_blob(uint16_t count)
{
    uint8_t *blob = store_data;
    blob[0] = 'N';
    blob[1] = 'D';
    blob[2] = 'R';
    blob[3] = 'G';
    blob[4] = 1;
    blob[5] = ND_IDIS_REGISTRY_ENTRY_SIZE;
    blob[6] = (uint8_t)(count >> 8);
    blob[7] = (uint8_t)count;
    uint8_t *entry = blob + ND_IDIS_REGISTRY_HEADER_SIZE;
    entry[0] = 8;
    entry[1] = 0;
    entry[2] = 0;
    entry[3] = 0;
    entry[4] = 1;
    entry[5] = 0;
    entry[6] = 0;
    entry[7] = 255;
    entry[8] = 3;
    entry[9] = 0;
    entry[10] = 1;
    entry[11] = 0;
    entry[12] = 1;
    entry[13] = 8;
    entry[14] = 0;
    entry[15] = 255;
    size_t length = ND_IDIS_REGISTRY_HEADER_SIZE + 2 * ND_IDIS_REGISTRY_ENTRY_SIZE;
    uint16_t sum = blob_checksum(blob, length);
    blob[length] = (uint8_t)(sum >> 8);
    blob[length + 1] = (uint8_t)sum;
    return length + ND_IDIS_REGISTRY_TRAILER_SIZE;
}

static void test_load_reads_and_validates_entries(void)
{
    make_blob(2);
    nd_idis_registry_t registry;
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_load(&registry, &store, 0));
    TEST_ASSERT_EQUAL_UINT16(2, registry.count);
    TEST_ASSERT_EQUAL_UINT8(8, registry.entries[0].class_id);
    TEST_ASSERT_EQUAL_UINT8(1, registry.entries[0].ln.a == 0
                                   && registry.entries[0].ln.b == 0
                                   && registry.entries[0].ln.c == 1);
    TEST_ASSERT_EQUAL_UINT8(3, registry.entries[1].class_id);
}

static void test_load_rejects_bad_magic(void)
{
    make_blob(2);
    store_data[0] = 'X';
    nd_idis_registry_t registry;
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_registry_load(&registry, &store, 0));
    TEST_ASSERT_EQUAL_UINT16(0, registry.count);
}

static void test_load_rejects_bad_checksum(void)
{
    make_blob(2);
    store_data[ND_IDIS_REGISTRY_HEADER_SIZE] ^= 0xFF;
    nd_idis_registry_t registry;
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_registry_load(&registry, &store, 0));
    TEST_ASSERT_EQUAL_UINT16(0, registry.count);
}

static void test_load_rejects_oversized_count(void)
{
    make_blob(2);
    store_data[6] = (uint8_t)((ND_IDIS_REGISTRY_MAX + 1) >> 8);
    store_data[7] = (uint8_t)(ND_IDIS_REGISTRY_MAX + 1);
    nd_idis_registry_t registry;
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_registry_load(&registry, &store, 0));
}

static void test_remove_deletes_matching_entry(void)
{
    nd_idis_registry_t registry;
    nd_idis_registry_clear(&registry);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_add(&registry, 8, 0,
                                                  &ND_OBIS_CLOCK));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_add(&registry, 3, 0,
                                                  &ND_OBIS_ACTIVE_ENERGY_IMPORT));
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_remove(&registry, 8,
                                                     &ND_OBIS_CLOCK));
    TEST_ASSERT_EQUAL_UINT16(1, registry.count);
    TEST_ASSERT_EQUAL_UINT8(3, registry.entries[0].class_id);
    TEST_ASSERT_EQUAL_INT(-1, nd_idis_registry_remove(&registry, 8,
                                                      &ND_OBIS_CLOCK));
}

static void test_blob_fits_the_documented_budget(void)
{
    TEST_ASSERT_EQUAL_UINT32(2890, nd_idis_registry_blob_size(360));
    TEST_ASSERT_EQUAL_UINT32(2880,
                             (uint32_t)sizeof(nd_idis_registry_entry_t) * 360);
}

static void test_object_list_encodes_loaded_registry(void)
{
    make_blob(2);
    nd_idis_association_t association;
    nd_idis_association_init(&association);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_load(&association.association.base,
                                                   &store, 0));
    uint8_t buffer[256];
    nd_axdr_writer_t out;
    nd_axdr_writer_init(&out, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(ND_OK,
                          nd_ic_association_get(&association, 16,
                                                &ND_OBIS_ASSOCIATION, 2,
                                                NULL, &out));
    TEST_ASSERT_GREATER_THAN_UINT32(0, out.len);
    TEST_ASSERT_EQUAL_UINT8(ND_AXDR_ARRAY, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(2, buffer[1]);
}

static void test_every_single_byte_corruption_is_rejected(void)
{
    size_t length = make_blob(2);
    uint8_t original[STORE_SIZE];
    nd_idis_registry_t registry;

    memcpy(original, store_data, length);
    for (size_t pos = 0; pos < length; pos++) {
        for (unsigned value = 0; value <= 0xFF; value++) {
            if (value == original[pos]) {
                continue;
            }
            memcpy(store_data, original, length);
            store_data[pos] = (uint8_t)value;
            TEST_ASSERT_EQUAL_INT(-1,
                                  nd_idis_registry_load(&registry, &store, 0));
        }
    }
    memcpy(store_data, original, length);
    TEST_ASSERT_EQUAL_INT(0, nd_idis_registry_load(&registry, &store, 0));
}

void setUp(void)
{
    memset(store_data, 0, sizeof(store_data));
}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_load_reads_and_validates_entries);
    RUN_TEST(test_load_rejects_bad_magic);
    RUN_TEST(test_load_rejects_bad_checksum);
    RUN_TEST(test_load_rejects_oversized_count);
    RUN_TEST(test_remove_deletes_matching_entry);
    RUN_TEST(test_blob_fits_the_documented_budget);
    RUN_TEST(test_object_list_encodes_loaded_registry);
    RUN_TEST(test_every_single_byte_corruption_is_rejected);
    return UNITY_END();
}
