/*
 * Instrumented read counts for the stategrid 698 mbase record path.
 *
 * The production search is `search_mid_way` and the record read is
 * `log_output_engine` in tools/app_resource_tool/resource_tool.c. The test
 * fills the instance port with `resource_tool_attach` and drives the same
 * `nd_log_output_port` entry the 698 record GETs call, counting
 * `nd_storage_read` calls at the device boundary. A count query
 * (`valid_num == 0`, one-byte buffer) pays only the mark search; each record
 * call pays one whole-slot read.
 *
 * A 64-slot ring is indexed by sequence mark (mode 1) and by time mark
 * (mode 2), the two single-frame selections. GET_3_Event has no seam below
 * its 1416-line legacy path; it reaches this same manager through
 * `nd_log_output_port`, so the manager counts cover it too.
 */
#include "unity.h"

#include <stdio.h>
#include <string.h>

#include "targets/stategrid_meter/tools/app_resource_tool/resource_tool.h"
#include "targets/stategrid_meter/app/storage_device.h"

#define SLOT_BYTES 16u
#define SPAN 64u
#define FLASH_BYTES (SPAN * SLOT_BYTES)
#define EEPROM_BYTES 0x2000u
#define MANAGER_ADDR 0x100u
#define RECORD_BLOCK 0x21u
#define MANAGER_BLOCK 0x31u

/* Two extra bytes after the 10-byte packed manager hold verify_data's
 * big-endian checksum. */
#define MANAGER_BYTES (sizeof(struct log_manager_t) + 2u)

static uint8_t flash_memory[FLASH_BYTES];
static uint8_t eeprom_memory[EEPROM_BYTES];

static stategrid_board_state_t test_board;

stategrid_board_state_t *stategrid_board_root(void)
{
    return &test_board;
}

static unsigned int mark_reads;
static unsigned int slot_reads;
static unsigned int other_reads;

int nd_storage_read(stategrid_context_t *instance, uint8_t device_name,
                    uint32_t addr, uint8_t *buf, uint16_t len)
{
    (void)instance;
    const uint8_t *memory =
        device_name == RES_DEV_FLASH ? flash_memory : eeprom_memory;
    if (len == 4u) {
        mark_reads++;
    } else if (len == SLOT_BYTES) {
        slot_reads++;
    } else {
        other_reads++;
    }
    memcpy(buf, memory + addr, len);
    return 0;
}

int nd_storage_write(stategrid_context_t *instance, uint8_t device_name,
                     uint32_t addr, const uint8_t *buf, uint16_t len)
{
    (void)instance;
    uint8_t *memory =
        device_name == RES_DEV_FLASH ? flash_memory : eeprom_memory;
    memcpy(memory + addr, buf, len);
    return 0;
}

int nd_storage_erase(stategrid_context_t *instance, uint8_t device_name,
                     uint32_t addr)
{
    (void)instance;
    uint8_t *memory =
        device_name == RES_DEV_FLASH ? flash_memory : eeprom_memory;
    memset(memory + addr, 0xFF, 4096u);
    return 0;
}

uint32_t nd_storage_size(stategrid_context_t *instance,
                         uint8_t device_name)
{
    (void)instance;
    return device_name == RES_DEV_FLASH ? FLASH_BYTES : EEPROM_BYTES;
}

/* resource_tool.c references the static checksum for its write path; this
 * test never writes through it. */
uint32_t do_block_static_checksum(void *instance)
{
    (void)instance;
    return 0;
}

static struct block_device_t record_device;
static struct block_device_t manager_device;
static struct resource_block_t record_block;
static struct resource_block_t manager_block;
static stategrid_context_t instance;

static int16_t record_hook(stategrid_context_t *ctx, uint16_t id,
                           uint32_t *addr, uint32_t *len, uint32_t *depth)
{
    (void)ctx;
    (void)id;
    *addr = 0;
    *len = FLASH_BYTES;
    if (depth != NULL) {
        *depth = SPAN;
    }
    return 0;
}

static int16_t manager_hook(stategrid_context_t *ctx, uint16_t id,
                            uint32_t *addr, uint32_t *len, uint32_t *depth)
{
    (void)ctx;
    (void)id;
    *addr = 0;
    *len = MANAGER_BYTES;
    if (depth != NULL) {
        *depth = 0;
    }
    return 0;
}

static void put_u32_msb(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

static void build_ring(void)
{
    memset(flash_memory, 0, sizeof(flash_memory));
    memset(eeprom_memory, 0xFF, sizeof(eeprom_memory));
    for (uint32_t slot = 0; slot < SPAN; slot++) {
        uint8_t *record = flash_memory + slot * SLOT_BYTES;
        record[0] = (uint8_t)slot;
        put_u32_msb(record + CIRCULAR_OFFSET_SEQUENCE, 1000u + slot * 10u);
        put_u32_msb(record + CIRCULAR_OFFSET_TIME, 50000u + slot * 60u);
    }

    /*
     * r_ptr 0, w_ptr one past the last slot, num below max_num: an
     * unwrapped ring, so search_circular probes one contiguous window.
     */
    struct log_manager_t manager = {
        .r_ptr = 0,
        .w_ptr = SPAN,
        .num = SPAN,
        .unit_size = SLOT_BYTES,
        .max_num = 2u * SPAN,
    };
    memcpy(eeprom_memory + MANAGER_ADDR, &manager, sizeof(manager));
    uint16_t sum = 0xA5A5;
    for (uint32_t index = 0; index < sizeof(manager); index++) {
        sum = (uint16_t)(sum + eeprom_memory[MANAGER_ADDR + index]);
    }
    eeprom_memory[MANAGER_ADDR + sizeof(manager)] = (uint8_t)(sum >> 8);
    eeprom_memory[MANAGER_ADDR + sizeof(manager) + 1u] = (uint8_t)sum;

    memset(&instance, 0, sizeof(instance));
    init_list_head(&instance.dev_list);
    init_list_head(&instance.map_list);

    memset(&record_device, 0, sizeof(record_device));
    record_device.device_name = RES_DEV_FLASH;
    record_device.first_addr = 0;
    record_device.size = FLASH_BYTES;
    record_device.page_size = 4096u;
    init_list_head(&record_device.sibling);
    init_list_head(&record_device.children);
    list_add_tail(&record_device.sibling, &instance.dev_list);

    memset(&manager_device, 0, sizeof(manager_device));
    manager_device.device_name = RES_DEV_EEPROM;
    manager_device.first_addr = 0;
    manager_device.size = EEPROM_BYTES;
    init_list_head(&manager_device.sibling);
    init_list_head(&manager_device.children);
    list_add_tail(&manager_device.sibling, &instance.dev_list);

    memset(&record_block, 0, sizeof(record_block));
    record_block.name = RECORD_BLOCK;
    record_block.first_addr = 0;
    record_block.size = FLASH_BYTES;
    record_block.relation_device = RES_DEV_EEPROM;
    record_block.relation_name = MANAGER_BLOCK;
    record_block.hook = record_hook;
    init_list_head(&record_block.sibling);
    list_add_tail(&record_block.sibling, &record_device.children);

    memset(&manager_block, 0, sizeof(manager_block));
    manager_block.name = MANAGER_BLOCK;
    manager_block.first_addr = MANAGER_ADDR;
    manager_block.size = MANAGER_BYTES;
    manager_block.hook = manager_hook;
    init_list_head(&manager_block.sibling);
    list_add_tail(&manager_block.sibling, &manager_device.children);

    resource_tool_attach(&instance);
}

void setUp(void)
{
    build_ring();
    mark_reads = 0;
    slot_reads = 0;
    other_reads = 0;
}

void tearDown(void) {}

static const uint32_t log_index =
    ((uint32_t)RES_DEV_FLASH << 24) | ((uint32_t)RECORD_BLOCK << 16);

static void run_selection(uint8_t mode, uint32_t start_mark,
                          uint32_t end_mark, uint32_t first_slot,
                          uint32_t rows, unsigned int expected_marks)
{
    struct circular_info_t info;
    uint8_t record[SLOT_BYTES];
    memset(&info, 0, sizeof(info));
    info.mode = mode;
    info.start_mark = start_mark;
    info.end_mark = end_mark;

    TEST_ASSERT_EQUAL_INT(0,
                          nd_log_output_port(&instance.resource, log_index,
                                             record, 1, &info));
    TEST_ASSERT_EQUAL_UINT16(first_slot, info.read_ptr);
    TEST_ASSERT_EQUAL_UINT16(rows, info.valid_num);
    TEST_ASSERT_EQUAL_UINT(0, slot_reads);
    TEST_ASSERT_EQUAL_UINT(1, other_reads);
    printf("mode %u: %u mark reads for %u rows\n", mode, mark_reads, rows);
    /* Exact probe count for this ring; the 64-slot worst case is
     * 2 smoke reads + 2 searches * 7 windows * (mid + boundary) = 30. */
    TEST_ASSERT_EQUAL_UINT(expected_marks, mark_reads);
    TEST_ASSERT_TRUE(mark_reads <= 30u);

    unsigned int probes = mark_reads;
    for (uint32_t row = 0; row < rows; row++) {
        TEST_ASSERT_EQUAL_INT(SLOT_BYTES,
                              nd_log_output_port(&instance.resource, log_index,
                                                 record, sizeof(record),
                                                 &info));
        TEST_ASSERT_EQUAL_UINT8(first_slot + row, record[0]);
        TEST_ASSERT_EQUAL_UINT(probes, mark_reads);
        TEST_ASSERT_EQUAL_UINT(row + 1u, slot_reads);
    }
}

static void test_frozen_index_selection_read_counts(void)
{
    /* Sequence marks run 1000 + slot * 10, slots 3..24. */
    run_selection(1, 1030u, 1240u, 3u, 22u, 15u);
}

static void test_frozen_time_selection_read_counts(void)
{
    /* Time marks run 50000 + slot * 60, slots 5..20. */
    run_selection(2, 50300u, 51200u, 5u, 16u, 19u);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_frozen_index_selection_read_counts);
    RUN_TEST(test_frozen_time_selection_read_counts);
    return UNITY_END();
}
