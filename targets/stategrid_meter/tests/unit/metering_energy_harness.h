#ifndef ND_TARGET_STATEGRID_METER_TESTS_UNIT_METERING_ENERGY_HARNESS_H
#define ND_TARGET_STATEGRID_METER_TESTS_UNIT_METERING_ENERGY_HARNESS_H

/*
 * Host rig for the target energy adapter: the EEPROM legacy record reads, the
 * dual-slot snapshot over a fault-injectable RAM block, and the control block
 * the adapter configures from. One static rig per test executable.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "domain/metering/metering.h"
#include "infra/memory/arena.h"
#include "infra/storage/block.h"
#include "infra/storage/block_memory.h"
#include "infra/storage/pulse_snapshot.h"
#include "targets/stategrid_meter/app/metering_energy_adapter.h"
#include "targets/stategrid_meter/app/stategrid_context.h"
#include "targets/stategrid_meter/persistence/legacy_control_block.h"
#include "targets/stategrid_meter/persistence/legacy_energy_record.h"
#include "targets/stategrid_meter/persistence/legacy_energy_store.h"
#include "targets/stategrid_meter/persistence/meter_record.h"

#define HARNESS_EEPROM_BYTES 256u
#define HARNESS_SNAPSHOT_BYTES 1024u
#define HARNESS_SEED_BYTES 1024u
#define HARNESS_METER_CONST 3200u
#define HARNESS_PULSES_PER_STEP 32u
#define HARNESS_SAVE_STEP 30u
#define HARNESS_LEGACY_BLOCK_OFFSET 0x20u
#define HARNESS_LEGACY_OFFSET                                                 \
    (HARNESS_LEGACY_BLOCK_OFFSET + ND_LEGACY_ENERGY_OFFSET_IN_BLOCK)
#define HARNESS_FLAG_OFFSET 0xE0u
#define HARNESS_EC2_ENERGY_INDEX 2000u
#define HARNESS_EC2_MONTH_INDEX 2001u

typedef struct {
    nd_block_ops_t ops;
    nd_block_ops_t *inner;
    int fail_reads;
    int fail_read_range;
    uint32_t read_fail_offset;
    uint32_t read_fail_bytes;
    int fail_writes;
} harness_fault_block_t;

typedef struct {
    uint8_t eeprom[HARNESS_EEPROM_BYTES];
    uint8_t snapshot_memory[HARNESS_SNAPSHOT_BYTES];
    uint8_t seed_memory[HARNESS_SEED_BYTES];
    nd_arena_t seed_arena;
    nd_block_ram_t ram;
    harness_fault_block_t fault;
    nd_pulse_snapshot_storage_t storage;
    nd_pulse_snapshot_config_t config;
    stategrid_context_t instance;
    struct mbase_control_block_t mbase;
} metering_energy_harness_t;

static metering_energy_harness_t harness;

static int harness_block_read(void *ctx, uint32_t offset, uint8_t *buffer,
                              size_t length)
{
    harness_fault_block_t *block = ctx;

    if (block->fail_reads) {
        return -1;
    }
    if (block->fail_read_range
        && offset >= block->read_fail_offset
        && offset + (uint32_t)length
               <= block->read_fail_offset + block->read_fail_bytes) {
        return -1;
    }
    return nd_block_read(block->inner, offset, buffer, length);
}

static int harness_block_program(void *ctx, uint32_t offset,
                                 const uint8_t *buffer, size_t length)
{
    harness_fault_block_t *block = ctx;

    return block->fail_writes ? -1
                              : nd_block_program(block->inner, offset, buffer,
                                                 length);
}

static int harness_block_erase(void *ctx, uint32_t offset, uint32_t length)
{
    harness_fault_block_t *block = ctx;

    return block->fail_writes ? -1
                              : nd_block_erase(block->inner, offset, length);
}

static inline void harness_init(void)
{
    memset(&harness, 0, sizeof(harness));
    memset(harness.eeprom, 0xFF, sizeof(harness.eeprom));
    memset(harness.snapshot_memory, 0xFF, sizeof(harness.snapshot_memory));
    nd_block_ram_init(&harness.ram, harness.snapshot_memory,
                      sizeof(harness.snapshot_memory));
    harness.fault.inner = nd_block_ram_port(&harness.ram);
    harness.fault.ops.ctx = &harness.fault;
    harness.fault.ops.read = harness_block_read;
    harness.fault.ops.program = harness_block_program;
    harness.fault.ops.erase = harness_block_erase;
    harness.fault.ops.size = harness.fault.inner->size;
    harness.fault.ops.erase_unit = harness.fault.inner->erase_unit;
    harness.config.block = &harness.fault.ops;
    harness.config.offset = 0;
    harness.config.blob_bytes = sizeof(legacy_energy_record_t);
    nd_pulse_snapshot_init(&harness.storage, &harness.config);

    nd_mbase_meter_const(&harness.mbase) = HARNESS_METER_CONST;
    nd_mbase_active_energy_cal_word(&harness.mbase) = ND_CAL_ADD_POS;
    nd_mbase_EC_SaveDiff(&harness.mbase) = HARNESS_SAVE_STEP;
    nd_mbase_TOUNo(&harness.mbase) = 1u;
    harness.instance.mbase = &harness.mbase;
    harness.instance.energy_store = nd_pulse_snapshot_port(&harness.storage);
    harness.instance.legacy_energy_offset = HARNESS_LEGACY_OFFSET;
    harness.instance.legacy_energy_flag_offset = HARNESS_FLAG_OFFSET;
    harness.instance.legacy_energy_ec2_energy_index = HARNESS_EC2_ENERGY_INDEX;
    harness.instance.legacy_energy_ec2_month_index = HARNESS_EC2_MONTH_INDEX;
}

/* Re-open the snapshot over the same bytes and drop the in-process book, as a
 * reset would. */
static inline void harness_reboot(void)
{
    metering_energy_adapter_reset();
    nd_pulse_snapshot_init(&harness.storage, &harness.config);
}

static inline nd_energy_ledger_t *harness_bind(void)
{
    return metering_energy_ledger_from_instance(&harness.instance);
}

static inline void harness_seed_snapshot(uint32_t steps)
{
    metering_config_t config = {
        .meter_const = HARNESS_METER_CONST,
        .cal_word = ND_CAL_ADD_POS,
        .save_step = HARNESS_SAVE_STEP,
    };

    nd_arena_init(&harness.seed_arena, harness.seed_memory,
                  sizeof(harness.seed_memory));
    metering_state_t *state =
        metering_create(nd_arena_allocator(&harness.seed_arena), &config);
    if (state == NULL) {
        return;
    }
    metering_add_pulses(state, ND_ENERGY_FORWARD, 1,
                        steps * HARNESS_PULSES_PER_STEP);
    legacy_energy_persist(nd_pulse_snapshot_port(&harness.storage), state);
    metering_destroy(state, nd_arena_allocator(&harness.seed_arena));
}

static inline int harness_read_base(legacy_energy_record_t *record)
{
    size_t length = 0;
    nd_pulse_status_t status = nd_pulse_store_base(
        nd_pulse_snapshot_port(&harness.storage), (uint8_t *)record,
        sizeof(*record), &length);

    return status == ND_PULSE_OK && length == sizeof(*record) ? 0 : -1;
}

int nd_storage_read(uint8_t device_name, uint32_t addr, uint8_t *buf,
                    uint16_t len)
{
    (void)device_name;
    if (addr > sizeof(harness.eeprom)
        || len > sizeof(harness.eeprom) - addr) {
        return -1;
    }
    memcpy(buf, harness.eeprom + addr, len);
    return 0;
}

uint32_t do_checksum(uint8_t *buf, uint16_t len)
{
    uint16_t checksum = 0xA5A5u;

    for (uint16_t index = 0; index < len; index++) {
        checksum = (uint16_t)(checksum + buf[index]);
    }
    return checksum;
}

#endif
