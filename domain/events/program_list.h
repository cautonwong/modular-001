#ifndef ND_DOMAIN_EVENTS_PROGRAM_LIST_H
#define ND_DOMAIN_EVENTS_PROGRAM_LIST_H

#include <stdint.h>

#define ND_PROGRAM_LIST_CAPACITY 10

/* Normalizes a stored entry count before inserting the next entry. */
uint8_t nd_program_list_insert_count(uint8_t count);

/* True when the next insertion starts a new program event. */
uint8_t nd_program_list_is_first(uint8_t count);

uint8_t nd_program_list_sets_time_698(uint32_t oad);
uint8_t nd_program_list_sets_time_645(uint32_t oad);

#endif
