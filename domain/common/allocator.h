#ifndef ND_DOMAIN_COMMON_ALLOCATOR_H
#define ND_DOMAIN_COMMON_ALLOCATOR_H

#include <stddef.h>

/*
 * The only way domain code obtains memory. Every aggregate receives one at
 * construction. The implementation lives in infra and is injected by the
 * composition root.
 *
 * nd_alloc returns zeroed memory so aggregates can rely on a single invariant.
 * dealloc takes the size because arena and pool allocators reclaim by span.
 */
typedef struct nd_allocator {
    void *ctx;
    void *(*alloc)(void *ctx, size_t size);
    void (*dealloc)(void *ctx, void *ptr, size_t size);
} nd_allocator_t;

static inline void *nd_alloc(const nd_allocator_t *allocator, size_t size)
{
    if (allocator == NULL || allocator->alloc == NULL) {
        return NULL;
    }
    return allocator->alloc(allocator->ctx, size);
}

static inline void nd_dealloc(const nd_allocator_t *allocator, void *ptr, size_t size)
{
    if (allocator != NULL && allocator->dealloc != NULL) {
        allocator->dealloc(allocator->ctx, ptr, size);
    }
}

#endif
