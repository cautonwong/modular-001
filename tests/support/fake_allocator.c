#include "support/fake_allocator.h"

#include <string.h>

#include "infra/memory/arena.h"

static void *fake_alloc(void *ctx, size_t size)
{
    fake_allocator_t *fake = ctx;
    size_t aligned = (size + ND_ARENA_ALIGNMENT - 1)
                     & ~(size_t)(ND_ARENA_ALIGNMENT - 1);

    fake->alloc_calls++;
    if (aligned > fake->capacity - fake->used) {
        return NULL;
    }

    void *result = fake->arena + fake->used;
    fake->used += aligned;
    memset(result, 0, size);
    return result;
}

static void fake_dealloc(void *ctx, void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    fake_allocator_t *fake = ctx;
    fake->dealloc_calls++;
}

void fake_allocator_init(fake_allocator_t *fake, unsigned char *arena, size_t capacity)
{
    uintptr_t address = (uintptr_t)arena;
    size_t skip = (ND_ARENA_ALIGNMENT - (address % ND_ARENA_ALIGNMENT))
                  % ND_ARENA_ALIGNMENT;

    fake->port.ctx = fake;
    fake->port.alloc = fake_alloc;
    fake->port.dealloc = fake_dealloc;
    fake->arena = arena + skip;
    fake->capacity = skip < capacity ? capacity - skip : 0;
    fake->used = 0;
    fake->alloc_calls = 0;
    fake->dealloc_calls = 0;
}
