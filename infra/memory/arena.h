#ifndef ND_INFRA_MEMORY_ARENA_H
#define ND_INFRA_MEMORY_ARENA_H

#include <stddef.h>
#include <stdint.h>

#include "domain/common/allocator.h"

/*
 * Bump allocator over a caller-owned buffer. It implements the domain
 * allocator port. dealloc only reclaims when the freed block is the most
 * recent allocation, which matches the lifetime of a composition root.
 */
#define ND_ARENA_ALIGNMENT 8

typedef struct {
    nd_allocator_t port;
    uint8_t *base;
    size_t capacity;
    size_t used;
} nd_arena_t;

void nd_arena_init(nd_arena_t *arena, void *buffer, size_t capacity);
void nd_arena_reset(nd_arena_t *arena);
size_t nd_arena_used(const nd_arena_t *arena);
nd_allocator_t *nd_arena_allocator(nd_arena_t *arena);

#endif
