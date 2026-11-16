#ifndef ND_INFRA_SOC_V85XXP_V8500_LINKER_SYMBOLS_H
#define ND_INFRA_SOC_V85XXP_V8500_LINKER_SYMBOLS_H

#include <stdint.h>

/* Provided by the target linker script; consumed by the startup code. */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

#endif
