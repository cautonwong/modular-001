#include "sim.h"

#include <stdio.h>
#include <string.h>

#include "nidavellir.h"
#include "infra/kernel/resource_blocks.h"
#include "support/file_store.h"

static file_store_t eeprom_store;
static file_store_t flash_store;

extern const struct dev_info_t eeprom_dev;
extern const struct dev_info_t nor_flash_dev;

static int sim_eeprom_initialize(struct instance_t *instance,
                                 const struct dev_info_t *dev)
{
    (void)instance;
    (void)dev;
    return 0;
}

static int sim_eeprom_control(struct instance_t *instance,
                              const struct dev_info_t *dev, uint8_t ctrl,
                              void *ctrl_parameter)
{
    (void)instance;
    (void)dev;
    (void)ctrl;
    (void)ctrl_parameter;
    return 0;
}

static int sim_eeprom_read(struct instance_t *instance,
                           const struct dev_info_t *dev, uint32_t addr,
                           uint16_t len, void *src)
{
    (void)instance;
    if (dev == &eeprom_dev) {
        return nd_store_read(file_store_port(&eeprom_store), addr, src, len);
    }
    return nd_store_read(file_store_port(&flash_store), addr, src, len);
}

static int sim_eeprom_write(struct instance_t *instance,
                            const struct dev_info_t *dev, uint32_t addr,
                            uint16_t len, void *src)
{
    (void)instance;
    if (dev == &eeprom_dev) {
        return nd_store_write(file_store_port(&eeprom_store), addr, src, len);
    }
    return nd_store_write(file_store_port(&flash_store), addr, src, len);
}

static const struct driver_ops_t sim_file_ops = {
    sim_eeprom_initialize,
    sim_eeprom_control,
    sim_eeprom_read,
    sim_eeprom_write,
};

const struct dev_info_t eeprom_dev = {
    DEV_EEPROM,
    1,
    0xFFFF,
    0,
    0,
    NULL,
    &sim_file_ops,
    NULL,
    NULL,
    NULL,
    NULL,
};

const struct dev_info_t nor_flash_dev = {
    DEV_FLASH,
    0,
    0x7FFFFF,
    0,
    0,
    NULL,
    &sim_file_ops,
    NULL,
    NULL,
    NULL,
    NULL,
};

static int sim_rn8209_initialize(struct instance_t *instance,
                                 const struct dev_info_t *dev)
{
    (void)instance;
    (void)dev;
    return 0;
}

static int sim_rn8209_control(struct instance_t *instance,
                              const struct dev_info_t *dev, uint8_t ctrl,
                              void *ctrl_parameter)
{
    (void)instance;
    (void)dev;
    (void)ctrl;
    (void)ctrl_parameter;
    /* The app treats a non-zero result from METERCHIP_CHK as a healthy
     * metering chip; all other control requests are accepted as no-ops. */
    return 1;
}

static int sim_meter_chip_fault = 1;

void sim_meter_set_chip_fault(int fault)
{
    sim_meter_chip_fault = fault;
}

static int sim_rn8209_read(struct instance_t *instance,
                           const struct dev_info_t *dev, uint32_t addr,
                           uint16_t len, void *src)
{
    (void)instance;
    (void)dev;
    memset(src, 0, len);
    if (addr == 0x7F && len >= 3 && !sim_meter_chip_fault) {
        uint32_t id = 0x00820900;
        memcpy(src, &id, 3);
    }
    return len;
}

static int sim_rn8209_write(struct instance_t *instance,
                            const struct dev_info_t *dev, uint32_t addr,
                            uint16_t len, void *src)
{
    (void)instance;
    (void)dev;
    (void)addr;
    (void)src;
    return len;
}

static const struct driver_ops_t sim_rn8209_ops = {
    sim_rn8209_initialize,
    sim_rn8209_control,
    sim_rn8209_read,
    sim_rn8209_write,
};

const struct dev_info_t rn8209_dev = {
    0,
    0,
    0,
    0,
    0,
    NULL,
    &sim_rn8209_ops,
    NULL,
    NULL,
    NULL,
    NULL,
};

int sim_devices_init(void)
{
    if (file_store_open(&eeprom_store, g_sim.eeprom_path, 0xFFFF, 0xFF) != 0) {
        fprintf(stderr, "firmware_sim: cannot open EEPROM image %s\n",
                g_sim.eeprom_path);
        return -1;
    }
    if (file_store_open(&flash_store, g_sim.flash_path, 0x7FFFFF, 0xFF) != 0) {
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
