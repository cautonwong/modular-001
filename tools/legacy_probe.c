/*
 * Prints the persisted offsets of the legacy aggregate root. Run from the repo
 * root:
 *
 *   gcc -std=gnu11 -DWIN32 -I nidavellir/include -I targets/stategrid_meter \
 *       tools/legacy_probe.c -o /tmp/legacy_probe && /tmp/legacy_probe
 *
 * The values are frozen in
 * targets/stategrid_meter/persistence/legacy_energy_record.h. Rerun this when
 * the legacy headers change, and update the static assertions.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "applications.h"

#define PRINT_OFFSET(field) \
    printf("%-40s %5zu\n", #field, offsetof(struct mbase_control_block_t, field))

int main(void)
{
    printf("sizeof(struct mbase_control_block_t) = %zu\n",
           sizeof(struct mbase_control_block_t));
    PRINT_OFFSET(static_section_start);
    PRINT_OFFSET(static_section_end);
    PRINT_OFFSET(cache_section_start);
    PRINT_OFFSET(cache_section_end);
    PRINT_OFFSET(persistent_section_start);
    PRINT_OFFSET(active_p);
    PRINT_OFFSET(active_n);
    PRINT_OFFSET(active_p_dec);
    PRINT_OFFSET(active_n_dec);
    PRINT_OFFSET(Energy_chksum);
    PRINT_OFFSET(MonthEnegery_dec);
    PRINT_OFFSET(MonthEnegery);
    PRINT_OFFSET(UseEnergy_chksum);
    PRINT_OFFSET(persistent_energy_EC_label);
    PRINT_OFFSET(persistent_section_end);
    return 0;
}
