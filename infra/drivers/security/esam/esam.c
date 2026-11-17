#include "infra/drivers/security/esam/esam.h"

#include <stddef.h>

#define ND_ESAM_ATTEMPTS 3
#define ND_ESAM_TIMEOUT 2000
#define ND_ESAM_CS_SETTLE_US 120u
#define ND_ESAM_TX_HOLD_US 15u
#define ND_ESAM_CS_HOLD_US 55u
#define ND_ESAM_RETRY_GAP_US 50u
#define ND_ESAM_REPLY_HEADER 0x90u
#define ND_ESAM_REPLY_HEADER_LOW 0x00u

struct nd_esam {
    nd_esam_config_t config;
    uint8_t link_ok;
};

_Static_assert(sizeof(struct nd_esam) <= sizeof(nd_esam_storage_t),
               "esam storage too small");

static void bus_delay(nd_esam_t *dev, uint32_t us)
{
    dev->config.bus.delay_us(dev->config.bus.ctx, us);
}

static void chip_select(nd_esam_t *dev, int high)
{
    dev->config.bus.cs(dev->config.bus.ctx, high);
}

static void send_byte(nd_esam_t *dev, uint8_t byte)
{
    (void)dev->config.bus.transfer(dev->config.bus.ctx, byte, NULL);
}

static int receive_byte(nd_esam_t *dev, uint8_t *byte)
{
    return dev->config.bus.transfer(dev->config.bus.ctx, 0, byte);
}

nd_esam_t *nd_esam_init(nd_esam_storage_t *storage,
                        const nd_esam_config_t *config)
{
    struct nd_esam *dev = (struct nd_esam *)storage;

    if (dev == NULL || config == NULL || config->bus.transfer == NULL
        || config->bus.cs == NULL || config->bus.power == NULL
        || config->bus.delay_us == NULL) {
        return NULL;
    }
    dev->config = *config;
    dev->link_ok = 1;
    return dev;
}

int nd_esam_power_on(nd_esam_t *dev)
{
    if (dev == NULL) {
        return -1;
    }
    chip_select(dev, 1);
    dev->config.bus.power(dev->config.bus.ctx, 1);
    return 0;
}

int nd_esam_power_off(nd_esam_t *dev)
{
    if (dev == NULL) {
        return -1;
    }
    dev->config.bus.power(dev->config.bus.ctx, 0);
    return 0;
}

int nd_esam_transceive(nd_esam_t *dev, const uint8_t head[4],
                       const uint8_t *tx, uint16_t tx_len,
                       uint8_t *rx, uint16_t *rx_len)
{
    uint8_t header[4] = {0};
    uint8_t busy = 0;
    uint8_t lrc;
    uint16_t i;
    uint16_t sw;
    int attempt;

    if (dev == NULL || head == NULL || rx == NULL || rx_len == NULL
        || (tx == NULL && tx_len != 0u)) {
        return 0;
    }

    chip_select(dev, 0);
    bus_delay(dev, ND_ESAM_CS_SETTLE_US);

    send_byte(dev, 0x55u);
    lrc = (uint8_t)(head[0] ^ head[1] ^ head[2] ^ head[3]);
    for (i = 0; i < 4u; i++) {
        send_byte(dev, head[i]);
    }
    lrc = (uint8_t)(lrc ^ (uint8_t)(tx_len >> 8));
    lrc = (uint8_t)(lrc ^ (uint8_t)tx_len);
    send_byte(dev, (uint8_t)(tx_len >> 8));
    send_byte(dev, (uint8_t)tx_len);
    for (i = 0; i < tx_len; i++) {
        send_byte(dev, tx[i]);
        lrc = (uint8_t)(lrc ^ tx[i]);
    }
    send_byte(dev, (uint8_t)~lrc);
    bus_delay(dev, ND_ESAM_TX_HOLD_US);
    chip_select(dev, 1);
    bus_delay(dev, ND_ESAM_CS_HOLD_US);

    for (attempt = 0; attempt < ND_ESAM_ATTEMPTS; attempt++) {
        int timeout = ND_ESAM_TIMEOUT;
        int err = 1;

        chip_select(dev, 0);
        bus_delay(dev, ND_ESAM_CS_SETTLE_US);
        while (busy != 0x55u && timeout-- > 0) {
            if (receive_byte(dev, &busy) == ND_ESAM_BUS_POWER_DOWN) {
                return 0;
            }
            if (busy == 0x55u) {
                uint8_t tmp;
                uint16_t count;

                for (i = 0; i < 4u; i++) {
                    if (receive_byte(dev, &header[i])
                        == ND_ESAM_BUS_POWER_DOWN) {
                        return 0;
                    }
                }
                if (header[0] == ND_ESAM_REPLY_HEADER
                    && header[1] == ND_ESAM_REPLY_HEADER_LOW) {
                    lrc = (uint8_t)(header[0] ^ header[1] ^ header[2]
                                    ^ header[3]);
                    count = (uint16_t)(((uint16_t)header[2] << 8)
                                       | header[3]);
                    if (count > ND_ESAM_MAX_APDU) {
                        count = ND_ESAM_MAX_APDU;
                    }
                    *rx_len = count;
                    for (i = 0; i < count; i++) {
                        if (receive_byte(dev, &rx[i])
                            == ND_ESAM_BUS_POWER_DOWN) {
                            return 0;
                        }
                        lrc = (uint8_t)(lrc ^ rx[i]);
                    }
                    lrc = (uint8_t)~lrc;
                    if (receive_byte(dev, &tmp)
                        == ND_ESAM_BUS_POWER_DOWN) {
                        return 0;
                    }
                    err = lrc != tmp;
                    break;
                }
            }
            bus_delay(dev, ND_ESAM_CS_SETTLE_US);
        }
        chip_select(dev, 1);
        bus_delay(dev, ND_ESAM_RETRY_GAP_US);
        dev->link_ok = (uint8_t)(attempt + 1 < ND_ESAM_ATTEMPTS);
        if (!err) {
            break;
        }
    }

    sw = (uint16_t)(((uint16_t)header[0] << 8) | header[1]);
    return (int)sw;
}

int nd_esam_link_ok(const nd_esam_t *dev)
{
    return dev != NULL ? dev->link_ok : 0;
}
