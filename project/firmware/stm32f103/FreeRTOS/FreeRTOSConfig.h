#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---- clock & tick ---- */
#define configCPU_CLOCK_HZ                  ((uint32_t)72000000)
#define configTICK_RATE_HZ                  ((TickType_t)1000)

/* ---- scheduler ---- */
#define configUSE_PREEMPTION                1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configMAX_PRIORITIES                5
#define configIDLE_SHOULD_YIELD             1
#define configMINIMAL_STACK_SIZE            ((uint16_t)128)   /* words, idle task */
#define configMAX_TASK_NAME_LEN             16
#define configUSE_16_BIT_TICKS              0

/* ---- heap (heap_4), 48KB RAM total, keep <=16KB for M1 ---- */
#define configTOTAL_HEAP_SIZE               ((size_t)(16 * 1024))

/* ---- sync objects ---- */
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         0
#define configUSE_COUNTING_SEMAPHORES       1
#define configQUEUE_REGISTRY_SIZE           0

/* ---- timers: not used in M1, keep off so timers.c not needed ---- */
#define configUSE_TIMERS                    0

/* ---- debug aids ---- */
#define configCHECK_FOR_STACK_OVERFLOW      2
#define configUSE_MALLOC_FAILED_HOOK        1
#define configUSE_IDLE_HOOK                 0
#define configUSE_TICK_HOOK                 0
#define configUSE_TRACE_FACILITY            0
#define configGENERATE_RUN_TIME_STATS       0

/* ---- Cortex-M3 NVIC: 4-bit preempt, no sub-priority ----
   IRQ priority 0..4 reserved for kernel-critical;
   5..15 may call FreeRTOS ISR-safe APIs. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  (5 << 4)
#define configKERNEL_INTERRUPT_PRIORITY       (15 << 4)

/* ---- map FreeRTOS handlers onto SPL startup vector names ---- */
#define vPortSVCHandler      SVC_Handler
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler

/* ---- optional API surface ---- */
#define INCLUDE_vTaskDelay            1
#define INCLUDE_vTaskDelayUntil       1
#define INCLUDE_vTaskSuspend          1
#define INCLUDE_vTaskDelete           1
#define INCLUDE_vTaskPrioritySet      1
#define INCLUDE_uxTaskPriorityGet     1
#define INCLUDE_xTaskGetSchedulerState 1

#endif /* FREERTOS_CONFIG_H */

