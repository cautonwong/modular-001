/**
* @ingroup     List Util
* @brief       
*
* @{
* @file
*
*/
#include "list.h"

void list_add(struct list_head *new_entry,
              struct list_head *list)
{
    struct list_head *list_next = list->next;
    
    list->next = new_entry;
    new_entry->prev = list;
    new_entry->next = list_next;
    list_next->prev = new_entry;
    
}

void list_add_tail(struct list_head *new_entry,
                   struct list_head *list)
{
    struct list_head *list_prev = list->prev;
    
    list->prev = new_entry;
    new_entry->next = list;
    new_entry->prev = list_prev;
    list_prev->next = new_entry;
}

void list_del(struct list_head *entry)
{
    struct list_head *list_next = entry->next;
    struct list_head *list_prev = entry->prev;
    
    list_next->prev = list_prev;
    list_prev->next = list_next;
    
}

void list_del_init(struct list_head *entry)
{
    list_del(entry);
    entry->next = entry->prev = entry;
}


int list_empty(struct list_head *entry)
{
    return (entry->next == entry);
}
/** @} */
