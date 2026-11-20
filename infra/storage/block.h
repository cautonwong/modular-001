#ifndef ND_INFRA_STORAGE_BLOCK_H
#define ND_INFRA_STORAGE_BLOCK_H

#include <stddef.h>
#include <stdint.h>

/* Port B: the raw device the pulse engine needs. A NOR device programs 1 to 0
 * and erases whole sectors; EEPROM, RAM and files set erase_unit to 1. */
typedef struct {
    void *ctx;
    int (*read)(void *ctx, uint32_t offset, uint8_t *buffer, size_t length);
    int (*program)(void *ctx, uint32_t offset, const uint8_t *buffer,
                   size_t length);
    int (*erase)(void *ctx, uint32_t offset, uint32_t length);
    uint32_t size;
    uint32_t erase_unit;
} nd_block_ops_t;

/* Only the bytes the caller is clearing are ever programmed; a torn program
 * can leave a bit at 1 but cannot set one. */
static inline int nd_block_read(const nd_block_ops_t *ops, uint32_t offset,
                                uint8_t *buffer, size_t length)
{
    return ops->read(ops->ctx, offset, buffer, length);
}

static inline int nd_block_program(const nd_block_ops_t *ops, uint32_t offset,
                                   const uint8_t *buffer, size_t length)
{
    return ops->program(ops->ctx, offset, buffer, length);
}

/* Range must be aligned to erase_unit and afterwards read 0xFF. */
static inline int nd_block_erase(const nd_block_ops_t *ops, uint32_t offset,
                                 uint32_t length)
{
    return ops->erase(ops->ctx, offset, length);
}

#endif
