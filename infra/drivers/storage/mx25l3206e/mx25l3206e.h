#ifndef ND_INFRA_DRIVERS_STORAGE_MX25L3206E_MX25L3206E_H
#define ND_INFRA_DRIVERS_STORAGE_MX25L3206E_MX25L3206E_H

#include <stdint.h>

#define ND_MX25L3206E_PAGE_SIZE 256u
#define ND_MX25L3206E_SECTOR_SIZE 4096u
/* Datasheet capacity: 32 Mbit. The board asserts its flash size equals it. */
#define ND_MX25L3206E_CAPACITY_BYTES 0x400000u

/* The bus is injected by the board. select(1) drives chip select low for one
 * command; transfer sends tx bytes and then receives rx bytes in that window. */
typedef struct nd_mx25l3206e_bus {
    void *ctx;
    int (*select)(void *ctx, int active);
    int (*transfer)(void *ctx, const uint8_t *tx, uint16_t tx_len,
                    uint8_t *rx, uint16_t rx_len);
    void (*delay_us)(void *ctx, uint32_t us);
} nd_mx25l3206e_bus_t;

typedef struct {
    nd_mx25l3206e_bus_t bus;
    uint32_t size;
} nd_mx25l3206e_config_t;

#define ND_MX25L3206E_STORAGE_SIZE 64

typedef union {
    uint64_t align;
    uint8_t bytes[ND_MX25L3206E_STORAGE_SIZE];
} nd_mx25l3206e_storage_t;

typedef struct nd_mx25l3206e nd_mx25l3206e_t;

nd_mx25l3206e_t *nd_mx25l3206e_init(nd_mx25l3206e_storage_t *storage,
                                    const nd_mx25l3206e_config_t *config);
int nd_mx25l3206e_read_id(nd_mx25l3206e_t *dev, uint8_t id[3]);
int nd_mx25l3206e_read(nd_mx25l3206e_t *dev, uint32_t addr, uint8_t *buf,
                       uint16_t len);
int nd_mx25l3206e_write(nd_mx25l3206e_t *dev, uint32_t addr,
                        const uint8_t *buf, uint16_t len);
int nd_mx25l3206e_erase(nd_mx25l3206e_t *dev, uint32_t addr, uint32_t len);

#endif
