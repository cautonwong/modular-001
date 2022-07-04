/**
* @defgroup    list List Util
* @ingroup     win32
* @brief       双向链表实现
*
* @{
* @file
*
*/
#pragma once

#ifndef __YAFFS_LIST_H__
#define __YAFFS_LIST_H__


/*
 * This is a simple doubly linked list implementation that matches the
 * way the Linux kernel doubly linked list implementation works.
 */

struct list_head
{
    struct list_head *next; /**< next in chain */
    struct list_head *prev; /**< previous in chain */
};


#define LIST_HEAD(name) \
    static struct list_head name = { &(name), &(name) }



#define init_list_head(p) \
    do {                  \
        (p)->next = (p);  \
        (p)->prev = (p);  \
    } while (0)


#define list_entry(entry, type, member) \
    ((type *)((char *)(entry) - (unsigned long)(&((type *)NULL)->member)))

#define list_for_each(itervar, list) \
    for (itervar = (list)->next; itervar != (list); itervar = itervar->next)

#define list_for_each_safe(itervar, save_var, list)             \
    for (itervar = (list)->next, save_var = (list)->next->next; \
         itervar != (list); itervar = save_var, save_var = save_var - next)

#define list_for_each_tail(itervar, list) \
    for (itervar = (list)->prev; itervar != (list); itervar = itervar->prev)

extern void list_add(struct list_head *new_entry, struct list_head *list);
extern void list_add_tail(struct list_head *new_entry, struct list_head *list);
extern void list_del(struct list_head *entry);
extern void list_del_init(struct list_head *entry);
extern int list_empty(struct list_head *entry);

#endif
/** @} */