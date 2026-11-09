#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

#include <stdint.h>

#ifndef __no_init
#if defined(__GNUC__)
#define __no_init __attribute__((section(".noinit")))
#else
#define __no_init
#endif
#endif
#undef  NULL
#define NULL ((void *)0)
typedef uint32_t leader_id_t;
typedef uint32_t filter_id_t;
typedef uint32_t object_id_t;
typedef uint32_t context_id_t;
typedef uint32_t worker_mask_t;
typedef uint32_t work_flags_t;
typedef uint8_t pid_t;
#define _PID_T_DECLARED /* keep newlib sys/types.h from redefining pid_t */
typedef uint8_t data_type_t;
typedef uint8_t tasklet_id_t;
typedef uint32_t pipe_id_t;
typedef uint32_t memory_id_t;
typedef uint32_t dev_id_t;
typedef uint32_t app_id_t;
#endif