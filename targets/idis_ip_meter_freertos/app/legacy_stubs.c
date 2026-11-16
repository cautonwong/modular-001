#include <stdint.h>

/* Legacy board drivers call the resource tool for calibration data; the
 * bring-up image has no resource store yet. */
int16_t nd_get_index(void *ctx, uint32_t index, uint8_t *buf,
                     uint16_t size)
{
    (void)ctx;
    (void)index;
    (void)buf;
    (void)size;
    return -1;
}

uint16_t nd_meter_status_metertype(void *ctx)
{
    (void)ctx;
    return 0;
}

void nd_meter_status_emu_error_set(void *ctx, int is_error)
{
    (void)ctx;
    (void)is_error;
}
