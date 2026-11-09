#include "unity.h"

#include "domain/common/allocator.h"
#include "support/fake_allocator.h"

static unsigned char arena[64];
static fake_allocator_t fake;

void setUp(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
}

void tearDown(void) {}

static void test_alloc_returns_zeroed_memory(void)
{
    fake_allocator_init(&fake, arena, sizeof(arena));
    arena[0] = 0xAA;
    arena[1] = 0xAA;

    unsigned char *block = nd_alloc(&fake.port, 8);

    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL_UINT8(0, block[0]);
    TEST_ASSERT_EQUAL_UINT8(0, block[1]);
}

static void test_alloc_reports_exhaustion(void)
{
    void *block = nd_alloc(&fake.port, sizeof(arena) + 1);

    TEST_ASSERT_NULL(block);
    TEST_ASSERT_EQUAL_size_t(1, fake.alloc_calls);
}

static void test_dealloc_forwards_to_port(void)
{
    void *block = nd_alloc(&fake.port, 4);

    nd_dealloc(&fake.port, block, 4);

    TEST_ASSERT_EQUAL_size_t(1, fake.dealloc_calls);
}

static void test_missing_port_yields_null(void)
{
    TEST_ASSERT_NULL(nd_alloc(NULL, 8));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc_returns_zeroed_memory);
    RUN_TEST(test_alloc_reports_exhaustion);
    RUN_TEST(test_dealloc_forwards_to_port);
    RUN_TEST(test_missing_port_yields_null);
    return UNITY_END();
}
