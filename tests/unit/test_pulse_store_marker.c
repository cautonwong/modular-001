/* Purpose: lock in the marker-restore fix that candidate 2 missed. After a
 * reboot the engine must restore marker_count and the current tariff from the
 * valid marker scan, otherwise the first post-reboot tariff change rewrites
 * slot 0 and the tariff timeline is corrupted. The cases reboot after every
 * marker slot, check that an unchanged tariff writes no marker, and compare
 * slot 0 byte for byte across a post-reboot tariff change. */
#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "domain/metering/pulse_lane.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/pulse_store.h"
#include "support/pulse_harness.h"

#define JOURNAL_BYTES 16384u
#define BASE_BYTES 1024u
#define MEMORY_BYTES (JOURNAL_BYTES + 2u * BASE_BYTES)
#define MARKER_BYTES 16u
#define MARKER_ZERO (2u * 16u) /* after the two journal headers */

static uint8_t memory[MEMORY_BYTES];
static nd_block_ram_t block;
static nd_pulse_store_storage_t storage;

static nd_pulse_store_config_t config_over(void)
{
    nd_pulse_store_config_t config = {
        .block = nd_block_ram_port(&block),
        .journal_offset = 0,
        .journal_bytes = JOURNAL_BYTES,
        .base_offset = {JOURNAL_BYTES, JOURNAL_BYTES + BASE_BYTES},
        .base_capacity = BASE_BYTES,
        .granularity = ND_PULSE_PROGRAM_GRANULARITY,
    };

    return config;
}

static void start(void)
{
    nd_pulse_store_config_t config;

    memset(memory, 0xFF, sizeof(memory));
    nd_block_ram_init(&block, memory, sizeof(memory));
    config = config_over();
    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
}

static void reopen(void)
{
    nd_pulse_store_config_t config = config_over();

    TEST_ASSERT_EQUAL_INT(0, nd_pulse_store_init(&storage, &config));
}

static void seal(void)
{
    nd_energy_set_t set;

    pulse_set_init(&set);
    TEST_ASSERT_EQUAL_INT(ND_PULSE_OK,
                          nd_pulse_store_commit(nd_pulse_store_port(&storage),
                                                (const uint8_t *)&set,
                                                sizeof(set)));
}

static void append_lane(nd_energy_direction_t direction, uint8_t tariff)
{
    TEST_ASSERT_EQUAL_INT(
        ND_PULSE_OK,
        nd_pulse_store_append(nd_pulse_store_port(&storage),
                              nd_energy_pulse_lane(direction, tariff)));
}

static int marker_blank(uint8_t slot)
{
    for (uint32_t index = 0; index < MARKER_BYTES; index++) {
        if (memory[MARKER_ZERO + slot * MARKER_BYTES + index] != 0xFFu) {
            return 0;
        }
    }
    return 1;
}

static void expect_slots(uint8_t tariffs)
{
    nd_energy_set_t set;

    reopen();
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    for (uint8_t tariff = 1; tariff <= tariffs; tariff++) {
        TEST_ASSERT_EQUAL_UINT32(1, pulse_slot_count(&set, ND_ENERGY_FORWARD,
                                                     tariff));
    }
    TEST_ASSERT_EQUAL_UINT32(tariffs, pulse_total(&set));
}

void setUp(void)
{
    start();
}

void tearDown(void) {}

static void test_first_post_reboot_change_does_not_overwrite_slot_zero(void)
{
    uint8_t markers[MARKER_BYTES * 2u];
    nd_energy_set_t set;

    seal();
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 2); /* slots 0 (t1) and 1 (t2) are used */
    memcpy(markers, memory + MARKER_ZERO, sizeof(markers));
    reopen();
    append_lane(ND_ENERGY_FORWARD, 3); /* must go to slot 2 */
    TEST_ASSERT_EQUAL_MEMORY(markers, memory + MARKER_ZERO, sizeof(markers));
    TEST_ASSERT_TRUE(marker_blank(3));
    reopen();
    TEST_ASSERT_EQUAL_INT(0, pulse_recover(nd_pulse_store_port(&storage), &set));
    TEST_ASSERT_EQUAL_UINT32(2, pulse_slot_count(&set, ND_ENERGY_FORWARD, 1));
    TEST_ASSERT_EQUAL_UINT32(1, pulse_slot_count(&set, ND_ENERGY_FORWARD, 2));
    TEST_ASSERT_EQUAL_UINT32(1, pulse_slot_count(&set, ND_ENERGY_FORWARD, 3));
    TEST_ASSERT_EQUAL_UINT32(4, pulse_total(&set));
}

static void test_unchanged_tariff_after_reboot_writes_no_marker(void)
{
    seal();
    append_lane(ND_ENERGY_FORWARD, 1);
    append_lane(ND_ENERGY_FORWARD, 2); /* slots 0 (t1) and 1 (t2) are used */
    reopen();
    append_lane(ND_ENERGY_FORWARD, 2); /* same tariff: no new marker */
    TEST_ASSERT_TRUE(marker_blank(2));
    append_lane(ND_ENERGY_FORWARD, 3); /* now slot 2 */
    TEST_ASSERT_FALSE(marker_blank(2));
    {
        nd_energy_set_t set;

        reopen();
        TEST_ASSERT_EQUAL_INT(0,
                              pulse_recover(nd_pulse_store_port(&storage), &set));
        TEST_ASSERT_EQUAL_UINT32(1, pulse_slot_count(&set, ND_ENERGY_FORWARD, 1));
        TEST_ASSERT_EQUAL_UINT32(2, pulse_slot_count(&set, ND_ENERGY_FORWARD, 2));
        TEST_ASSERT_EQUAL_UINT32(1, pulse_slot_count(&set, ND_ENERGY_FORWARD, 3));
        TEST_ASSERT_EQUAL_UINT32(4, pulse_total(&set));
    }
}

static void test_reboot_after_every_marker_slot(void)
{
    seal();
    expect_slots(0);
    for (uint8_t tariff = 1; tariff <= 8u; tariff++) {
        append_lane(ND_ENERGY_FORWARD, tariff);
        reopen();
        expect_slots(tariff);
        TEST_ASSERT_TRUE(marker_blank(tariff)); /* next slot still free */
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_first_post_reboot_change_does_not_overwrite_slot_zero);
    RUN_TEST(test_unchanged_tariff_after_reboot_writes_no_marker);
    RUN_TEST(test_reboot_after_every_marker_slot);
    return UNITY_END();
}
