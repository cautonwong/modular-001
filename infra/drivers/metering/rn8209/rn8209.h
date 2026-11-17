#ifndef ND_INFRA_DRIVERS_METERING_RN8209_RN8209_H
#define ND_INFRA_DRIVERS_METERING_RN8209_RN8209_H

#include <stdint.h>

/* Chip register addresses. */
#define ND_RN8209_SYSCON 0x00u
#define ND_RN8209_EMUCON 0x01u
#define ND_RN8209_HFCONST 0x02u
#define ND_RN8209_PSTART 0x03u
#define ND_RN8209_DSTART 0x04u
#define ND_RN8209_GPQA 0x05u
#define ND_RN8209_GPQB 0x06u
#define ND_RN8209_PHSA 0x07u
#define ND_RN8209_PHSB 0x08u
#define ND_RN8209_APOSA 0x0Au
#define ND_RN8209_APOSB 0x0Bu
#define ND_RN8209_IARMSOS 0x0Eu
#define ND_RN8209_IBRMSOS 0x0Fu
#define ND_RN8209_IBGAIN 0x10u
#define ND_RN8209_EMUCON2 0x17u
#define ND_RN8209_PFCNT 0x20u
#define ND_RN8209_IARMS 0x22u
#define ND_RN8209_IBRMS 0x23u
#define ND_RN8209_URMS 0x24u
#define ND_RN8209_UFREQ 0x25u
#define ND_RN8209_POWERPA 0x26u
#define ND_RN8209_POWERPB 0x27u
#define ND_RN8209_POWERQA 0x28u
#define ND_RN8209_ENERGYP 0x29u
#define ND_RN8209_EMUSTATUS 0x2Du
#define ND_RN8209_CIE 0x40u
#define ND_RN8209_SYSSTATUS 0x43u
#define ND_RN8209_DEVICEID 0x7Fu
#define ND_RN8209_HEAT_CMP 0x51u
#define ND_RN8209_CMD_REG 0xEAu

#define ND_RN8209_CMD_WR_EN 0xE5u
#define ND_RN8209_CMD_WR_DIS 0xDCu
#define ND_RN8209_CMD_RESET 0xFAu

#define ND_RN8209_DEVICE_ID 0x00820900u

/* The board returns this from a bus callback when the supply collapses. */
#define ND_RN8209_BUS_POWER_DOWN (-2)

typedef struct nd_rn8209_bus {
    void *ctx;
    /* Send a frame. hold_tx keeps TX enabled so a read command can keep the
     * line driven until the reply starts. */
    int (*write)(void *ctx, const uint8_t *buf, uint16_t len, int hold_tx);
    int (*read)(void *ctx, uint8_t *buf, uint16_t len);
    void (*delay_us)(void *ctx, uint32_t us);
    /* Board power sequence. power_on also selects the UART and TX line;
     * power_off only resets the chip. */
    void (*reset)(void *ctx, int power_on);
} nd_rn8209_bus_t;

typedef struct {
    nd_rn8209_bus_t bus;
} nd_rn8209_config_t;

#define ND_RN8209_STORAGE_SIZE 64

typedef union {
    uint64_t align;
    uint8_t bytes[ND_RN8209_STORAGE_SIZE];
} nd_rn8209_storage_t;

typedef struct nd_rn8209 nd_rn8209_t;

typedef enum {
    ND_RN8209_CONTROL_POWER_ON = 0,
    ND_RN8209_CONTROL_POWER_OFF,
    ND_RN8209_CONTROL_CHECK,
} nd_rn8209_control_t;

typedef struct {
    uint16_t pstart;
    uint16_t hfconst;
    uint16_t gain;
} nd_rn8209_control_param_t;

nd_rn8209_t *nd_rn8209_init(nd_rn8209_storage_t *storage,
                            const nd_rn8209_config_t *config);
int nd_rn8209_read(nd_rn8209_t *dev, uint8_t reg, void *buf, uint8_t len);
int nd_rn8209_write(nd_rn8209_t *dev, uint8_t reg, const void *buf, uint8_t len);
int nd_rn8209_write_raw(nd_rn8209_t *dev, uint8_t reg, const void *buf,
                        uint8_t len);
int nd_rn8209_command(nd_rn8209_t *dev, uint8_t command);
/* Chip sequences sometimes need a gap the protocol does not encode. */
void nd_rn8209_delay_us(nd_rn8209_t *dev, uint32_t us);
int nd_rn8209_read_id(nd_rn8209_t *dev, uint32_t *id);
int nd_rn8209_control(nd_rn8209_t *dev, nd_rn8209_control_t control,
                      const nd_rn8209_control_param_t *param);

#endif
