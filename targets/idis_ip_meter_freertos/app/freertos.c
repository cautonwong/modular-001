#include "freertos_app.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "time_link.h"

#define APP_TASK_STACK_WORDS 768u
#define APP_TASK_PRIORITY 3u

static TaskHandle_t app_task_handle;
static TaskHandle_t worker_task_handle;
static QueueHandle_t save_queue;

#define SAVE_QUEUE_DEPTH 1u
#define SAVE_WORK_ITEM 1u

uint32_t freertos_uptime_seconds(void)
{
    return (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
}

void vApplicationMallocFailedHook(void)
{
    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    for (;;) {
    }
}

static void worker_task(void *argument)
{
    (void)argument;
    uint8_t item;
    for (;;) {
        if (xQueueReceive(save_queue, &item, portMAX_DELAY) == pdPASS
            && item == SAVE_WORK_ITEM) {
            idis_save_persisted();
        }
    }
}

void freertos_request_save(void)
{
    uint8_t item = SAVE_WORK_ITEM;
    if (save_queue != NULL) {
        (void)xQueueSend(save_queue, &item, 0);
    }
}

static void app_task(void *argument)
{
    (void)argument;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        ndv_timer_tick(NULL);
        idis_app_tick();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}

void freertos_start(void)
{
    save_queue = xQueueCreate(SAVE_QUEUE_DEPTH, sizeof(uint8_t));
    if (save_queue == NULL
        || xTaskCreate(app_task, "app", APP_TASK_STACK_WORDS, NULL,
                       APP_TASK_PRIORITY, &app_task_handle) != pdPASS
        || xTaskCreate(worker_task, "work", 512, NULL, 1,
                       &worker_task_handle) != pdPASS) {
        for (;;) {
        }
    }
    vTaskStartScheduler();
    for (;;) {
    }
}
