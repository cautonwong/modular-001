#ifndef ND_TESTS_SIM_COMPAT_H
#define ND_TESTS_SIM_COMPAT_H

/*
 * Host compatibility shim for the stategrid_meter firmware simulation.
 *
 * The firmware is written for newlib and the Cortex-M0 CMSIS headers. This
 * header is force-included into every simulation translation unit so the
 * portable application code can be compiled by a Linux host compiler without
 * pulling in the register-level CMSIS implementation.
 */

#include <stdint.h>

/*
 * nidavellir/include/kernel/types.h defines pid_t as uint8_t and guards it
 * with newlib's _PID_T_DECLARED. glibc uses __pid_t_defined instead, and its
 * time.h/sys/types.h would otherwise typedef a conflicting int pid_t. Define
 * both guards before any system header is seen and keep the firmware's
 * one-byte type.
 */
#ifndef _PID_T_DECLARED
typedef unsigned char pid_t;
#define _PID_T_DECLARED 1
#endif
#ifndef __pid_t_defined
#define __pid_t_defined 1
#endif

/* IAR/ARM compiler keywords used by the firmware sources. */
#ifndef __no_init
#define __no_init
#endif
#ifndef __ramfunc
#define __ramfunc
#endif
#ifndef __root
#define __root
#endif
#ifndef __weak
#define __weak __attribute__((weak))
#endif
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#ifndef __align
#define __align(n) __attribute__((aligned(n)))
#endif
#ifndef __irq
#define __irq
#endif
#ifndef __task
#define __task
#endif
#ifndef __INLINE
#define __INLINE inline
#endif
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif
#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE static inline __attribute__((always_inline))
#endif
#ifndef __USED
#define __USED __attribute__((used))
#endif
#ifndef __UNUSED
#define __UNUSED __attribute__((unused))
#endif
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif
#ifndef __NO_RETURN
#define __NO_RETURN __attribute__((noreturn))
#endif
#ifndef __PACKED
#define __PACKED __attribute__((packed))
#endif
#ifndef __COMPILER_BARRIER
#define __COMPILER_BARRIER() __sync_synchronize()
#endif

/* CMSIS register qualifiers, normally supplied by cmsis_compiler.h. */
#ifndef __I
#define __I volatile const
#endif
#ifndef __O
#define __O volatile
#endif
#ifndef __IO
#define __IO volatile
#endif
#ifndef __IM
#define __IM volatile const
#endif
#ifndef __OM
#define __OM volatile
#endif
#ifndef __IOM
#define __IOM volatile
#endif

/*
 * Cortex-M0 intrinsics. On the host there is no interrupt controller or
 * special-purpose register; the simulation only needs these to compile.
 */
#ifndef __disable_irq
#define __disable_irq() ((void)0)
#endif
#ifndef __enable_irq
#define __enable_irq() ((void)0)
#endif
#ifndef __get_PRIMASK
#define __get_PRIMASK() (0u)
#endif
#ifndef __set_PRIMASK
#define __set_PRIMASK(mask) ((void)(mask))
#endif
#ifndef __get_IPSR
#define __get_IPSR() (0u)
#endif
#ifndef __get_APSR
#define __get_APSR() (0u)
#endif
#ifndef __get_xPSR
#define __get_xPSR() (0u)
#endif
#ifndef __get_CONTROL
#define __get_CONTROL() (0u)
#endif
#ifndef __set_CONTROL
#define __set_CONTROL(value) ((void)(value))
#endif
#ifndef __get_MSP
#define __get_MSP() ((uint32_t)0)
#endif
#ifndef __set_MSP
#define __set_MSP(value) ((void)(value))
#endif
#ifndef __get_PSP
#define __get_PSP() ((uint32_t)0)
#endif
#ifndef __set_PSP
#define __set_PSP(value) ((void)(value))
#endif
#ifndef __NOP
#define __NOP() ((void)0)
#endif
#ifndef __WFI
#define __WFI() ((void)0)
#endif
#ifndef __WFE
#define __WFE() ((void)0)
#endif
#ifndef __SEV
#define __SEV() ((void)0)
#endif
#ifndef __ISB
#define __ISB() __sync_synchronize()
#endif
#ifndef __DSB
#define __DSB() __sync_synchronize()
#endif
#ifndef __DMB
#define __DMB() __sync_synchronize()
#endif
#ifndef __CLZ
#define __CLZ(value) ((uint32_t)__builtin_clz((uint32_t)(value)))
#endif
#ifndef __REV
#define __REV(value) ((uint32_t)__builtin_bswap32((uint32_t)(value)))
#endif
#ifndef __REV16
#define __REV16(value) ((uint32_t)((((uint32_t)(value) & 0x00ff00ffu) << 8) | \
                                   (((uint32_t)(value) & 0xff00ff00u) >> 8)))
#endif
#ifndef __REVSH
#define __REVSH(value) ((uint32_t)(int32_t)(int16_t)__builtin_bswap16((uint16_t)(value)))
#endif
#ifndef __SSAT
#define __SSAT(value, sat) ((int32_t)(value))
#endif
#ifndef __USAT
#define __USAT(value, sat) ((uint32_t)(value))
#endif
#ifndef __LDREXW
#define __LDREXW(addr) (*(volatile uint32_t *)(addr))
#endif
#ifndef __STREXW
#define __STREXW(value, addr) (*(volatile uint32_t *)(addr) = (uint32_t)(value), 0u)
#endif
#ifndef __CLREX
#define __CLREX() ((void)0)
#endif
#ifndef __BKPT
#define __BKPT(value) ((void)(value))
#endif

/* CMSIS core helper used by the firmware; there is no reset controller here. */
void nd_sim_system_reset(void);
#ifndef NVIC_SystemReset
#define NVIC_SystemReset() nd_sim_system_reset()
#endif

#endif /* ND_TESTS_SIM_COMPAT_H */
