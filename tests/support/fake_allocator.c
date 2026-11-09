#include "support/fake_allocator.h"

#include <string.h>

static void *fake_alloc(void *ctx, size_t size)
{
    fake_allocator_t *fake = ctx;
    size_t aligned = (size + 3u) & ~(size_t)3u;

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
    fake->port.ctx = fake;
    fake->port.alloc = fake_alloc;
    fake->port.dealloc = fake_dealloc;
    fake->arena = arena;
    fake->capacity = capacity;
    fake->used = 0;
    fake->alloc_calls = 0;
    fake->dealloc_calls = 0;
}
