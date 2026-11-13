#include "report_list.h"

static uint32_t read_oad_lsb(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8)
           | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
}

static void write_oad_lsb(uint8_t *buffer, uint32_t oad)
{
    buffer[0] = (uint8_t)oad;
    buffer[1] = (uint8_t)(oad >> 8);
    buffer[2] = (uint8_t)(oad >> 16);
    buffer[3] = (uint8_t)(oad >> 24);
}

int nd_report_list_parse(nd_report_list_t *list, const uint8_t *buffer,
                         size_t length)
{
    uint8_t i;

    if ((list == NULL) || (buffer == NULL) || (length < 1)) {
        return -1;
    }
    if ((buffer[0] > ND_REPORT_LIST_MAX)
        || (length < 1 + (size_t)buffer[0] * 4)) {
        return -1;
    }

    list->count = buffer[0];
    for (i = 0; i < list->count; i++) {
        list->oad[i] = read_oad_lsb(buffer + 1 + (size_t)i * 4);
    }
    return 0;
}

size_t nd_report_list_encode(const nd_report_list_t *list, uint8_t *buffer,
                             size_t length)
{
    size_t needed;
    uint8_t i;

    if ((list == NULL) || (buffer == NULL)
        || (list->count > ND_REPORT_LIST_MAX)) {
        return 0;
    }

    needed = 1 + (size_t)list->count * 4;
    if (length < needed) {
        return 0;
    }

    buffer[0] = list->count;
    for (i = 0; i < list->count; i++) {
        write_oad_lsb(buffer + 1 + (size_t)i * 4, list->oad[i]);
    }
    return needed;
}

int nd_report_list_find(const nd_report_list_t *list, uint32_t oad)
{
    uint8_t i;

    if (list == NULL) {
        return -1;
    }
    for (i = 0; i < list->count; i++) {
        if (list->oad[i] == oad) {
            return i;
        }
    }
    return -1;
}

int nd_report_list_remove_at(nd_report_list_t *list, uint8_t index)
{
    uint8_t i;

    if ((list == NULL) || (index >= list->count)) {
        return 0;
    }
    for (i = index; i + 1 < list->count; i++) {
        list->oad[i] = list->oad[i + 1];
    }
    list->count--;
    return 1;
}

int nd_report_list_remove(nd_report_list_t *list, uint32_t oad)
{
    int index = nd_report_list_find(list, oad);

    if (index < 0) {
        return 0;
    }
    return nd_report_list_remove_at(list, (uint8_t)index);
}

void nd_report_list_insert_front(nd_report_list_t *list, uint32_t oad)
{
    uint8_t i;
    uint8_t count;

    if (list == NULL) {
        return;
    }

    count = list->count;
    if (count < ND_REPORT_LIST_MAX) {
        count++;
    }
    for (i = count - 1; i > 0; i--) {
        list->oad[i] = list->oad[i - 1];
    }
    list->oad[0] = oad;
    list->count = count;
}
