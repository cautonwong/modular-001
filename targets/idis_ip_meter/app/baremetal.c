#include <stddef.h>
#include "baremetal.h"

#include "time_link.h"

static volatile uint32_t systick_ticks;

uint32_t baremetal_uptime_seconds(void)
{
    return systick_ticks / 100u;
}

void SysTick_Handler(void)
{
    systick_ticks++;
    ndv_timer_tick(NULL);
}

/* Bare metal never uses SVC or PendSV, but keeping the entries named makes the
 * vector table explicit and leaves a clean seam for an RTOS target copy. */
void SVC_Handler(void)
{
    for (;;) {
    }
}

void PendSV_Handler(void)
{
    for (;;) {
    }
}
