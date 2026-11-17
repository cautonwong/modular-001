#ifndef ND_INFRA_DRIVERS_SECURITY_ESAM_ESAM_H
#define ND_INFRA_DRIVERS_SECURITY_ESAM_ESAM_H

#include <stdint.h>

#define ND_ESAM_MAX_APDU 2000u

/* The board returns this from a receive callback when the supply collapses. */
#define ND_ESAM_BUS_POWER_DOWN (-2)

typedef struct nd_esam_bus {
    void *ctx;
    /* One SPI byte. rx == NULL discards the reply. Returns 0, or
     * ND_ESAM_BUS_POWER_DOWN. */
    int (*transfer)(void *ctx, uint8_t tx, uint8_t *rx);
    void (*cs)(void *ctx, int high);
    /* Power rail and SPI enable. On also waits for the chip to boot. */
    void (*power)(void *ctx, int on);
    void (*delay_us)(void *ctx, uint32_t us);
} nd_esam_bus_t;

typedef struct {
    nd_esam_bus_t bus;
} nd_esam_config_t;

#define ND_ESAM_STORAGE_SIZE 64

typedef union {
    uint64_t align;
    uint8_t bytes[ND_ESAM_STORAGE_SIZE];
} nd_esam_storage_t;

typedef struct nd_esam nd_esam_t;

nd_esam_t *nd_esam_init(nd_esam_storage_t *storage,
                        const nd_esam_config_t *config);
int nd_esam_power_on(nd_esam_t *dev);
int nd_esam_power_off(nd_esam_t *dev);
/* head is the 4-byte command header. rx may alias tx; the frame is sent
 * before the reply is read. Returns the status word, 0 when none arrived. */
int nd_esam_transceive(nd_esam_t *dev, const uint8_t head[4],
                       const uint8_t *tx, uint16_t tx_len,
                       uint8_t *rx, uint16_t *rx_len);
/* 1 when the last receive sequence got a frame within two attempts. */
int nd_esam_link_ok(const nd_esam_t *dev);

#endif
