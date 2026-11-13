#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

#define configCPU_CLOCK_HZ                      26214400u
#define configTICK_RATE_HZ                      100u
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                128u
#define configMAX_TASK_NAME_LEN                 4
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            0
#define configUSE_MUTEXES                       0
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configUSE_TIMERS                        0
#define configUSE_CO_ROUTINES                   0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_TRACE_FACILITY                0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configCHECK_HANDLER_INSTALLATION        0
#define configENABLE_MPU                        0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   (12u * 1024u)
#define configUSE_TICKLESS_IDLE                 0
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    0xC0u
#define configKERNEL_INTERRUPT_PRIORITY         0xC0u
#define configASSERT(x) do { if (!(x)) { for (;;) { } } } while (0)

#define INCLUDE_vTaskDelay                      0
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_xTaskGetTickCount               1
#define INCLUDE_xTaskGetSchedulerState          0
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xQueueGetMutexHolder            0

#endif
