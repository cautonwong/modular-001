#include "unity.h"

#include <stddef.h>

#include "infra/memory/arena.h"

static unsigned char buffer[64];
static nd_arena_t arena;

void setUp(void)
{
    nd_arena_init(&arena, buffer, sizeof(buffer));
}

void tearDown(void) {}

static void test_alloc_returns_zeroed_aligned_blocks(void)
{
    unsigned char *first = nd_alloc(nd_arena_allocator(&arena), 4);
    unsigned char *second = nd_alloc(nd_arena_allocator(&arena), 8);

    TEST_ASSERT_EQUAL_PTR(buffer, first);
    TEST_ASSERT_EQUAL_PTR(buffer + ND_ARENA_ALIGNMENT, second);
    TEST_ASSERT_EQUAL_size_t(16, nd_arena_used(&arena));
    for (size_t index = 0; index < 4; index++) {
        TEST_ASSERT_EQUAL_UINT8(0, first[index]);
    }
}

static void test_exhaustion_returns_null(void)
{
    TEST_ASSERT_NOT_NULL(nd_alloc(nd_arena_allocator(&arena), 64));
    TEST_ASSERT_NULL(nd_alloc(nd_arena_allocator(&arena), 1));
}

static void test_zero_size_returns_null(void)
{
    TEST_ASSERT_NULL(nd_alloc(nd_arena_allocator(&arena), 0));
}

static void test_most_recent_block_is_reclaimed(void)
{
    unsigned char *block = nd_alloc(nd_arena_allocator(&arena), 16);
    TEST_ASSERT_EQUAL_size_t(16, nd_arena_used(&arena));

    nd_dealloc(nd_arena_allocator(&arena), block, 16);

    TEST_ASSERT_EQUAL_size_t(0, nd_arena_used(&arena));
}

static void test_lower_block_is_not_reclaimed(void)
{
    unsigned char *block = nd_alloc(nd_arena_allocator(&arena), 16);
    nd_alloc(nd_arena_allocator(&arena), 16);

    nd_dealloc(nd_arena_allocator(&arena), block, 16);

    TEST_ASSERT_EQUAL_size_t(32, nd_arena_used(&arena));
}

static void test_reset_reclaims_everything(void)
{
    nd_alloc(nd_arena_allocator(&arena), 32);

    nd_arena_reset(&arena);

    TEST_ASSERT_EQUAL_size_t(0, nd_arena_used(&arena));
    TEST_ASSERT_EQUAL_PTR(buffer, nd_alloc(nd_arena_allocator(&arena), 4));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc_returns_zeroed_aligned_blocks);
    RUN_TEST(test_exhaustion_returns_null);
    RUN_TEST(test_zero_size_returns_null);
    RUN_TEST(test_most_recent_block_is_reclaimed);
    RUN_TEST(test_lower_block_is_not_reclaimed);
    RUN_TEST(test_reset_reclaims_everything);
    return UNITY_END();
}
