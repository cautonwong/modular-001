#ifndef ND_DOMAIN_FREEZE_STORAGE_H
#define ND_DOMAIN_FREEZE_STORAGE_H

#include <stdint.h>

#define ND_FREEZE_SECTOR_BYTES 4096u

/* Storage units for one freeze scheme. Flash counts sectors including the
 * reserved one; EEPROM counts bytes including the reserved byte. */
uint32_t nd_freeze_scheme_units(uint8_t on_flash, uint32_t record_bytes,
                                uint32_t depth);

#endif
