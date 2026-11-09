#ifndef ND_TEST_FAKE_ALLOCATOR_H
#define ND_TEST_FAKE_ALLOCATOR_H

#include <stddef.h>

#include "domain/common/allocator.h"

typedef struct {
    nd_allocator_t port;
    unsigned char *arena;
    size_t capacity;
    size_t used;
    size_t alloc_calls;
    size_t dealloc_calls;
} fake_allocator_t;

void fake_allocator_init(fake_allocator_t *fake, unsigned char *arena, size_t capacity);

#endif
