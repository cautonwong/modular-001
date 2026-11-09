#include "infra/memory/arena.h"

#include <string.h>

static size_t align_up(size_t value)
{
    size_t mask = ND_ARENA_ALIGNMENT - 1;
    return (value + mask) & ~mask;
}

static void *arena_alloc(void *ctx, size_t size)
{
    nd_arena_t *arena = ctx;
    size_t aligned = align_up(size);

    if (size == 0 || aligned > arena->capacity - arena->used) {
        return NULL;
    }

    uint8_t *result = arena->base + arena->used;
    arena->used += aligned;
    memset(result, 0, size);
    return result;
}

static void arena_dealloc(void *ctx, void *ptr, size_t size)
{
    nd_arena_t *arena = ctx;
    size_t aligned = align_up(size);

    if (ptr == NULL) {
        return;
    }
    if ((uint8_t *)ptr + aligned == arena->base + arena->used) {
        arena->used = (size_t)((uint8_t *)ptr - arena->base);
    }
}

void nd_arena_init(nd_arena_t *arena, void *buffer, size_t capacity)
{
    arena->port.ctx = arena;
    arena->port.alloc = arena_alloc;
    arena->port.dealloc = arena_dealloc;
    arena->base = buffer;
    arena->capacity = capacity;
    arena->used = 0;
}

void nd_arena_reset(nd_arena_t *arena)
{
    arena->used = 0;
}

size_t nd_arena_used(const nd_arena_t *arena)
{
    return arena->used;
}

nd_allocator_t *nd_arena_allocator(nd_arena_t *arena)
{
    return &arena->port;
}
