#include "unity.h"

#include <stddef.h>

#include "infra/kernel/list.h"

struct node {
    int value;
    struct list_head link;
};

static struct list_head head;

void setUp(void)
{
    init_list_head(&head);
}

void tearDown(void) {}

static void test_empty_and_add_order(void)
{
    struct node first = {1, {0}};
    struct node second = {2, {0}};
    struct node third = {3, {0}};

    TEST_ASSERT_TRUE(list_empty(&head));

    list_add(&first.link, &head);
    list_add(&second.link, &head);
    list_add_tail(&third.link, &head);

    TEST_ASSERT_FALSE(list_empty(&head));
    TEST_ASSERT_EQUAL_PTR(&second, list_entry(head.next, struct node, link));
    TEST_ASSERT_EQUAL_PTR(&third, list_entry(head.next->next->next, struct node, link));
}

static void test_iteration_and_entry_conversion(void)
{
    struct node first = {1, {0}};
    struct node second = {2, {0}};
    struct node third = {3, {0}};
    list_add_tail(&first.link, &head);
    list_add_tail(&second.link, &head);
    list_add_tail(&third.link, &head);

    int values[3] = {0};
    int index = 0;
    struct list_head *cursor;
    list_for_each(cursor, &head) {
        values[index++] = list_entry(cursor, struct node, link)->value;
    }

    TEST_ASSERT_EQUAL_INT(3, index);
    TEST_ASSERT_EQUAL_INT(1, values[0]);
    TEST_ASSERT_EQUAL_INT(2, values[1]);
    TEST_ASSERT_EQUAL_INT(3, values[2]);
}

static void test_delete_removes_entries(void)
{
    struct node first = {1, {0}};
    struct node second = {2, {0}};
    struct node third = {3, {0}};
    list_add_tail(&first.link, &head);
    list_add_tail(&second.link, &head);
    list_add_tail(&third.link, &head);

    list_del(&second.link);
    TEST_ASSERT_EQUAL_PTR(&first.link, head.next);
    TEST_ASSERT_EQUAL_PTR(&third.link, head.next->next);

    list_del_init(&first.link);
    TEST_ASSERT_EQUAL_PTR(&first.link, first.link.next);
    TEST_ASSERT_EQUAL_PTR(&first.link, first.link.prev);

    list_del_init(&third.link);
    TEST_ASSERT_TRUE(list_empty(&head));
}

static void test_safe_iteration_handles_deletion(void)
{
    struct node first = {1, {0}};
    struct node second = {2, {0}};
    struct node third = {3, {0}};
    list_add_tail(&first.link, &head);
    list_add_tail(&second.link, &head);
    list_add_tail(&third.link, &head);

    struct list_head *cursor;
    struct list_head *save;
    list_for_each_safe(cursor, save, &head) {
        list_del(cursor);
    }

    TEST_ASSERT_TRUE(list_empty(&head));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_and_add_order);
    RUN_TEST(test_iteration_and_entry_conversion);
    RUN_TEST(test_delete_removes_entries);
    RUN_TEST(test_safe_iteration_handles_deletion);
    return UNITY_END();
}
