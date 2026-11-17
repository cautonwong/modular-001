#include "infra/drivers/storage/mx25l3206e/mx25l3206e.h"

#include <stddef.h>

#define ND_MX_CMD_WREN 0x06u
#define ND_MX_CMD_RDSR 0x05u
#define ND_MX_CMD_READ 0x03u
#define ND_MX_CMD_PP 0x02u
#define ND_MX_CMD_SE 0x20u
#define ND_MX_CMD_RDID 0x9Fu

#define ND_MX_STATUS_BUSY 0x01u
#define ND_MX_STATUS_POLLS 2000u
#define ND_MX_VERIFY_CHUNK 32u

struct nd_mx25l3206e {
    nd_mx25l3206e_config_t config;
};

_Static_assert(sizeof(struct nd_mx25l3206e) <= sizeof(nd_mx25l3206e_storage_t),
               "mx25l3206e storage too small");

static int in_range(const nd_mx25l3206e_t *dev, uint32_t addr, uint32_t len)
{
    return dev != NULL && addr <= dev->config.size
           && len <= dev->config.size - addr;
}

static int select_chip(nd_mx25l3206e_t *dev, int active)
{
    return dev->config.bus.select(dev->config.bus.ctx, active);
}

static int transfer(nd_mx25l3206e_t *dev, const uint8_t *tx, uint16_t tx_len,
                    uint8_t *rx, uint16_t rx_len)
{
    return dev->config.bus.transfer(dev->config.bus.ctx, tx, tx_len, rx, rx_len);
}

static int command(nd_mx25l3206e_t *dev, const uint8_t *tx, uint16_t tx_len,
                   uint8_t *rx, uint16_t rx_len)
{
    int result;

    if (select_chip(dev, 1) != 0) {
        return -1;
    }
    result = transfer(dev, tx, tx_len, rx, rx_len);
    if (select_chip(dev, 0) != 0) {
        result = -1;
    }
    return result;
}

static void address_command(uint8_t out[4], uint8_t opcode, uint32_t addr)
{
    out[0] = opcode;
    out[1] = (uint8_t)(addr >> 16);
    out[2] = (uint8_t)(addr >> 8);
    out[3] = (uint8_t)addr;
}

static int status_idle(nd_mx25l3206e_t *dev)
{
    uint8_t opcode = ND_MX_CMD_RDSR;
    uint8_t status = 0xFFu;

    if (command(dev, &opcode, 1, &status, 1) != 0) {
        return 0;
    }
    return (status & ND_MX_STATUS_BUSY) == 0;
}

static int wait_ready(nd_mx25l3206e_t *dev)
{
    uint16_t pulses;

    /* Two consecutive idle reads keep the legacy debounce. */
    for (pulses = 0; pulses < ND_MX_STATUS_POLLS; pulses++) {
        if (status_idle(dev) && status_idle(dev)) {
            return 0;
        }
    }
    return -1;
}

static int write_enable(nd_mx25l3206e_t *dev)
{
    uint8_t opcode = ND_MX_CMD_WREN;

    if (command(dev, &opcode, 1, NULL, 0) != 0) {
        return -1;
    }
    if (dev->config.bus.delay_us != NULL) {
        dev->config.bus.delay_us(dev->config.bus.ctx, 10);
    }
    return 0;
}

nd_mx25l3206e_t *nd_mx25l3206e_init(nd_mx25l3206e_storage_t *storage,
                                    const nd_mx25l3206e_config_t *config)
{
    struct nd_mx25l3206e *dev = (struct nd_mx25l3206e *)storage;

    if (dev == NULL || config == NULL || config->bus.select == NULL
        || config->bus.transfer == NULL || config->size == 0) {
        return NULL;
    }
    dev->config = *config;
    return dev;
}

int nd_mx25l3206e_read_id(nd_mx25l3206e_t *dev, uint8_t id[3])
{
    uint8_t opcode = ND_MX_CMD_RDID;

    if (dev == NULL || id == NULL) {
        return -1;
    }
    return command(dev, &opcode, 1, id, 3);
}

int nd_mx25l3206e_read(nd_mx25l3206e_t *dev, uint32_t addr, uint8_t *buf,
                       uint16_t len)
{
    uint8_t cmd[4];

    if (buf == NULL || !in_range(dev, addr, len)) {
        return -1;
    }
    address_command(cmd, ND_MX_CMD_READ, addr);
    return command(dev, cmd, 4, buf, len);
}

static int verify(nd_mx25l3206e_t *dev, uint32_t addr, const uint8_t *buf,
                  uint16_t len)
{
    uint8_t probe[ND_MX_VERIFY_CHUNK];
    uint16_t done = 0;

    while (done < len) {
        uint16_t chunk = (uint16_t)(len - done);
        uint16_t i;

        if (chunk > sizeof(probe)) {
            chunk = (uint16_t)sizeof(probe);
        }
        if (nd_mx25l3206e_read(dev, addr + done, probe, chunk) != 0) {
            return -1;
        }
        for (i = 0; i < chunk; i++) {
            if (probe[i] != buf[done + i]) {
                return -1;
            }
        }
        done = (uint16_t)(done + chunk);
    }
    return 0;
}

int nd_mx25l3206e_write(nd_mx25l3206e_t *dev, uint32_t addr,
                        const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || !in_range(dev, addr, len)) {
        return -1;
    }
    while (len > 0) {
        uint16_t chunk = (uint16_t)(ND_MX25L3206E_PAGE_SIZE
                                    - (addr & (ND_MX25L3206E_PAGE_SIZE - 1u)));
        uint8_t cmd[4];
        int result;

        if (chunk > len) {
            chunk = len;
        }
        if (write_enable(dev) != 0) {
            return -1;
        }
        address_command(cmd, ND_MX_CMD_PP, addr);
        if (select_chip(dev, 1) != 0) {
            return -1;
        }
        result = transfer(dev, cmd, 4, NULL, 0);
        if (result == 0) {
            result = transfer(dev, buf, chunk, NULL, 0);
        }
        if (select_chip(dev, 0) != 0) {
            result = -1;
        }
        if (result != 0) {
            return -1;
        }
        if (dev->config.bus.delay_us != NULL) {
            dev->config.bus.delay_us(dev->config.bus.ctx, 10);
        }
        if (wait_ready(dev) != 0 || verify(dev, addr, buf, chunk) != 0) {
            return -1;
        }
        addr += chunk;
        buf += chunk;
        len = (uint16_t)(len - chunk);
    }
    return 0;
}

int nd_mx25l3206e_erase(nd_mx25l3206e_t *dev, uint32_t addr, uint32_t len)
{
    uint32_t current;
    uint32_t end;

    if (!in_range(dev, addr, len) || len == 0) {
        return -1;
    }
    current = addr & ~(ND_MX25L3206E_SECTOR_SIZE - 1u);
    end = addr + len;
    while (current < end) {
        uint8_t cmd[4];

        if (write_enable(dev) != 0) {
            return -1;
        }
        address_command(cmd, ND_MX_CMD_SE, current);
        if (command(dev, cmd, 4, NULL, 0) != 0 || wait_ready(dev) != 0) {
            return -1;
        }
        current += ND_MX25L3206E_SECTOR_SIZE;
    }
    return 0;
}
