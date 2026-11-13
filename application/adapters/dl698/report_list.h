#ifndef ND_APPLICATION_ADAPTERS_DL698_REPORT_LIST_H
#define ND_APPLICATION_ADAPTERS_DL698_REPORT_LIST_H

#include <stddef.h>
#include <stdint.h>

#define ND_REPORT_LIST_MAX 60

typedef struct nd_report_list {
    uint8_t count;
    uint32_t oad[ND_REPORT_LIST_MAX];
} nd_report_list_t;

int nd_report_list_parse(nd_report_list_t *list, const uint8_t *buffer,
                         size_t length);
size_t nd_report_list_encode(const nd_report_list_t *list, uint8_t *buffer,
                             size_t length);
int nd_report_list_find(const nd_report_list_t *list, uint32_t oad);
int nd_report_list_remove_at(nd_report_list_t *list, uint8_t index);
int nd_report_list_remove(nd_report_list_t *list, uint32_t oad);
void nd_report_list_insert_front(nd_report_list_t *list, uint32_t oad);

#endif
