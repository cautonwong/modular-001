#ifndef ND_TESTS_SIM_CORE_CM0_H
#define ND_TESTS_SIM_CORE_CM0_H

/*
 * Portable stand-in for the CMSIS core_cm0.h header. It is placed before the
 * real header on the include path for the host simulation. The device header
 * (infra/soc/v85xxp inc/target.h) supplies IRQn_Type and the register
 * structures; only the core constants and qualifiers are needed here.
 */

#include <stdint.h>

#define __NVIC_PRIO_BITS 2

#endif /* ND_TESTS_SIM_CORE_CM0_H */
