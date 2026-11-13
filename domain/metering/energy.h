#ifndef ND_DOMAIN_METERING_ENERGY_H
#define ND_DOMAIN_METERING_ENERGY_H

#include <stdint.h>

#define ND_TARIFF_SLOTS 13
#define ND_TARIFF_TOTAL 0

#define ND_CAL_ADD_POS 0x80u
#define ND_CAL_DEC_POS 0x40u
#define ND_CAL_ADD_NEG 0x20u
#define ND_CAL_DEC_NEG 0x10u

typedef enum {
    ND_ENERGY_FORWARD = 0,
    ND_ENERGY_REVERSE = 1
} nd_energy_direction_t;

typedef enum {
    ND_ENERGY_RES_0_01_KWH = 0,
    ND_ENERGY_RES_0_0001_KWH = 1,
    ND_ENERGY_RES_1_KWH = 2
} nd_energy_resolution_t;

/* integer is hundredths of a kWh, fraction is pulses. */
typedef struct {
    uint32_t integer;
    uint16_t fraction;
} nd_energy_amount_t;

typedef struct {
    nd_energy_amount_t slots[ND_TARIFF_SLOTS];
} nd_energy_channel_t;

typedef struct {
    nd_energy_channel_t forward;
    nd_energy_channel_t reverse;
    uint32_t meter_const;
    uint8_t cal_word;
    uint32_t save_step;
    nd_energy_amount_t month;
} nd_energy_book_t;

typedef struct {
    int energy_changed;
    int month_changed;
    int energy_save_due;
    int month_save_due;
} nd_energy_update_t;

void nd_energy_book_init(nd_energy_book_t *book, uint32_t meter_const,
                         uint8_t cal_word, uint32_t save_step);

nd_energy_update_t nd_energy_add_pulses(nd_energy_book_t *book,
                                        nd_energy_direction_t direction,
                                        uint8_t tariff, uint32_t pulses);

int64_t nd_energy_read_amount(const nd_energy_book_t *book,
                              const nd_energy_amount_t *amount,
                              nd_energy_resolution_t resolution);

int64_t nd_energy_read(const nd_energy_book_t *book,
                       nd_energy_direction_t direction, uint8_t tariff,
                       nd_energy_resolution_t resolution);

int64_t nd_energy_read_combined(const nd_energy_book_t *book, uint8_t tariff,
                                nd_energy_resolution_t resolution);
uint32_t nd_energy_channel_add_pulses(nd_energy_channel_t *channel,
                                      uint8_t tariff, uint32_t pulses,
                                      uint32_t meter_const);
int64_t nd_energy_channel_read_raw(const nd_energy_channel_t *channel,
                                   uint8_t tariff, uint32_t meter_const);
int64_t nd_energy_channel_read(const nd_energy_channel_t *channel,
                               uint8_t tariff, uint32_t meter_const,
                               nd_energy_resolution_t resolution);

void nd_energy_clear(nd_energy_book_t *book);

void nd_energy_set_total(nd_energy_book_t *book, nd_energy_direction_t direction,
                         uint32_t integer);

#endif
