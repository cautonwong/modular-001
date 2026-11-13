#include <stdint.h>

struct instance_t;

/* Legacy board drivers call the resource tool for calibration data; the
 * bring-up image has no resource store yet. */
int16_t nd_get_index(struct instance_t *instance, uint32_t index, uint8_t *buf,
                     uint16_t size)
{
    (void)instance;
    (void)index;
    (void)buf;
    (void)size;
    return -1;
}

uint16_t nd_meter_status_metertype(struct instance_t *instance)
{
    (void)instance;
    return 0;
}

void nd_meter_status_emu_error_set(struct instance_t *instance, int is_error)
{
    (void)instance;
    (void)is_error;
}
