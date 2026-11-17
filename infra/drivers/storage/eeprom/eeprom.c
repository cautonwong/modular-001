#include "infra/drivers/storage/eeprom/eeprom.h"

#include <stddef.h>

struct nd_eeprom {
    nd_eeprom_config_t config;
};

_Static_assert(sizeof(struct nd_eeprom) <= sizeof(nd_eeprom_storage_t),
               "eeprom storage too small");

#define ND_EEPROM_RETRIES 8u

static int read_once(nd_eeprom_t *dev, uint16_t addr, uint8_t *buf,
                     uint16_t len)
{
    return dev->config.bus.read(dev->config.bus.ctx, dev->config.device_address,
                                addr, buf, len);
}

static int matches(nd_eeprom_t *dev, uint16_t addr, const uint8_t *buf,
                   uint16_t len)
{
    uint8_t probe[32];
    uint16_t done = 0;

    while (done < len) {
        uint16_t chunk = (uint16_t)(len - done);
        uint16_t i;

        if (chunk > sizeof(probe)) {
            chunk = (uint16_t)sizeof(probe);
        }
        if (read_once(dev, (uint16_t)(addr + done), probe, chunk) != 0) {
            return 0;
        }
        for (i = 0; i < chunk; i++) {
            if (probe[i] != buf[done + i]) {
                return 0;
            }
        }
        done = (uint16_t)(done + chunk);
    }
    return 1;
}

static int write_pages(nd_eeprom_t *dev, uint16_t addr, const uint8_t *buf,
                       uint16_t len)
{
    while (len > 0) {
        uint16_t chunk = (uint16_t)(dev->config.page_size
                                    - (addr & (dev->config.page_size - 1u)));
        if (chunk > len) {
            chunk = len;
        }
        if (dev->config.bus.write(dev->config.bus.ctx,
                                  dev->config.device_address, addr, buf,
                                  chunk) != 0) {
            return -1;
        }
        addr = (uint16_t)(addr + chunk);
        buf += chunk;
        len = (uint16_t)(len - chunk);
    }
    return 0;
}

nd_eeprom_t *nd_eeprom_init(nd_eeprom_storage_t *storage,
                            const nd_eeprom_config_t *config)
{
    struct nd_eeprom *dev = (struct nd_eeprom *)storage;

    if (dev == NULL || config == NULL || config->bus.read == NULL
        || config->bus.write == NULL || config->page_size == 0
        || (config->page_size & (config->page_size - 1u)) != 0) {
        return NULL;
    }
    dev->config = *config;
    return dev;
}

int nd_eeprom_read(nd_eeprom_t *dev, uint32_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t attempt;

    if (dev == NULL || buf == NULL || addr > dev->config.size
        || len > dev->config.size - addr) {
        return -1;
    }
    for (attempt = 0; attempt < ND_EEPROM_RETRIES; attempt++) {
        if (read_once(dev, (uint16_t)addr, buf, len) == 0) {
            return 0;
        }
    }
    return -1;
}

int nd_eeprom_write(nd_eeprom_t *dev, uint32_t addr, const uint8_t *buf,
                    uint16_t len)
{
    uint8_t attempt;

    if (dev == NULL || buf == NULL || addr > dev->config.size
        || len > dev->config.size - addr) {
        return -1;
    }
    for (attempt = 0; attempt < ND_EEPROM_RETRIES; attempt++) {
        if (matches(dev, (uint16_t)addr, buf, len)) {
            return 0;
        }
        if (write_pages(dev, (uint16_t)addr, buf, len) != 0) {
            continue;
        }
        if (dev->config.bus.delay_us != NULL) {
            dev->config.bus.delay_us(dev->config.bus.ctx, 3000);
        }
        if (matches(dev, (uint16_t)addr, buf, len)) {
            return 0;
        }
    }
    return -1;
}
