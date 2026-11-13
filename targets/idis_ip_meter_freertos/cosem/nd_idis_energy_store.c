#include "targets/idis_ip_meter_freertos/cosem/nd_idis_energy_store.h"

#include <stddef.h>

#define ND_IDIS_ENERGY_MAGIC 0x454E4531u

typedef struct {
    uint32_t magic;
    nd_energy_set_t set;
} nd_idis_energy_record_t;

int nd_idis_energy_save(const nd_energy_set_t *set, nd_store_t *store,
                        uint32_t offset)
{
    if (set == NULL || store == NULL) {
        return -1;
    }
    nd_idis_energy_record_t record = {
        .magic = ND_IDIS_ENERGY_MAGIC,
        .set = *set,
    };
    return nd_store_write(store, offset, (const uint8_t *)&record,
                          sizeof(record));
}

int nd_idis_energy_load(nd_energy_set_t *set, nd_store_t *store,
                        uint32_t offset)
{
    if (set == NULL || store == NULL) {
        return -1;
    }
    nd_idis_energy_record_t record;
    if (nd_store_read(store, offset, (uint8_t *)&record, sizeof(record)) != 0
        || record.magic != ND_IDIS_ENERGY_MAGIC) {
        return -1;
    }
    *set = record.set;
    return 0;
}
