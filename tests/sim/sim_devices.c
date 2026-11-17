#include "sim.h"

#include <stdio.h>

#include "targets/stategrid_meter/app/resource_ids.h"
#include "support/file_store.h"
#include "targets/stategrid_meter/board/board_eeprom.h"
#include "targets/stategrid_meter/board/board_flash.h"
#include "targets/stategrid_meter/board/board_rn8209.h"

static file_store_t eeprom_store;
static file_store_t flash_store;

int nd_storage_read(uint8_t device_name, uint32_t addr, uint8_t *buf,
                    uint16_t len)
{
    if (device_name == RES_DEV_EEPROM) {
        return nd_store_read(file_store_port(&eeprom_store), addr, buf, len);
    }
    if (device_name == RES_DEV_FLASH) {
        return nd_store_read(file_store_port(&flash_store), addr, buf, len);
    }
    return -1;
}

int nd_storage_write(uint8_t device_name, uint32_t addr, const uint8_t *buf,
                     uint16_t len)
{
    if (device_name == RES_DEV_EEPROM) {
        return nd_store_write(file_store_port(&eeprom_store), addr, buf, len);
    }
    if (device_name == RES_DEV_FLASH) {
        return nd_store_write(file_store_port(&flash_store), addr, buf, len);
    }
    return -1;
}

int nd_storage_erase(uint8_t device_name, uint32_t addr)
{
    /* The old sim control stub ignored erase; the image must stay untouched. */
    (void)device_name;
    (void)addr;
    return 0;
}

uint32_t nd_storage_size(uint8_t device_name)
{
    if (device_name == RES_DEV_EEPROM) {
        return BOARD_EEPROM_SIZE;
    }
    if (device_name == RES_DEV_FLASH) {
        return BOARD_FLASH_SIZE;
    }
    return 0;
}

static uint32_t sim_rn8209_regs[0x80];
static uint8_t sim_rn8209_pending;
static int sim_meter_chip_fault = 1;

void sim_meter_set_chip_fault(int fault)
{
    sim_meter_chip_fault = fault;
}

static int sim_rn8209_bus_write(void *ctx, const uint8_t *buf, uint16_t len,
                                int hold_tx)
{
    uint32_t value = 0;
    uint8_t reg;
    uint16_t i;

    (void)ctx;
    (void)hold_tx;
    if (len == 1 && (buf[0] & 0x80u) == 0u) {
        sim_rn8209_pending = (uint8_t)(buf[0] & 0x7Fu);
        return 0;
    }
    if (len < 2) {
        return -1;
    }
    reg = (uint8_t)(buf[0] & 0x7Fu);
    for (i = 1; i + 1 < len; i++) {
        value = (value << 8) | buf[i];
    }
    sim_rn8209_regs[reg] = value;
    return 0;
}

static int sim_rn8209_bus_read(void *ctx, uint8_t *buf, uint16_t len)
{
    uint16_t payload = (uint16_t)(len - 1u);
    uint32_t value;
    uint8_t sum;
    uint16_t i;

    (void)ctx;
    if (payload == 0u || payload > 4u) {
        return -1;
    }
    value = sim_rn8209_regs[sim_rn8209_pending];
    if (sim_rn8209_pending == 0x7Fu) {
        value = sim_meter_chip_fault ? 0u : 0x00820900u;
    }
    for (i = 0; i < payload; i++) {
        buf[i] = (uint8_t)(value >> (8u * (payload - 1u - i)));
    }
    sum = sim_rn8209_pending;
    for (i = 0; i < payload; i++) {
        sum = (uint8_t)(sum + buf[i]);
    }
    buf[payload] = (uint8_t)~sum;
    return 0;
}

static void sim_rn8209_bus_delay(void *ctx, uint32_t us)
{
    (void)ctx;
    (void)us;
}

static void sim_rn8209_bus_reset(void *ctx, int power_on)
{
    (void)ctx;
    (void)power_on;
}

static nd_rn8209_storage_t sim_rn8209_storage;
static nd_rn8209_t *sim_rn8209;

static const nd_rn8209_config_t sim_rn8209_config = {
    .bus = {
        .ctx = NULL,
        .write = sim_rn8209_bus_write,
        .read = sim_rn8209_bus_read,
        .delay_us = sim_rn8209_bus_delay,
        .reset = sim_rn8209_bus_reset,
    },
};

nd_rn8209_t *board_rn8209(void)
{
    if (sim_rn8209 == NULL) {
        sim_rn8209_regs[ND_RN8209_SYSCON] = 0x1643u;
        sim_rn8209_regs[ND_RN8209_EMUCON] = 0x0001u;
        sim_rn8209_regs[ND_RN8209_HFCONST] = 0x0DB4u;
        sim_rn8209_regs[ND_RN8209_GPQA] = 0xFFFFu;
        sim_rn8209_regs[ND_RN8209_CIE] = 0x01u;
        sim_rn8209 = nd_rn8209_init(&sim_rn8209_storage, &sim_rn8209_config);
    }
    return sim_rn8209;
}

int sim_devices_init(void)
{
    if (file_store_open(&eeprom_store, g_sim.eeprom_path, BOARD_EEPROM_SIZE, 0xFF) != 0) {
        fprintf(stderr, "firmware_sim: cannot open EEPROM image %s\n",
                g_sim.eeprom_path);
        return -1;
    }
    if (file_store_open(&flash_store, g_sim.flash_path, BOARD_FLASH_SIZE, 0xFF) != 0) {
        fprintf(stderr, "firmware_sim: cannot open flash image %s\n",
                g_sim.flash_path);
        file_store_close(&eeprom_store);
        return -1;
    }
    return 0;
}

void sim_devices_close(void)
{
    file_store_close(&flash_store);
    file_store_close(&eeprom_store);
}
