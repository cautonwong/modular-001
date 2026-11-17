#include "infra/drivers/metering/rn8209/rn8209.h"

#include <stddef.h>

struct nd_rn8209 {
    nd_rn8209_config_t config;
};

_Static_assert(sizeof(struct nd_rn8209) <= sizeof(nd_rn8209_storage_t),
               "rn8209 storage too small");

#define ND_RN8209_READ_ATTEMPTS 3u
#define ND_RN8209_WRITE_ATTEMPTS 10u
#define ND_RN8209_MAX_PAYLOAD 4u
#define ND_RN8209_DEFAULT_SYSCON 0x1643u
#define ND_RN8209_DEFAULT_EMUCON 0x0001u
#define ND_RN8209_DEFAULT_CIE 0x01u

static void bus_delay(nd_rn8209_t *dev, uint32_t us)
{
    if (dev->config.bus.delay_us != NULL) {
        dev->config.bus.delay_us(dev->config.bus.ctx, us);
    }
}

static int payload_matches(const uint8_t *left, const void *right, uint8_t len)
{
    const uint8_t *bytes = (const uint8_t *)right;
    uint8_t i;

    for (i = 0; i < len; i++) {
        if (left[i] != bytes[i]) {
            return 0;
        }
    }
    return 1;
}

static int raw_write(nd_rn8209_t *dev, uint8_t reg, const void *buf,
                     uint8_t len)
{
    uint8_t frame[ND_RN8209_MAX_PAYLOAD + 2u];
    const uint8_t *bytes = (const uint8_t *)buf;
    uint8_t sum = 0;
    uint8_t summed = (uint8_t)(len + 1u);
    uint8_t i;

    frame[0] = (uint8_t)(reg | 0x80u);
    for (i = 0; i < len; i++) {
        frame[1u + i] = bytes[len - 1u - i];
    }
    for (i = 0; i < summed; i++) {
        sum = (uint8_t)(sum + frame[i]);
    }
    frame[len + 1u] = (uint8_t)~sum;

    return dev->config.bus.write(dev->config.bus.ctx, frame,
                                 (uint16_t)(len + 2u), 0) == 0
               ? 0
               : -1;
}

nd_rn8209_t *nd_rn8209_init(nd_rn8209_storage_t *storage,
                            const nd_rn8209_config_t *config)
{
    struct nd_rn8209 *dev = (struct nd_rn8209 *)storage;

    if (dev == NULL || config == NULL || config->bus.write == NULL
        || config->bus.read == NULL) {
        return NULL;
    }
    dev->config = *config;
    return dev;
}

int nd_rn8209_read(nd_rn8209_t *dev, uint8_t reg, void *buf, uint8_t len)
{
    uint8_t reply[ND_RN8209_MAX_PAYLOAD + 1u] = {0};
    uint8_t *out = (uint8_t *)buf;
    uint8_t attempt;

    if (dev == NULL || buf == NULL || len == 0 || len > ND_RN8209_MAX_PAYLOAD) {
        return -1;
    }
    for (attempt = 0; attempt < ND_RN8209_READ_ATTEMPTS; attempt++) {
        uint8_t sum = reg;
        uint8_t i;
        int result;

        bus_delay(dev, 500);
        result = dev->config.bus.write(dev->config.bus.ctx, &reg, 1, 1);
        if (result == ND_RN8209_BUS_POWER_DOWN) {
            return ND_RN8209_BUS_POWER_DOWN;
        }
        if (result != 0) {
            continue;
        }
        result = dev->config.bus.read(dev->config.bus.ctx, reply,
                                      (uint16_t)(len + 1u));
        if (result == ND_RN8209_BUS_POWER_DOWN) {
            return ND_RN8209_BUS_POWER_DOWN;
        }
        if (result != 0) {
            continue;
        }
        for (i = 0; i < len; i++) {
            sum = (uint8_t)(sum + reply[i]);
        }
        if ((uint8_t)~sum != reply[len]) {
            continue;
        }
        for (i = 0; i < len; i++) {
            out[i] = reply[len - 1u - i];
        }
        return 0;
    }
    return -1;
}

int nd_rn8209_write_raw(nd_rn8209_t *dev, uint8_t reg, const void *buf,
                        uint8_t len)
{
    if (dev == NULL || buf == NULL || len == 0 || len > ND_RN8209_MAX_PAYLOAD) {
        return -1;
    }
    return raw_write(dev, reg, buf, len);
}

int nd_rn8209_write(nd_rn8209_t *dev, uint8_t reg, const void *buf, uint8_t len)
{
    uint8_t verify[ND_RN8209_MAX_PAYLOAD] = {0};
    uint8_t attempt;

    if (dev == NULL || buf == NULL || len == 0 || len > ND_RN8209_MAX_PAYLOAD) {
        return -1;
    }
    for (attempt = 0; attempt < ND_RN8209_WRITE_ATTEMPTS; attempt++) {
        if (raw_write(dev, reg, buf, len) == 0
            && nd_rn8209_read(dev, reg, verify, len) == 0
            && payload_matches(verify, buf, len)) {
            return 0;
        }
        bus_delay(dev, 5000);
    }
    return -1;
}

int nd_rn8209_command(nd_rn8209_t *dev, uint8_t command)
{
    int result;

    if (dev == NULL) {
        return -1;
    }
    bus_delay(dev, 5000);
    result = raw_write(dev, ND_RN8209_CMD_REG, &command, 1);
    bus_delay(dev, 5000);
    return result;
}

void nd_rn8209_delay_us(nd_rn8209_t *dev, uint32_t us)
{
    if (dev != NULL) {
        bus_delay(dev, us);
    }
}

int nd_rn8209_read_id(nd_rn8209_t *dev, uint32_t *id)
{
    uint32_t value = 0;

    if (dev == NULL || id == NULL) {
        return -1;
    }
    if (nd_rn8209_read(dev, ND_RN8209_DEVICEID, &value, 3) != 0
        || value != ND_RN8209_DEVICE_ID) {
        return -1;
    }
    *id = value;
    return 0;
}

static int write_config(nd_rn8209_t *dev, uint8_t reg, uint16_t value)
{
    return nd_rn8209_write(dev, reg, &value, 2);
}

static int check_registers(nd_rn8209_t *dev,
                           const nd_rn8209_control_param_t *param)
{
    static const uint8_t registers[] = {
        ND_RN8209_SYSCON,
        ND_RN8209_EMUCON,
        ND_RN8209_HFCONST,
        ND_RN8209_GPQA,
    };
    const uint16_t expected[] = {
        ND_RN8209_DEFAULT_SYSCON,
        ND_RN8209_DEFAULT_EMUCON,
        param->hfconst,
        param->gain,
    };
    uint8_t count = (uint8_t)(sizeof(registers) / sizeof(registers[0]));
    uint8_t i;

    for (i = 0; i < count; i++) {
        uint16_t actual = 0;
        int result = nd_rn8209_read(dev, registers[i], &actual, 2);

        if (result == ND_RN8209_BUS_POWER_DOWN) {
            /* The old check treated a collapsing supply as healthy so the
             * caller would not reset the chip while power is failing. */
            return 0;
        }
        if (result != 0 || actual != expected[i]) {
            return -1;
        }
    }
    return 0;
}

int nd_rn8209_control(nd_rn8209_t *dev, nd_rn8209_control_t control,
                      const nd_rn8209_control_param_t *param)
{
    uint8_t cie = ND_RN8209_DEFAULT_CIE;
    uint32_t id = 0;

    if (dev == NULL) {
        return -1;
    }
    switch (control) {
    case ND_RN8209_CONTROL_POWER_ON:
        if (param == NULL) {
            return -1;
        }
        if (dev->config.bus.reset != NULL) {
            dev->config.bus.reset(dev->config.bus.ctx, 1);
        }
        if (nd_rn8209_read_id(dev, &id) != 0) {
            return -1;
        }
        (void)nd_rn8209_command(dev, ND_RN8209_CMD_WR_EN);
        /* Chip presence is the health signal. A failed setup write stops the
         * sequence and leaves the status clean, matching the old control. */
        if (write_config(dev, ND_RN8209_SYSCON, ND_RN8209_DEFAULT_SYSCON) != 0
            || write_config(dev, ND_RN8209_EMUCON,
                            ND_RN8209_DEFAULT_EMUCON) != 0
            || write_config(dev, ND_RN8209_HFCONST, param->hfconst) != 0
            || write_config(dev, ND_RN8209_PSTART, param->pstart) != 0
            || nd_rn8209_write(dev, ND_RN8209_CIE, &cie, 1) != 0) {
            return 0;
        }
        (void)nd_rn8209_command(dev, ND_RN8209_CMD_WR_DIS);
        return 0;
    case ND_RN8209_CONTROL_POWER_OFF:
        if (dev->config.bus.reset != NULL) {
            dev->config.bus.reset(dev->config.bus.ctx, 0);
        }
        return 0;
    case ND_RN8209_CONTROL_CHECK:
        if (param == NULL) {
            return -1;
        }
        return check_registers(dev, param);
    default:
        return -1;
    }
}
