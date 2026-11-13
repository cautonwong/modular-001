#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "domain/metering/metering.h"
#include "infra/memory/arena.h"
#include "kernel/legacy_control_block.h"
#include "support/file_store.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"
#include "targets/stategrid_meter/persistence/legacy_energy_store.h"

#define EEPROM_PATH "backupfiles/eeprom.bin"
#define FLASH_PATH "backupfiles/flash.bin"
#define EEPROM_SIZE 1368u
#define FLASH_SIZE (256u * 1024u)
#define FLASH_SECTOR 4096u

#define PERSISTENT_OFFSET ((uint32_t)offsetof(struct mbase_control_block_t, persistent_section_start))
#define ENERGY_OFFSET (PERSISTENT_OFFSET + (uint32_t)ND_LEGACY_ENERGY_OFFSET_IN_BLOCK)
#define ENERGY_END (ENERGY_OFFSET + (uint32_t)sizeof(legacy_energy_record_t))

static unsigned char arena_memory[4096];
static nd_arena_t arena;
static file_store_t eeprom;
static file_store_t flash;
static metering_config_t config;
static metering_state_t *state;

static metering_state_t *new_metering(void)
{
    return metering_create(nd_arena_allocator(&arena), &config);
}

static void assert_bytes(const uint8_t *bytes, size_t length, uint8_t value)
{
    size_t i;

    for (i = 0; i < length; i++) {
        TEST_ASSERT_EQUAL_UINT8(value, bytes[i]);
    }
}

void setUp(void)
{
    unlink(EEPROM_PATH);
    unlink(FLASH_PATH);

    nd_arena_init(&arena, arena_memory, sizeof(arena_memory));
    config.meter_const = 2000;
    config.cal_word = ND_CAL_ADD_POS | ND_CAL_ADD_NEG;
    config.save_step = 200;

    state = new_metering();
    TEST_ASSERT_NOT_NULL(state);

    TEST_ASSERT_EQUAL_INT(0, file_store_open(&eeprom, EEPROM_PATH, EEPROM_SIZE, 0xFF));
    TEST_ASSERT_EQUAL_INT(0, file_store_open(&flash, FLASH_PATH, FLASH_SIZE, 0xFF));
}

void tearDown(void)
{
    file_store_close(&eeprom);
    file_store_close(&flash);
    metering_destroy(state, nd_arena_allocator(&arena));
}

static void test_layout_matches_the_file_image(void)
{
    TEST_ASSERT_EQUAL_size_t(1368u, sizeof(struct mbase_control_block_t));
    TEST_ASSERT_EQUAL_size_t(849u, offsetof(struct mbase_control_block_t,
                                            persistent_section_start));
    TEST_ASSERT_EQUAL_size_t(852u, offsetof(struct mbase_control_block_t, active_p));
    TEST_ASSERT_EQUAL_size_t(904u, offsetof(struct mbase_control_block_t, active_n));
    TEST_ASSERT_EQUAL_UINT32(852u, ENERGY_OFFSET);
    TEST_ASSERT_EQUAL_UINT32(1024u, ENERGY_END);
    TEST_ASSERT_EQUAL_UINT32(EEPROM_SIZE, eeprom.size);
}

static void test_energy_survives_close_and_reopen(void)
{
    legacy_energy_record_t record;
    struct stat info;
    uint8_t before[849];
    uint8_t after[344];

    metering_add_pulses(state, ND_ENERGY_FORWARD, 1, 4000);
    TEST_ASSERT_EQUAL_INT(0, legacy_energy_save(file_store_port(&eeprom),
                                                PERSISTENT_OFFSET, state));

    file_store_close(&eeprom);
    TEST_ASSERT_EQUAL_INT(0, file_store_open(&eeprom, EEPROM_PATH, EEPROM_SIZE, 0xFF));

    metering_state_t *restored = new_metering();
    TEST_ASSERT_NOT_NULL(restored);
    TEST_ASSERT_EQUAL_INT(0, legacy_energy_load(file_store_port(&eeprom),
                                                PERSISTENT_OFFSET, restored));
    TEST_ASSERT_EQUAL_INT64(metering_read(state, ND_ENERGY_FORWARD, 1,
                                          ND_ENERGY_RES_0_0001_KWH),
                            metering_read(restored, ND_ENERGY_FORWARD, 1,
                                          ND_ENERGY_RES_0_0001_KWH));
    metering_destroy(restored, nd_arena_allocator(&arena));

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(file_store_port(&eeprom), ENERGY_OFFSET,
                                           (uint8_t *)&record, sizeof(record)));
    TEST_ASSERT_EQUAL_UINT32(200u, record.active_p[0]);

    TEST_ASSERT_EQUAL_INT(0, stat(EEPROM_PATH, &info));
    TEST_ASSERT_EQUAL_INT(1368, (int)info.st_size);

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(file_store_port(&eeprom), 0,
                                           before, sizeof(before)));
    assert_bytes(before, sizeof(before), 0xFF);
    TEST_ASSERT_EQUAL_INT(0, nd_store_read(file_store_port(&eeprom), ENERGY_END,
                                           after, sizeof(after)));
    assert_bytes(after, sizeof(after), 0xFF);
}

static void test_flash_erase_and_reopen(void)
{
    const uint8_t pattern[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t readback[8];
    size_t i;

    TEST_ASSERT_EQUAL_INT(0, nd_store_write(file_store_port(&flash), 0,
                                            pattern, sizeof(pattern)));
    TEST_ASSERT_EQUAL_INT(0, nd_store_write(file_store_port(&flash), 8192,
                                            pattern, sizeof(pattern)));
    TEST_ASSERT_EQUAL_INT(0, file_store_erase(&flash, 0, FLASH_SECTOR));

    file_store_close(&flash);
    TEST_ASSERT_EQUAL_INT(0, file_store_open(&flash, FLASH_PATH, FLASH_SIZE, 0xFF));

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(file_store_port(&flash), 0,
                                           readback, sizeof(readback)));
    for (i = 0; i < sizeof(readback); i++) {
        TEST_ASSERT_EQUAL_UINT8(0xFF, readback[i]);
    }

    TEST_ASSERT_EQUAL_INT(0, nd_store_read(file_store_port(&flash), 8192,
                                           readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(pattern, readback, sizeof(pattern));

    TEST_ASSERT_EQUAL_INT(-1, nd_store_read(file_store_port(&flash),
                                            FLASH_SIZE - 4u, readback, sizeof(readback)));
    TEST_ASSERT_EQUAL_INT(-1, file_store_erase(&flash, FLASH_SIZE - 4u, 8u));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_layout_matches_the_file_image);
    RUN_TEST(test_energy_survives_close_and_reopen);
    RUN_TEST(test_flash_erase_and_reopen);
    return UNITY_END();
}
