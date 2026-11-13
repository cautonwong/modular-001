#include "domain/events/program_list.h"

uint8_t nd_program_list_insert_count(uint8_t count)
{
    if (count > ND_PROGRAM_LIST_CAPACITY) {
        return 0;
    }
    if (count == ND_PROGRAM_LIST_CAPACITY) {
        return (uint8_t)(ND_PROGRAM_LIST_CAPACITY - 1);
    }
    return count;
}

uint8_t nd_program_list_is_first(uint8_t count)
{
    return (uint8_t)(count == 0);
}

uint8_t nd_program_list_sets_time_698(uint32_t oad)
{
    return (uint8_t)(oad == 0x40000200u);
}

uint8_t nd_program_list_sets_time_645(uint32_t oad)
{
    return (uint8_t)(oad == 0x04000101u || oad == 0x04000102u
                     || oad == 0x0400010Cu);
}
