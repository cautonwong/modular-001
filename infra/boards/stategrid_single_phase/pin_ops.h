#ifndef ND_BOARD_PIN_OPS_H
#define ND_BOARD_PIN_OPS_H

#include <stdint.h>

static inline uint32_t nd_pin_bitband_addr(uint32_t addr, uint32_t bit)
{
    return (addr & 0xF0000000u) + 0x02000000u
           + ((addr & 0x000FFFFFu) << 5) + (bit << 2);
}

static inline void nd_pin_reg_set(uint32_t addr, uint32_t mask)
{
    *(volatile uint32_t *)addr |= mask;
}

static inline void nd_pin_reg_clear(uint32_t addr, uint32_t mask)
{
    *(volatile uint32_t *)addr &= ~mask;
}

static inline void nd_pin_write(uint32_t addr, uint32_t bit, int high)
{
    *(volatile uint32_t *)nd_pin_bitband_addr(addr, bit) = high ? 1u : 0u;
}

static inline int nd_pin_read(uint32_t addr, uint32_t bit)
{
    return (int)*(volatile uint32_t *)nd_pin_bitband_addr(addr, bit);
}

#endif
