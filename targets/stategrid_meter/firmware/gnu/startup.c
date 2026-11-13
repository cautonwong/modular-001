/*
 * GNU startup for the V85XXP link check.
 *
 * This is not a replacement for the IAR runtime. It only gives the linker a
 * valid vector table, data copy and C entry point so the product object set
 * can be checked as a final image.
 */
#include <stdint.h>

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

void Default_Handler(void)
{
    for (;;) {
    }
}

__attribute__((section(".vectors"), used))
void (*const vectors[])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
};

void Reset_Handler(void)
{
    uint32_t *source = &_sidata;
    uint32_t *destination = &_sdata;

    while (destination < &_edata) {
        *destination++ = *source++;
    }
    for (destination = &_sbss; destination < &_ebss; destination++) {
        *destination = 0;
    }

    (void)main();
    for (;;) {
    }
}
